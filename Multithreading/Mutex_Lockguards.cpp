// 1. Thread Fundamentals
// 2. Data Race & Undefined Behavior
// 3. Mutex & Locking Mechanisms

// 1. THREAD FUNDAMENTALS
// A thread is an independent execution path inside the same process.
// All threads:
// - Share memory (heap, globals)
// - Have their own stack

// Example: Multi-threaded sensor processing (ADAS-style thinking)

// Let’s simulate 3 sensors:
// - Camera
// - Radar
// - Lidar
// Each runs independently.

#include <thread>
#include <iostream>

// void cameraTask() {
//     for (int i=1; i <=5; i++) {
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//         std::cout << "[Camera] Frame " << i << std::endl;
//     }
// }

// void radarTask() {
//     for (int i=1; i<=5; i++) {
//         std::this_thread::sleep_for(std::chrono::milliseconds(150));
//         std::cout << "[Radar] Frame " << i << std::endl;
//     }
// }

// void lidarTask() {
//     for (int i = 1; i <= 5; ++i) {
//         std::this_thread::sleep_for(std::chrono::milliseconds(200));
//         std::cout << "[Lidar] Sweep " << i << std::endl;
//     }
// }

// int main() {

//     std::thread t1(cameraTask);
//     std::thread t2(radarTask);
//     std::thread t3(lidarTask);

//     t1.join();
//     t2.join();
//     t3.join();

//     std::cout << "All sensors finished tasks." << std::endl;

//     return 0;
// }

// What’s happening
// All three threads run concurrently
// Execution order is non-deterministic
// Output will be interleaved
// Key Takeaways
// std::thread starts execution immediately
// join() = wait for completion
// Threads run independently → no guaranteed order
// Critical Rule - If you don’t join() or detach() → program crashes (std::terminate)


// 2. DATA RACE & UNDEFINED BEHAVIOR
// A data race occurs when:
// Two or more threads access the same memory
// At least one is a write
// No synchronization
// Result: Undefined Behavior (UB)

// #include <thread>
// #include <iostream>

// int counter = 0;

// void increment() {
//     for (int i=0; i<1000000; i++) {
//         ++counter; // Data race on 'counter'
//     }
// }

// int main() {
//     std::thread t1(increment);
//     std::thread t2(increment);

//     t1.join();
//     t2.join();

//     std::cout << "Final Counter: " << counter << std::endl; // UB: May not be 2000000

//     return 0;
// }   

// Expected:Counter = 2000000
// Actual:Counter = 1748392   (or anything random)
// Data race is not just “wrong result” — it’s undefined behavior
// The compiler can legally break your program in unpredictable ways.

// 3. MUTEX & LOCKING MECHANISMS
// A mutex ensures: Only one thread enters a critical section at a time.

// #include <iostream>
// #include <thread>
// #include <mutex>

// int counter = 0;
// std::mutex mtx;

// void increment() { 
//     for(int i=0; i<1000000; i++) {
//         std::lock_guard<std::mutex> lock(mtx); // Lock is acquired here and automatically released at the end of scope
//         ++counter; // Safe access to 'counter'
//     }
// }

// int main() {
//     std::thread t1(increment);
//     std::thread t2(increment);

//     t1.join();
//     t2.join();

//     std::cout << "Final Counter: " << counter << std::endl;
// }

// #include <iostream>
// #include <thread>
// #include <mutex>

// std::mutex msgMutx;

// void sensorMsg(const std::string& source, int count) {
//     for (int i=1; i<=count; ++i) {
//         {
//             std::lock_guard<std::mutex> lock(msgMutx);
//             std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//             std::cout << "[" << source << "] Message " << i << "Thread" << std::this_thread::get_id() << std::endl;
//         }
//     }
// }

// int main() {
//     std::thread t1(sensorMsg, "Camera", 5);
//     std::thread t2(sensorMsg, "Radar", 5);
//     std::thread t3(sensorMsg, "Lidar", 5);

//     t1.join();
//     t2.join();
//     t3.join();

//     return 0;   
// }

/* ---------------------------------------------------- */
// Goal:
// Observe:
// Unsafe shared modification
// Then fix it with mutex
// This is much more important than Task 1 because now we are dealing with real shared data corruption.

// Without MUTEX
// #include<vector>
// std::vector<int> sharedData;

// void pushValues(int value) {
//     for (int i=1;i <1000; ++i) {
//         sharedData.push_back(value + i);
//     }
// }

// int main()
// {
//     std::thread t1(pushValues, 1000);
//     std::thread t2(pushValues, 2000);

//     t1.join();
//     t2.join();

//     std::cout << "Final vector size = " << sharedData.size() << "\n";
//     return 0;
// }

// std::vector::push_back() is not thread-safe.
// Internally it may:
// - write new element
// - increase size
// - reallocate memory when capacity is full
// - move existing elements

// If two threads do that simultaneously:
// internal vector state gets corrupted
// This is real-world dangerous.

// WITH MUTEX
// #include <mutex>
// #include <vector>

// std::vector<int> sharedData;
// std::mutex dataMutex;

// void pushValues(int startValue) {
//     for (int i=0; i<10000; ++i) {
//         std::lock_guard<std::mutex> lg(dataMutex); // Lock is acquired here and automatically released at the end of scope
//         sharedData.push_back(startValue+i);
//     }
// }

// int main() {
//     std::thread t1(pushValues, 1000);
//     std::thread t2(pushValues, 3000);

//     t1.join();
//     t2.join();

//     std::cout << " Final vector size = " << sharedData.size() << "\n";

//     for (size_t i=0; i<10 && i<sharedData.size(); ++i) {
//         std::cout << sharedData[i] << " ";
//     }
//     std::cout << "\n";

//     return 0;
// }

// This works, but it is not efficient.
// Why?
// Because this:
// std::lock_guard<std::mutex> lock(dataMutex);
// sharedData.push_back(...);
// locks 10,000 times per thread.

// That creates:
// - lock contention
// - unnecessary synchronization overhead

// BETTER APPROACH WITH MUTEX
#include <mutex>
#include <vector>

std::vector<int> sharedData;
std::mutex dataMutex;

void pushValues(int startValue) {
    std::vector<int> localData;
    localData.reserve(10000);

    for (int i=0; i<10000; ++i) {
        localData.push_back(startValue + i);
    }

    std::lock_guard<std::mutex> lg(dataMutex);
    sharedData.insert(sharedData.end(), localData.begin(), localData.end());
}

int main() {
    std::thread t1(pushValues, 1000);
    std::thread t2(pushValues, 3000);

    t1.join();
    t2.join();

    for (size_t i=0; i<10 && i<sharedData.size(); ++i) {
        std::cout << sharedData[i] << " ";
    }
    std::cout << "\n";

    std::cout << " Final vector size = " << sharedData.size() << "\n";

    return 0;
}

// Old version: 10,000 locks per thread
// Better version: 1 lock per thread

/*--------------------------------------------------*/

