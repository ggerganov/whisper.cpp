#include "common-grpc.h"

#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <stdio.h>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <google/protobuf/util/time_util.h>

#include "transcription.grpc.pb.h"


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using google::protobuf::util::TimeUtil;
using google::protobuf::Timestamp;
using whispercpp::transcription::AudioSegmentRequest;
using whispercpp::transcription::TranscriptResponse;


audio_async::audio_async(int len_ms) {
    m_len_ms = len_ms;
    m_running = false;
}

audio_async::~audio_async() {
    grpc_shutdown();
}

bool audio_async::init(std::string grpc_server_host, int grpc_server_port, int sample_rate) {

    // audio buffer param init
    m_sample_rate = sample_rate;
    m_audio.resize((m_sample_rate*m_len_ms)/1000);

    // gRPC server startup
    grpc::EnableDefaultHealthCheckService(true);
    std::string grpc_server_address = grpc_server_host + ":" + std::to_string(grpc_server_port);
    grpc_start_async_service(grpc_server_address);
    fprintf(stdout, "\n***************** Started gRPC server at: %s *****************\n", grpc_server_address.c_str());

    return true;
}

//
// BEGIN GPRC METHODS
//
void audio_async::grpc_start_async_service(std::string server_address) {

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());    
    builder.RegisterService(&m_service);
    mup_cq = builder.AddCompletionQueue();
    mup_server = builder.BuildAndStart();

    grpc_start_new_connection_listener();
    new std::thread(&audio_async::grpc_handler_thread, this);   
}

void audio_async::grpc_shutdown() {

    mup_server->Shutdown();
    mup_cq->Shutdown();  
}

void audio_async::grpc_start_new_connection_listener() {

    std::unique_ptr<ServerContext> context = std::make_unique<ServerContext>();
    // This initiates a single stream for a single client. This implementation is for a single client/thread
    // at a time (the parallelism of the underlying model does not allow for any more than that anyway)
    m_transcript_seq_num = 1;
    mup_stream.reset(
        new ServerAsyncReaderWriter<TranscriptResponse, AudioSegmentRequest>(context.get()));
    m_service.RequestTranscribeAudio(context.get(), mup_stream.get(), mup_cq.get(), mup_cq.get(),
        reinterpret_cast<void*>(GrpcTagType::CONNECT));
    // This is important as the server should know when the client is done.
    context->AsyncNotifyWhenDone(reinterpret_cast<void*>(GrpcTagType::DONE));    
}

// gRPC async completion queue handler thread runnable method
void audio_async::grpc_handler_thread() {

    while (true) {
        if (m_running) {
            void* got_tag = nullptr;
            bool ok = false;
            if (!mup_cq->Next(&got_tag, &ok)) {
                break;
            }

            if (ok) {
                // Inline event handler and quasi-state machine (though this handles asynch reads/writes, so a strict SM is not applicable)
                switch (static_cast<GrpcTagType>(reinterpret_cast<size_t>(got_tag))) {
                case GrpcTagType::READ:
                    // Read done, great -- just ingest into processing buffer and wait for next activity (read or transcription write)
                    grpc_ingest_request_audio_data();
                    grpc_wait_for_request();
                    break;
                case GrpcTagType::CONNECT:
                    fprintf(stdout, "\n>>>>>> gRPC CLIENT CONNECTED\n");
                    m_connected = true;
                    grpc_wait_for_request();
                    break;
                case GrpcTagType::WRITE:
                    // Async write done, great -- just wait for next activity (read or new transcription write) now that it is out
                    m_writing = false;
                    break;
                case GrpcTagType::DONE:
                    m_connected = false;
                    break;
                case GrpcTagType::FINISH:
                    pause();
                    break;
                default:
                    assert(false);
                }
            } else {
                pause();
                clear();    // clear pointers to any remaining audio data for new connections
                grpc_start_new_connection_listener();   // Now we can allow another client to connect
                resume();
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }
}

void audio_async::grpc_wait_for_request() {

    if (m_connected) {
        mup_stream->Read(&m_request, reinterpret_cast<void*>(GrpcTagType::READ));
    }
}

static inline std::vector<float> convert_s16le_string_data_to_floats(std::string strdata) {

    std::vector<int16_t> intdata(strdata.size() / sizeof(int16_t));
    std::vector<float> floats(intdata.size());
    memcpy(intdata.data(), strdata.data(), strdata.size());     // assumes little endian system
    int i = 0;
    for (auto& d : intdata) {
        floats.at(i++) = (float) d / 32768.0f;
    }
    return floats;
} 

void audio_async::grpc_ingest_request_audio_data() {

    std::vector<float> sampleData = convert_s16le_string_data_to_floats(m_request.audio_data());
    callback((uint8_t*) sampleData.data(), sampleData.size()*sizeof(float));
}

static inline Timestamp* create_grpc_timestamp(int64_t epoch_ms) {

    Timestamp* pb_time = new Timestamp;
    pb_time->set_seconds(epoch_ms / 1000);
    pb_time->set_nanos(epoch_ms % 1000 * 1000000);
    return pb_time;
}

void audio_async::grpc_send_transcription(std::string transcript, int64_t start_time_ms, int64_t end_time_ms) 
{
    if (m_connected) {
      while (m_writing) {  // Let's wait for previous write to finish
        std::this_thread::sleep_for(std::chrono::milliseconds(2));    
      }
      m_writing = true;
      TranscriptResponse response;
      response.set_transcription(transcript);
      response.set_seq_num(m_transcript_seq_num++);   
      response.set_allocated_start_time(create_grpc_timestamp(start_time_ms));
      response.set_allocated_end_time(create_grpc_timestamp(end_time_ms));
      mup_stream->Write(response, reinterpret_cast<void*>(GrpcTagType::WRITE));
    } else {
      fprintf(stderr, "%s: >>>>>> CANNOT SEND TRANSCRIPTION -- NOT CONNECTED (ANYMORE)\n", __func__);
    }
}
//
// END GRPC METHODS
//

bool audio_async::resume() {

    if (m_running) {
        fprintf(stderr, "%s: already running!\n", __func__);
        return false;
    }

    m_running = true;

    return true;
}

bool audio_async::pause() {

    if (!m_running) {
        fprintf(stderr, "%s: already paused!\n", __func__);
        return false;
    }

    m_running = false;

    return true;
}

bool audio_async::clear() {

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_audio_pos = 0;
        m_audio_len = 0;
    }

    return true;
}

// callback to be called by gRPC to populate processing buffer with request data
void audio_async::callback(uint8_t * stream, int len) {

    if (!m_running) {
        return;
    }

    size_t n_samples = len / sizeof(float);

    if (n_samples > m_audio.size()) {
        n_samples = m_audio.size();

        stream += (len - (n_samples * sizeof(float)));
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_req_head_audio_timestamp_ms = TimeUtil::TimestampToMilliseconds(m_request.send_time());       

        if (m_audio_pos + n_samples > m_audio.size()) {
            const size_t n0 = m_audio.size() - m_audio_pos;

            memcpy(&m_audio[m_audio_pos], stream, n0 * sizeof(float));
            memcpy(&m_audio[0], stream + n0 * sizeof(float), (n_samples - n0) * sizeof(float));

            m_audio_pos = (m_audio_pos + n_samples) % m_audio.size();
            m_audio_len = m_audio.size();
        } else {
            memcpy(&m_audio[m_audio_pos], stream, n_samples * sizeof(float));
            m_audio_pos = (m_audio_pos + n_samples) % m_audio.size();
            m_audio_len = std::min(m_audio_len + n_samples, m_audio.size());
        }
    }
}

// method for processor (whisper) to get next chunk of audio buffer data to transcribe
void audio_async::get(int ms, std::vector<float> & result, int64_t & req_start_timestamp_ms, int64_t & req_end_timestamp_ms) {

    if (!m_running) {
        fprintf(stderr, "%s: not running!\n", __func__);
        return;
    }

    result.clear();

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (ms <= 0) {
            ms = m_len_ms;
        }

        size_t n_samples = (m_sample_rate * ms) / 1000;
        if (n_samples > m_audio_len) {
            n_samples = m_audio_len;
        }

        result.resize(n_samples);

        int s0 = m_audio_pos - n_samples;
        if (s0 < 0) {
            s0 += m_audio.size();
        }

        if (s0 + n_samples > m_audio.size()) {
            const size_t n0 = m_audio.size() - s0;

            memcpy(result.data(), &m_audio[s0], n0 * sizeof(float));
            memcpy(&result[n0], &m_audio[0], (n_samples - n0) * sizeof(float));
        } else {
            memcpy(result.data(), &m_audio[s0], n_samples * sizeof(float));
        }

        // Naive--may not account for state of circular buffer relative to requested time/size, but probably good enough
        req_start_timestamp_ms = m_req_head_audio_timestamp_ms - (n_samples / m_sample_rate) * 1000;
        req_end_timestamp_ms = m_req_head_audio_timestamp_ms;
    }
}

bool audio_async::is_running() {
    return m_running;
}