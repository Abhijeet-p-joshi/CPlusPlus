#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

// ----------------------------
// Domain object
// ----------------------------
struct Task {
    int id{};
    std::string payload{};
};

// ----------------------------
// Thread-safe bounded queue
// ----------------------------
class TaskQueue {
public:
    explicit TaskQueue(std::size_t capacity)
        : m_capacity(capacity) {}

    // Push blocks if queue is full
    void push(Task task) {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_notFull.wait(lock, [this] {
            return m_queue.size() < m_capacity || m_shutdown;
        });

        if (m_shutdown) {
            return;
        }

        m_queue.push(std::move(task));
        lock.unlock();
        m_notEmpty.notify_one();
    }

    // Pop blocks if queue is empty
    bool pop(Task& task) {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_notEmpty.wait(lock, [this] {
            return !m_queue.empty() || m_shutdown;
        });

        if (m_queue.empty()) {
            return false; // shutdown or no work left
        }

        task = std::move(m_queue.front());
        m_queue.pop();

        lock.unlock();
        m_notFull.notify_one();
        return true;
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_shutdown = true;
        }
        m_notEmpty.notify_all();
        m_notFull.notify_all();
    }

private:
    std::queue<Task> m_queue;
    std::size_t m_capacity{};
    bool m_shutdown{false};

    std::mutex m_mutex;
    std::condition_variable m_notEmpty;
    std::condition_variable m_notFull;
};

// ----------------------------
// Simulated producer
// ----------------------------
void producer(TaskQueue& queue,
              int producerId,
              int taskCount,
              std::atomic<int>& producedCount)
{
    for (int i = 0; i < taskCount; ++i) {
        Task task;
        task.id = producerId * 1000 + i;
        task.payload = "Task from producer " + std::to_string(producerId);
        // Task task{
        //     .id = producerId * 1000 + i,
        //     .payload = "Task from producer " + std::to_string(producerId)
        // };

        queue.push(std::move(task));
        ++producedCount;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// ----------------------------
// Simulated consumer
// ----------------------------
void consumer(TaskQueue& queue,
              int consumerId,
              std::atomic<int>& consumedCount,
              std::atomic<bool>& stopRequested)
{
    while (!stopRequested.load()) {
        Task task;
        if (!queue.pop(task)) {
            // queue shutdown or empty after shutdown
            break;
        }

        // Simulated processing
        std::cout << "[Consumer " << consumerId << "] Processing task "
                  << task.id << " | " << task.payload << '\n';

        ++consumedCount;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// ----------------------------
// Main
// ----------------------------
int main() {
    constexpr int producerCount = 2;
    constexpr int consumerCount = 3;
    constexpr int tasksPerProducer = 5;

    TaskQueue queue(5); // bounded queue

    std::atomic<int> producedCount{0};
    std::atomic<int> consumedCount{0};
    std::atomic<bool> stopRequested{false};

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    // Start consumers
    for (int i = 0; i < consumerCount; ++i) {
        consumers.emplace_back(
            consumer,
            std::ref(queue),
            i,
            std::ref(consumedCount),
            std::ref(stopRequested)
        );
    }

    // Start producers
    for (int i = 0; i < producerCount; ++i) {
        producers.emplace_back(
            producer,
            std::ref(queue),
            i,
            tasksPerProducer,
            std::ref(producedCount)
        );
    }

    // Join producers first
    for (auto& t : producers) {
        if (t.joinable()) {
            t.join();
        }
    }

    // All tasks produced, signal shutdown
    stopRequested.store(true);
    queue.shutdown();

    // Join consumers
    for (auto& t : consumers) {
        if (t.joinable()) {
            t.join();
        }
    }

    std::cout << "\n=== Summary ===\n";
    std::cout << "Produced: " << producedCount.load() << '\n';
    std::cout << "Consumed: " << consumedCount.load() << '\n';

    return 0;
}