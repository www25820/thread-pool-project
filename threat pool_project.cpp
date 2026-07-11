/**
 * 线程池前置练习 —— 生产者-消费者（条件变量实战）
 *
 * 一个 Producer 线程生产 10 个任务字符串
 * 一个 Consumer 线程取出任务并打印
 * condition_variable 让消费者"没活就睡，有活就干"
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <string>

std::queue<std::string> tasks;       // 任务队列
std::condition_variable cv;          // 条件变量：通知"有活了"
std::mutex mtx;                      // 保护队列 + 条件变量
bool done = false;                   // 生产结束标志（消费者据此退出）

// ===== 生产者：往里放任务 =====
void Producer() {
    for (int i = 0; i < 10; i++) {
        {
            // 🔒 锁包住「修改共享数据 + 通知」这两个动作
            std::lock_guard<std::mutex> lock(mtx);
            tasks.push("task" + std::to_string(i));
            std::cout << "生产: task" << i << std::endl;
        }  // ← 离开这个 {} 锁自动释放，消费者马上能拿到锁干活
        cv.notify_one();             // 唤醒一个消费者 放锁外也行，放锁内也行
        //                          //   放锁外更高效：避免消费者被叫醒后再等锁释放
        std::this_thread::sleep_for(std::chrono::seconds(1));  // 模拟耗时
    }

    // 通知消费者：生产结束
    {
        std::lock_guard<std::mutex> lock(mtx);
        done = true;
    }
    cv.notify_one();                 // 最后叫醒一次，让它检查 done 标志
}

// ===== 消费者：等着拿任务 =====
void Consumer() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);

        // wait 模式：
        //   condition 返回 true  → 不睡眠，拿着锁直接往下走
        //   condition 返回 false → 释放锁 → 睡眠 → 被 notify 叫醒
        //                           → 重新拿锁 → 再检查一次 condition
        cv.wait(lock, [] { return !tasks.empty() || done; });
        //                         ↑ 有活就干    ↑ 生产者说不干了就退出

        // 👇 走到这里说明：锁已拿到 + 条件满足（要么有任务，要么 done）
        if (done && tasks.empty()) {
            break;                   // 收工
        }

        // 取出一个任务
        std::string task = tasks.front();
        tasks.pop();

        // 🔓 干活前先放锁，避免阻塞生产者（如果以后多个线程的话）
        lock.unlock();

        std::cout << "消费: " << task << std::endl;
        // 这里就是你线程池真正执行任务的地方 ↓
        // task();  ← 将来 task 是一个 std::function<void()>
    }
    std::cout << "消费者退出" << std::endl;
}

int main() {
    std::thread t1(Producer);
    std::thread t2(Consumer);

    t1.join();                       // 等生产者结束
    t2.join();                       // 等消费者结束

    std::cout << "全部完成" << std::endl;
    return 0;
}
