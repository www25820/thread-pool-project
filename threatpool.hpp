#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <string>
#include <vector>
#include <functional>   // std::function — 万能函数容器
#include <future>       // std::future, std::packaged_task
#include <type_traits>  // std::invoke_result_t — 推导返回值类型

class ThreadPool {
public:
    // ===== 构造函数：创建 numThreads 个工作线程 =====
    ThreadPool(int numThreads) : done(false) {
        for (int i = 0; i < numThreads; i++) {
            threads.emplace_back([this] {
                while (true) {
                    std::unique_lock<std::mutex> lock(mtx);

                    // 条件变量 — 没任务且没关闭就睡觉
                    cv.wait(lock, [this] { return !tasks.empty() || done; });

                    // 关闭 + 队列空 → 线程退出
                    if (done && tasks.empty()) {
                        break;
                    }

                    // 取出一个任务（move 避免拷贝 std::function）
                    auto task = std::move(tasks.front());
                    tasks.pop();

                    // 放锁，在锁外执行 — 避免阻塞其他线程取任务
                    lock.unlock();

                    task();
                }
            });
        }
    }

    // ===== 析构函数：优雅关闭 =====
    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            done = true;               // 通知：该收工了
        }
        cv.notify_all();                // 叫醒所有睡着的线程

        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();               // 等每个线程完全退出
            }
        }
    }

    // ===== 提交任务（泛型，返回 future 拿结果） =====
    // enqueue(函数, 参数...) → future<返回值>
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>//声明返回类型
    {
        // 推导返回值类型 — F=lambda int(int), Args=int → ReturnType = int
        using ReturnType = std::invoke_result_t<F, Args...>;

        // 逐层打包 "用户的函数 + 参数" 变成一个 void() 任务
        //
        // 第 1 层: std::bind(函数, 参数...)
        //   → 把函数和参数捆在一起，变成无参可调用对象
        //
        // 第 2 层: std::packaged_task<ReturnType()>
        //   → 把 bind 结果包起来，执行完后自动把返回值写入 future
        //     （promise 需要手动 set_value，packaged_task 自动）
        //
        // 第 3 层: std::make_shared<...>
        //   → packaged_task 不可拷贝，用 shared_ptr 包住，指针可以拷贝
        //     这样才能被 lambda 捕获，安全地在多个线程间传递
        auto ptask = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        // 拿到 future — 这是返回给调用方的"取结果凭证"
        std::future<ReturnType> result = ptask->get_future();

        // 第 4 层: [ptask] { (*ptask)(); }
        //   → 把 shared_ptr 捕获进 lambda，lambda 类型是 void()
        //   → 工作线程调 task() 时，lambda 执行 (*ptask)()，结果自动存入 future
        {
            std::lock_guard<std::mutex> lock(mtx);
            tasks.emplace([ptask] { (*ptask)(); });
        }

        // 叫醒一个工作线程来干活
        cv.notify_one();

        return result;
    }

private:
    std::vector<std::thread> threads;               // 工作线程数组
    std::queue<std::function<void()>> tasks;        // 任务队列
    std::mutex mtx;                                 // 保护队列 + done
    std::condition_variable cv;                     // 条件变量
    bool done = false;                              // 关闭标志
};
