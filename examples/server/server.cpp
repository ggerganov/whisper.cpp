#include "common.h"

#include "whisper.h"

#include "httplib.h"

#include "json.hpp"

#include <cmath>

#include <fstream>

#include <cstdio>

#include <string>

#include <thread>

#include <vector>

#include <cstring>

#include <sstream>

#include <atomic>

#include <queue>

#include <condition_variable>

#include <csignal>

#include <memory>

#include <future>


#if defined(_MSC_VER)
#pragma warning(disable: 4244 4267)
#endif

#if defined(WHISPER_CUDA)
#include <cuda_runtime.h>
#endif

using namespace httplib;
using json = nlohmann::ordered_json;

namespace {

  const std::string json_format = "json";
  const std::string text_format = "text";
  const std::string srt_format = "srt";
  const std::string vjson_format = "verbose_json";
  const std::string vtt_format = "vtt";

  struct server_params {
    std::string hostname = "127.0.0.1";
    std::string public_path = "examples/server/public";
    std::string request_path = "";
    std::string inference_path = "/inference";
    int32_t port = 8080;
    int32_t read_timeout = 600;
    int32_t write_timeout = 600;
    bool ffmpeg_converter = false;
  };

  struct whisper_params {
    int32_t n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t n_processors = 1;
    int32_t offset_t_ms = 0;
    int32_t offset_n = 0;
    int32_t duration_ms = 0;
    int32_t max_context = -1;
    int32_t max_len = 0;
    int32_t best_of = 2;
    int32_t beam_size = -1;
    int32_t audio_ctx = 0;
    float word_thold = 0.01f;
    float entropy_thold = 2.40f;
    float logprob_thold = -1.00f;
    float temperature = 0.00f;
    float temperature_inc = 0.20f;
    bool translate = false;
    bool detect_language = false;
    bool diarize = false;
    bool tinydiarize = false;
    bool split_on_word = false;
    bool no_timestamps = false;
    bool use_gpu = true;
    bool flash_attn = false;
    std::string language = "en";
    std::string prompt = "";
    std::string model = "models/ggml-base.en.bin";
    std::string response_format = json_format;
    std::string openvino_encode_device = "CPU";
    std::string dtw = "";
  };

  size_t get_available_gpu_memory() {
    #if defined(WHISPER_CUDA)
    size_t free_memory, total_memory;
    cudaMemGetInfo( & free_memory, & total_memory);
    return free_memory;
    #elif defined(__APPLE__)
    // Fixed values based on typical Apple devices
    // You can adjust these based on your target devices
    uint64_t base_memory = 4ULL * 1024 * 1024 * 1024; // 4GB base
    return base_memory;
    #else
    return SIZE_MAX;
    #endif
  }

  int calculate_num_instances(const whisper_params & params) {
    #if defined(WHISPER_CUDA)
    size_t available_memory = get_available_gpu_memory();
    size_t model_memory_usage = 1024 * 1024 * 1024; // 1GB per instance
    return std::max(1, static_cast < int > (available_memory / model_memory_usage));
    #elif defined(__APPLE__)
    // For Apple devices, use a fixed number based on device capability
    // You can make this more sophisticated based on device detection
    return 4; // Conservative default for Apple devices
    #else
    return std::max(1, static_cast < int > (std::thread::hardware_concurrency() / 2));
    #endif
  }

  struct whisper_instance {
      int id; // Unique identifier for the instance
      std::unique_ptr<whisper_context, decltype(&whisper_free)> ctx;
      std::atomic<bool> in_use;

      whisper_instance(int id, whisper_context* c)
          : id(id), ctx(c, whisper_free), in_use(false) {}
  };


  class whisper_pool {
  public:
      whisper_pool(const whisper_params& params, int num_instances) {
          printf("Initializing whisper pool with %d instances\n", num_instances);

          for (int i = 0; i < num_instances; ++i) {
              auto instance = init_whisper(params, i); // Pass the instance ID
              if (instance) {
                  instances.emplace_back(std::move(instance));
              } else {
                  fprintf(stderr, "Failed to initialize instance %d\n", i);
              }
          }

          printf("Successfully initialized %zu instances\n", instances.size());
      }

      std::shared_ptr<whisper_instance> get_instance() {
          std::unique_lock<std::mutex> lock(mutex);
          condition.wait(lock, [this] {
              return std::any_of(instances.begin(), instances.end(),
                                 [](const std::shared_ptr<whisper_instance>& inst) { return !inst->in_use.load(); });
          });

          for (auto& inst : instances) {
              if (!inst->in_use.exchange(true)) {
                  printf("Instance %d acquired\n", inst->id); // Log acquisition
                  return inst;
              }
          }

          return nullptr;
      }


      void release_instance(std::shared_ptr<whisper_instance> instance) {
          printf("Instance %d released\n", instance->id); // Log release
          instance->in_use.store(false);
          condition.notify_one();
      }

    private: std::vector < std::shared_ptr < whisper_instance >> instances;
    std::mutex mutex;
    std::condition_variable condition;

    std::shared_ptr < whisper_instance > init_whisper(const whisper_params & params, int id) {
      whisper_context_params cparams = whisper_context_default_params();
      #if defined(WHISPER_CUDA)
      cparams.use_gpu = true;
      #elif defined(__APPLE__)
      cparams.use_gpu = true;
      // Metal-specific parameters could be set here if whisper.cpp adds them
      #else
      cparams.use_gpu = false;
      #endif
      cparams.flash_attn = params.flash_attn;

      whisper_context * ctx = whisper_init_from_file_with_params(params.model.c_str(), cparams);
      if (ctx == nullptr) {
        fprintf(stderr, "Error: Failed to initialize whisper context for instance %d\n", id);
        return nullptr;
      }

      //whisper_ctx_init_openvino_encoder(ctx, nullptr, params.openvino_encode_device.c_str(), nullptr);

      return std::make_shared<whisper_instance>(id, ctx);
    }
  };

  struct server_task {
    int id;
    whisper_params params;
    std::vector < float > pcmf32;
    std::vector < std::vector < float >> pcmf32s;
    std::promise < std::string > result_promise;
  };

  class server_queue {
    public: void post(std::shared_ptr < server_task > task) {
      std::lock_guard < std::mutex > lock(mutex);
      tasks.push(task);
      condition.notify_one();
    }

    std::shared_ptr<server_task> receive() {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [this] {
            return !tasks.empty();
        });
        auto task = tasks.front();
        tasks.pop();
        return task;
    }

    bool empty() {
      std::lock_guard < std::mutex > lock(mutex);
      return tasks.empty();
    }

    private: std::queue < std::shared_ptr < server_task >> tasks;
    std::mutex mutex;
    std::condition_variable condition;
  };

  class MyThreadPool {
    public: MyThreadPool(size_t num_threads): stop(false) {
      for (size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back([this] {
          while (true) {
            std:: function < void() > task;
            {
              std::unique_lock < std::mutex > lock(queue_mutex);
              condition.wait(lock, [this] {
                return stop || !tasks.empty();
              });
              if (stop && tasks.empty()) return;
              task = std::move(tasks.front());
              tasks.pop();
            }
            task();
          }
        });
      }
    }

    template < class F >
    void enqueue(F && f) {
      {
        std::unique_lock < std::mutex > lock(queue_mutex);
        tasks.emplace(std::forward < F > (f));
      }
      condition.notify_one();
    }

    ~MyThreadPool() {
      {
        std::unique_lock < std::mutex > lock(queue_mutex);
        stop = true;
      }
      condition.notify_all();
      for (std::thread & worker: workers) {
        worker.join();
      }
    }

    private: std::vector < std::thread > workers;
    std::queue < std:: function < void() >> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
  };

  std::atomic < int > server_state(0);
  std::atomic < bool > running(true);

  void signal_handler(int signal) {
    running = false;
  }

  bool parse_params(int argc, char ** argv, whisper_params & params, server_params & sparams) {
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-h" || arg == "--help") {
        return false;
      } else if (arg == "-t" || arg == "--threads") {
        params.n_threads = std::stoi(argv[++i]);
      } else if (arg == "-p" || arg == "--processors") {
        params.n_processors = std::stoi(argv[++i]);
      } else if (arg == "-ot" || arg == "--offset-t") {
        params.offset_t_ms = std::stoi(argv[++i]);
      } else if (arg == "-on" || arg == "--offset-n") {
        params.offset_n = std::stoi(argv[++i]);
      } else if (arg == "-d" || arg == "--duration") {
        params.duration_ms = std::stoi(argv[++i]);
      } else if (arg == "-mc" || arg == "--max-context") {
        params.max_context = std::stoi(argv[++i]);
      } else if (arg == "-ml" || arg == "--max-len") {
        params.max_len = std::stoi(argv[++i]);
      } else if (arg == "-bo" || arg == "--best-of") {
        params.best_of = std::stoi(argv[++i]);
      } else if (arg == "-bs" || arg == "--beam-size") {
        params.beam_size = std::stoi(argv[++i]);
      } else if (arg == "-ac" || arg == "--audio-ctx") {
        params.audio_ctx = std::stoi(argv[++i]);
      } else if (arg == "-wt" || arg == "--word-thold") {
        params.word_thold = std::stof(argv[++i]);
      } else if (arg == "-et" || arg == "--entropy-thold") {
        params.entropy_thold = std::stof(argv[++i]);
      } else if (arg == "-lpt" || arg == "--logprob-thold") {
        params.logprob_thold = std::stof(argv[++i]);
      } else if (arg == "-tr" || arg == "--translate") {
        params.translate = true;
      } else if (arg == "-di" || arg == "--diarize") {
        params.diarize = true;
      } else if (arg == "-tdrz" || arg == "--tinydiarize") {
        params.tinydiarize = true;
      } else if (arg == "-sow" || arg == "--split-on-word") {
        params.split_on_word = true;
      } else if (arg == "-l" || arg == "--language") {
        params.language = argv[++i];
      } else if (arg == "-m" || arg == "--model") {
        params.model = argv[++i];
      } else if (arg == "-fa" || arg == "--flash-attn") {
        params.flash_attn = true;
      } else if (arg == "--host") {
        sparams.hostname = argv[++i];
      } else if (arg == "--port") {
        sparams.port = std::stoi(argv[++i]);
      } else if (arg == "--path") {
        sparams.public_path = argv[++i];
      }
    }
    return true;
  }

  std::string process_audio(whisper_context * ctx,
    const whisper_params & params,
      const std::vector < float > & pcmf32,
        const std::vector < std::vector < float >> & pcmf32s) {
    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.print_realtime = false;
    wparams.print_progress = false;
    wparams.print_timestamps = !params.no_timestamps;
    wparams.translate = params.translate;
    wparams.language = params.language.c_str();
    wparams.n_threads = params.n_threads;
    wparams.offset_ms = params.offset_t_ms;
    wparams.duration_ms = params.duration_ms;

    if (whisper_full_parallel(ctx, wparams, pcmf32.data(), pcmf32.size(), params.n_processors) != 0) {
      return "{\"error\":\"failed to process audio\"}";
    }

    if (params.response_format == json_format) {
      json result = {
        {
          "text",
          whisper_full_get_segment_text(ctx, 0)
        }
      };
      return result.dump();
    } else {
      return whisper_full_get_segment_text(ctx, 0);
    }
  }

} // namespace

int main(int argc, char ** argv) {
  whisper_params params;
  server_params sparams;

  if (!parse_params(argc, argv, params, sparams)) {
    return 1;
  }

  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  int num_instances = calculate_num_instances(params);
  std::unique_ptr < whisper_pool > pool = std::make_unique < whisper_pool > (params, num_instances);

  Server svr;
  svr.set_default_headers({
    {
      "Server",
      "whisper.cpp"
    },
    {
      "Access-Control-Allow-Origin",
      "*"
    },
    {
      "Access-Control-Allow-Headers",
      "content-type, authorization"
    }
  });

  MyThreadPool thread_pool(std::max(1, static_cast < int > (std::thread::hardware_concurrency()) - 1));
  server_queue task_queue;

  std::vector<std::thread> worker_threads;
     int num_worker_threads = num_instances; // or adjust as needed
     for (int i = 0; i < num_worker_threads; ++i) {
         worker_threads.emplace_back([&]() {
             while (running) {
                 auto task = task_queue.receive();
                 auto instance = pool->get_instance();
                 if (instance) {
                     printf("Processing task %d using instance %d\n", task->id, instance->id);
                     std::string result = process_audio(instance->ctx.get(), task->params, task->pcmf32, task->pcmf32s);
                     task->result_promise.set_value(result);
                     pool->release_instance(instance);
                 } else {
                     task->result_promise.set_value("{\"error\":\"no available instances\"}");
                 }
             }
         });
     }


  svr.Post(sparams.request_path + sparams.inference_path, [&](const Request& req, Response& res) {
      try {
          auto task = std::make_shared<server_task>();
          task->id = rand();
          task->params = params;

          if (!req.has_file("file")) {
              res.set_content("{\"error\":\"no file\"}", "application/json");
              return;
          }

          auto audio_file = req.get_file_value("file");
          if (!::read_wav(audio_file.content, task->pcmf32, task->pcmf32s, params.diarize)) {
              res.set_content("{\"error\":\"failed to read audio\"}", "application/json");
              return;
          }

          std::shared_future<std::string> future_result = task->result_promise.get_future().share();
          task_queue.post(task);

          // Set up the content provider for asynchronous response
          res.set_chunked_content_provider("application/json",
              [future_result](size_t offset, httplib::DataSink& sink) mutable {
                  try {
                      // Wait for the result
                      std::string result = future_result.get();
                      sink.write(result.data(), result.size());
                      sink.done();
                  } catch (const std::exception& e) {
                      // Handle exceptions and send error response
                      std::string error = "{\"error\":\"" + std::string(e.what()) + "\"}";
                      sink.write(error.data(), error.size());
                      sink.done();
                  }
                  return true;
              });
      } catch (const std::exception& e) {
          res.set_content("{\"error\":\"internal server error\"}", "application/json");
      }
  });


  svr.Post(sparams.request_path + "/load", [ & ](const Request & req, Response & res) {
    thread_pool.enqueue([ & , req]() {
      if (!req.has_file("model")) {
        res.set_content("{\"error\":\"no model file\"}", "application/json");
        return;
      }

      auto model_file = req.get_file_value("model");
      params.model = model_file.filename;

      // Reinitialize the whisper pool with the new model
      int new_num_instances = calculate_num_instances(params);
      pool = std::make_unique < whisper_pool > (params, new_num_instances);

      res.set_content("{\"status\":\"model loaded\", \"instances\":" + std::to_string(new_num_instances) + "}", "application/json");
    });
  });

  svr.set_exception_handler([](const Request & , Response & res, std::exception_ptr ep) {
    try {
      std::rethrow_exception(ep);
    } catch (std::exception & e) {
      res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
    }
  });

  svr.set_error_handler([](const Request & req, Response & res) {
    res.set_content("{\"error\":\"invalid request\"}", "application/json");
  });

  svr.set_read_timeout(sparams.read_timeout);
  svr.set_write_timeout(sparams.write_timeout);

  if (!svr.bind_to_port(sparams.hostname, sparams.port)) {
    fprintf(stderr, "couldn't bind to server socket: hostname=%s port=%d\n",
      sparams.hostname.c_str(), sparams.port);
    return 1;
  }

  svr.set_base_dir(sparams.public_path);

  printf("whisper server listening at http://%s:%d with %d model instances (%s)\n",
    sparams.hostname.c_str(), sparams.port, num_instances,
    #if defined(WHISPER_CUDA)
    "CUDA enabled"
    #elif defined(__APPLE__)
    "Metal enabled"
    #else "CPU only"
    #endif
  );

  std::thread server_thread([ & svr]() {
    svr.listen_after_bind();
  });

  while (running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  svr.stop();
  server_thread.join();
  // Join all worker threads
     for (auto& worker : worker_threads) {
         worker.join();
     }

  return 0;
}
