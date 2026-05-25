#pragma once

#include <vector>
#include <thread>
#include <future>
#include <atomic>
#include <memory>

#include "SPSC.h"
#include "Task.h"

class ThreadPool {
public:
    ThreadPool(size_t num_threads);
    ~ThreadPool();

    [[nodiscard]] std::future<Result> enqueue_chunk(const size_t chunk_id, const JobType type,
                                                    const size_t original_size, std::vector<uint8_t>&& data);

private:
    void worker_loop(const size_t worker_id);

    std::vector<std::thread> _workers;
    std::vector<std::unique_ptr<SPSC<Task>>> _queues; 
    
    std::atomic<bool> _is_running;
    size_t _next_queue_idx = 0;
};
