#pragma once


#include <atomic>
#include <cstdint>
#include <vector>
#include <mutex>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "transcription.grpc.pb.h"


using whispercpp::transcription::AudioTranscription;
using whispercpp::transcription::AudioSegmentRequest;
using whispercpp::transcription::TranscriptResponse;
using grpc::Server;
using grpc::ServerAsyncReaderWriter;
using grpc::ServerCompletionQueue;

//
// gRPC server audio capture (single client/thread at a time)
// The gRPC reading is done in a separate thread, via a gRPC async completion queue,
// so reading asynchronous vs. the main thread, which can write transcript segments
// as they are completed by the ASR service
//
class audio_async {
public:
    audio_async(int len_ms);
    ~audio_async();

    bool init(std::string grpc_server_host, int grpc_server_port, int sample_rate);

    // start capturing audio via the provided gRPC callback
    // keep last len_ms seconds of audio in a circular buffer
    bool resume();
    bool pause();
    bool clear();
    bool is_running();

    // get audio data from the circular buffer
    void get(int ms, std::vector<float> & audio, int64_t & req_start_timestamp_ms, int64_t & req_end_timestamp_ms);
    
    // method for async sending of transcript chunks
    void grpc_send_transcription(std::string transcript, int64_t start_time_ms = 0, int64_t end_time_ms = 0) ;

private:
    // GRPC service methods and enums/types
    enum class GrpcTagType { READ = 1, WRITE = 2, CONNECT = 3, DONE = 4, FINISH = 5 };
    void grpc_start_new_connection_listener();
    void grpc_handler_thread();
    void grpc_wait_for_request();
    void grpc_ingest_request_audio_data();
    void grpc_start_async_service(std::string server_address);
    void grpc_shutdown();

    // processing buffer load callback to be called by gRPC
    void callback(uint8_t * stream, int len);    

    int seq_num = 1;
    int m_len_ms = 0;
    int m_sample_rate = 0;

    std::atomic_bool m_running = false;
    std::mutex       m_mutex;

    std::vector<float> m_audio;
    size_t             m_audio_pos = 0;
    size_t             m_audio_len = 0;

    // GRPC service members
    std::atomic_bool m_connected = false;
    std::atomic_bool m_writing = false;
    std::unique_ptr<ServerCompletionQueue> mup_cq;
    std::unique_ptr<Server> mup_server;  
    std::unique_ptr<ServerAsyncReaderWriter<TranscriptResponse, AudioSegmentRequest>> mup_stream;
    AudioTranscription::AsyncService m_service;
    AudioSegmentRequest m_request;
    int m_transcript_seq_num = 0;
    int64_t m_req_head_audio_timestamp_ms = 0;    
};