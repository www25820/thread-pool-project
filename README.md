# ThreadPool

基于 C++17 的轻量级线程池实现，支持任务提交、异步返回和优雅关闭。

## 功能

- **生产者-消费者模型**：工作线程阻塞等待任务，有任务时自动唤醒执行
- **条件变量同步**：`std::condition_variable` 实现线程间等待/通知，任务队列为空时工作线程休眠，不空转
- **线程安全**：`std::mutex` + `std::lock_guard` 保护共享任务队列，避免数据竞争
- **细粒度锁**：锁仅保护队列操作，任务执行在锁外进行，减少锁竞争
- **优雅退出**：支持关闭信号，工作线程消费完剩余任务后安全退出
- **虚假唤醒防护**：`wait` 第二参数（条件谓词）确保仅在条件满足时唤醒

## 环境要求

- 操作系统：Windows / Linux
- 编译器：MinGW-w64 (g++ 8.0+) 或 MSVC 2017+
- 标准：C++17 或更高

## 编译

```bash
g++ -std=c++17 -pthread "threat pool_project.cpp" -o thread_pool
```

VS Code 下配置 `.vscode/tasks.json` 指定编译器路径后按 `Ctrl+Shift+B` 构建。

## 运行

```bash
./thread_pool
```

## 项目结构

```text
threat pool_project/
├── threat pool_project.cpp     # 生产者-消费者原型（条件变量实战）
├── ThreadPool.hpp              # 线程池核心实现（规划中）
├── .vscode/                    # VS Code 调试/编译配置
├── .gitignore
└── README.md
```

## 进度

- [x] `std::condition_variable` 用法（`wait` / `notify_one` / 条件谓词）
- [x] 生产者-消费者基本模型（1 生产者 + 1 消费者）
- [x] `std::unique_lock` 与 `std::lock_guard` 选型与锁粒度控制
- [x] Lambda 表达式与条件谓词
- [x] 虚假唤醒原理与防护
- [ ] `std::function` + `std::packaged_task` + `std::future` 任务封装
- [ ] ThreadPool 类：线程数组 + 任务队列 + `submit()` 接口
- [ ] `submit()` 返回 `std::future`，支持异步获取结果
- [ ] 多生产者/多消费者场景压力测试
- [ ] 性能对比（裸线程 vs 线程池）

## 要点

| 概念 | 说明 |
| :--- | :--- |
| `cv.wait(lock, pred)` | `pred` 为 `false` 时原子地释放锁并阻塞；被 `notify` 后重新获取锁并检查 `pred` |
| `cv.notify_one()` vs `notify_all()` | 前者唤醒一个等待线程，避免惊群；后者唤醒全部 |
| 锁粒度 | 仅对共享数据的读写加锁，业务逻辑在锁外执行，否则退化为串行 |
| 虚假唤醒 | 操作系统可能无故唤醒阻塞线程，条件谓词是标准防护手段 |
| `unique_lock` vs `lock_guard` | 前者可手动 `unlock()`/`lock()`，配合条件变量必须使用前者；后者仅 RAII 自动管理 |
| 原子唤醒语义 | `wait` 内部「释放锁 → 进入等待队列」是原子的，保证 `notify` 不丢失 |

## 参考资料

- [cppreference: std::condition_variable](https://en.cppreference.com/w/cpp/thread/condition_variable)
- [C++ Concurrency in Action (2nd Edition)](https://www.manning.com/books/c-plus-plus-concurrency-in-action-second-edition)
