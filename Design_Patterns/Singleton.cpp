#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>
#include <queue>

struct Frame { 
    int id;
};

enum class DriverState {
    ALERT,
    DROWSY,
    DISTRACTED
};

class DmsManager { 
    public:
        static DmsManager& getInstance() {
            static DmsManager  dmsinstance;
            return dmsinstance;
        }

        DmsManager(const DmsManager&) = delete;
        DmsManager& operator=(const DmsManager&) = delete;

        void start() {
            running_ = true;
            workerThread_ = std::thread([this]() {
                process();
            });
        }

        void stop() {
            running_ = false;
            if(workerThread_.joinable()) {
                workerThread_.join();
            }
        }

        DriverState getDriverState() const { 
            return driverState_;
        }

        void pushFrame(const Frame& frame) {
            std::lock_guard<std::mutex> lock(mutex_);
            frameQueue_.push(frame);
        }

    private:
        DmsManager() : running_(false), driverState_(DriverState::ALERT) {}

        ~DmsManager() { stop(); }

        void process() {
            while (running_) {
                Frame frame;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    if (!frameQueue_.empty()) {
                        frame = frameQueue_.front();
                        frameQueue_.pop();
                    }
                    else {
                        continue;
                    }
                }

                // Simulate processing
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                // Simple logic: even frame → ALERT, odd → DROWSY
                if (frame.id % 2 == 0) {
                    driverState_ = DriverState::ALERT;
                } else {
                    driverState_ = DriverState::DROWSY;
                }

                std::cout << "Processed Frame " << frame.id
                        << " | Driver State: "
                        << (driverState_ == DriverState::ALERT ? "ALERT" : "DROWSY")
                        << std::endl;
            }
        }

        std::atomic<bool> running_;
        std::atomic<DriverState> driverState_;
        std::mutex mutex_;
        std::thread workerThread_;
        std::queue<Frame> frameQueue_;
};

int main() {
    auto& dms = DmsManager::getInstance();

    dms.start();

    std::vector<std::thread> producers;

    for (int i = 0; i < 2; ++i) {
        producers.emplace_back([i]() {
            for (int j = 0; j < 5; ++j) {
                Frame frame{ i * 10 + j };
                DmsManager::getInstance().pushFrame(frame);
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }
        });
    }

    for (auto& prod : producers) {
        prod.join();
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    dms.stop();

    return 0;
}