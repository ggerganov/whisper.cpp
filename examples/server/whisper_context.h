#pragma once

#include "whisper.h"
#include "whisper_params.h"
#include <memory>
#include <mutex>
#include <atomic>
#include <condition_variable>
using namespace whisper;
// RAII wrapper for whisper_context
class WhisperContext {
public:
    WhisperContext(const std::string& model_path, const whisper_context_params& params) {
        ctx = whisper_init_from_file_with_params(model_path.c_str(), params);
        if (!ctx) {
            throw std::runtime_error("Failed to initialize whisper context");
        }
    }

    ~WhisperContext() {
        if (ctx) {
            whisper_free(ctx);
        }
    }

    whisper_context* get() { return ctx; }

    // Prevent copying
    WhisperContext(const WhisperContext&) = delete;
    WhisperContext& operator=(const WhisperContext&) = delete;

private:
    whisper_context* ctx;
};

// Thread-safe pool of whisper contexts with round-robin allocation
class WhisperContextPool {
public:
    struct Instance {
        whisper_context* ctx;
        whisper::params params;
        whisper::params default_params;
    };

    WhisperContextPool(size_t num_instances, const std::string& model_path, const whisper_context_params& params) {
        if (num_instances == 0) {
            throw std::invalid_argument("Pool must have at least one instance");
        }
        
        contexts.reserve(num_instances);
        for (size_t i = 0; i < num_instances; i++) {
            contexts.push_back(std::unique_ptr<WhisperContext>(new WhisperContext(model_path, params)));
        }
    }

    // Get next available instance
    std::shared_ptr<Instance> get_instance() {
        std::unique_lock<std::mutex> lock(mutex);
        
        // Wait if shutdown is in progress
        cv.wait(lock, [this]() { return !shutting_down; });

        if (is_shutdown) {
            throw std::runtime_error("Context pool has been shut down");
        }
        
        // Get current context and advance to next
        auto instance = std::make_shared<Instance>();
        instance->ctx = contexts[current_idx]->get();
        current_idx = (current_idx + 1) % contexts.size();
        active_operations++;
        
        return instance;
    }

    // Release an instance back to the pool
    void release_instance(std::shared_ptr<Instance>& instance) {
        std::unique_lock<std::mutex> lock(mutex);
        active_operations--;
        if (shutting_down && active_operations == 0) {
            cv.notify_all();
        }
    }

    // Gracefully reset the pool
    void reset() {
        std::unique_lock<std::mutex> lock(mutex);
        shutting_down = true;
        
        // Wait for any ongoing operations to complete
        cv.wait(lock, [this]() { 
            return active_operations == 0; 
        });

        // Clear all contexts
        contexts.clear();
        is_shutdown = true;
        
        cv.notify_all();
    }

private:
    std::vector<std::unique_ptr<WhisperContext>> contexts;
    std::mutex mutex;
    std::condition_variable cv;
    size_t current_idx = 0; // Index for round-robin allocation
    std::atomic<size_t> active_operations{0};
    bool shutting_down = false;
    bool is_shutdown = false;
};