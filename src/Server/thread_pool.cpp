#include <AtlasHTTP/Server/thread_pool.hpp>

void HTTP::ThreadPool::enqueue(std::function<void()> task){
    {
        std::lock_guard<std::mutex> lock(queue_mtx);
        thread_queue.push(task);
    }
    cv.notify_one();
    
    //signal logic here send_signal_thread(thread_id)
}

void HTTP::ThreadPool::worker(){
    while(true){
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mtx);

            cv.wait(lock,[this](){
                return !thread_queue.empty();
            });

            task = thread_queue.front();
            thread_queue.pop();
        }
        task();
        //HTTP::Server::handle_client_connection(client_socket);
    }
}

void HTTP::ThreadPool::init(int num_threads){
    for (int i = 0;i<num_threads;i++){
        workers.emplace_back([this]() {
            worker();
        });
    }
}

// design -> thread once done with its job increases available_threads -> major challenge how i would distribute the request -> every thread needs a flag is_available which would mark whether it is available to take request if it is 

// or better approach would be to just let a while loop run until a flag (is_vacant) is true if its false it should break out of loop and then fetch that request and again go back into the loop 