#ifndef ThreadPool_h
#define ThreadPool_h

#include <string>
#include <functional>
#include <queue>
#include <vector>

#include <thread>

#include <mutex>
#include <atomic>
#include <condition_variable>

#include <AtlasHTTP/Server/request.hpp>

namespace HTTP {

class ThreadPool {
    private:
        std::condition_variable cv;
        std::queue<std::function<void()>> thread_queue;
        std::vector<std::thread> workers;
        std::mutex queue_mtx;
    public:
        void enqueue(std::function<void()> task);
        void worker();
        void init(int num_threads);
};

}
#endif