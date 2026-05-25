#include "ThreadPool.h"

#include "LZ77.h"

ThreadPool::ThreadPool(size_t num_threads, uint8_t codec_id) : _is_running(true), _next_queue_idx(0) {
    for (size_t i = 0; i < num_threads; ++i) {
        _queues.emplace_back(std::make_unique<SPSC<Task>>());
        _workers.emplace_back(&ThreadPool::worker_loop, this, i, codec_id);
    }
}

ThreadPool::~ThreadPool() {
    _is_running.store(false, std::memory_order_release);

    for (auto& worker : _workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

std::future<Result> ThreadPool::enqueue_chunk(const size_t chunk_id, const JobType type, 
                                              const size_t original_size, std::vector<uint8_t>&& data) {
    std::promise<Result> promise;
    auto future = promise.get_future();

    Task task { chunk_id, type, original_size, std::move(data), std::move(promise) };

    size_t target_worker = _next_queue_idx;
    _next_queue_idx = (_next_queue_idx + 1) % _queues.size();

    while (!_queues[target_worker]->push(std::move(task))) {
        std::this_thread::yield();
    }

    return future;
}

void ThreadPool::worker_loop(const size_t worker_id, const uint8_t codec_id) {
    CodecVariant local_codec;
    switch (codec_id) {
        case LZ77::get_codec_id():
            local_codec.emplace<LZ77>();
            break;
        case LZ78::get_codec_id():
            local_codec.emplace<LZ78>();
            break;
        default:
            throw std::runtime_error("Unsupported codec ID: " + std::to_string(codec_id));
    }

    SPSC<Task>* my_queue = _queues[worker_id].get();
    Task current_task;

    while (_is_running) {
        if (my_queue->pop(current_task)) {
            Result res;
            res.chunk_id = current_task.chunk_id;

            std::visit([&current_task, &res](auto& codec) {
                if (current_task.type == JobType::Compress) {
                    res.original_size = current_task.input_data.size();
                    res.output_data = codec.compress_block(current_task.input_data);
                } else {
                    res.original_size = current_task.original_size;
                    res.output_data = codec.decompress_block(current_task.input_data, current_task.original_size);
                }
            }, local_codec);
            
            current_task.promise.set_value(std::move(res));
        } else {
            std::this_thread::yield();
        }
    }
}
