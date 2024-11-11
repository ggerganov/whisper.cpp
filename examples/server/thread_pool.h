#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

class CustomThreadPool {
public:
    CustomThreadPool(size_t num_threads) : stop(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] {
                            return stop || !tasks.empty();
                        });
                        if (stop && tasks.empty()) {
                            return;
                        }
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop) {
                throw std::runtime_error("enqueue on stopped CustomThreadPool");
            }
            tasks.emplace([task](){ (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    void shutdown() {
        // First prevent any new tasks from being enqueued
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        
        // Wake up all worker threads
        condition.notify_all();
        
        // Wait for all tasks to complete and threads to exit
        for (std::thread &worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }

        // Clear any remaining tasks
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            while (!tasks.empty()) {
                tasks.pop();
            }
        }
    }

    ~CustomThreadPool() {
        if (!stop) {
            shutdown();
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
}; 