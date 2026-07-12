#include<iostream>
#include<thread>
#include<mutex>
#include<queue>
#include<condition_variable>
#include<string>
#include <vector>
class ThreadPool {
    public:
    ThreadPool(int numThreads):done(false) {
        for(int i=0;i<numThreads;i++){
            threads.emplace_back(([this]{
                while(true){
                    std::unique_lock<std::mutex> lock(mtx);
                    cv.wait(lock, [this]{ return !tasks.empty() || done; });
                    if(done && tasks.empty()){
                        break;
                    }
                    auto task = tasks.front();//等效std::function<void()>task(std::move(tasks.front());//取任务
                    tasks.pop();
                    lock.unlock(); // 释放锁，允许其他线程访问队列
                    std::cout << "消费: " << task << std::endl;
                }
            }));
        }
        
    }
    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            done = true;
        }
        cv.notify_all(); // 通知所有线程工作，完成任务队列
        for(auto& thread : threads){
            if(thread.joinable()){
                thread.join();
            }
        }
    }
    void addTask(const std::string& task) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            tasks.push(task);
            std::cout << "生产: " << task << std::endl;
        }
        cv.notify_one(); // 通知一个线程有新任务
    }
    private:
        std::vector<std::thread> threads;       // 线程池
        std::queue<std::string> tasks;       // 任务队列
        std::condition_variable cv;          // 条件变量：通知"有活了"
        std::mutex mtx;                      // 保护队列 + 条件变量
        bool done = false;                   // 生产结束标志（消费者据此退出）

};