# Multithreading - Basics

## What is a Mutex?
A mutex = mutual exclusion object.

Its purpose is simple:
At most one thread can own the mutex at a time.

If one thread holds the mutex:
other threads trying to lock it must wait


## Why do we need it?
Because multiple threads may access the same shared data:

Eg:
counter++;
sharedVector.push_back(x);
sharedMap[key] = value;

Without coordination, those operations can overlap and corrupt data.
So we put a mutex around the critical section.

## Critical Section
A critical section is: The piece of code that accesses shared mutable state and must not be executed concurrently.

Example:
```c++
++counter;
```
or
```c++
sharedData.push_back(value);
```

## What is a Lock Object?

This is where many juniors get confused.

**Important distinction:**
A mutex is the thing being locked
Example:
std::mutex mtx;

A lock object is the thing that manages the locking
Examples:
std::lock_guard
std::unique_lock
std::scoped_lock

So:

```c++
std::mutex mtx;
std::lock_guard<std::mutex> lock(mtx);
```

means:
*mtx* = resource controlling access
*lock* = RAII wrapper that acquires/releases it

## Why Lock Wrappers Exist

You can do this manually:

```c++
mtx.lock();
// critical section
mtx.unlock();
```

**But this is bad.**

🔹 Why manual locking is dangerous
Problem 1 — Forgetting unlock

```c++
mtx.lock();
doSomething();
// forgot mtx.unlock()
```
Now the mutex stays locked forever → program stalls.

Problem 2 — Exception safety

```c++
mtx.lock();
someFunctionThatThrowsException();
mtx.unlock();
```
If an exception occurs before unlock(), the mutex remains locked.

**That is catastrophic.**

**Solution: RAII**

Use lock wrappers that automatically unlock when leaving scope.

This is RAII: Resource Acquisition Is Initialization

Meaning:
- acquire resource in constructor
- release in destructor

That is why lock wrappers exist.

## Categories of Locking Tools in C++

**CATEGORY A — Mutex Types**
These are the actual synchronization primitives.
1. **std::mutex**: Basic exclusive lock
2. **std::recursive_mutex**: Same thread can lock multiple times (usually avoid)
3. **std::timed_mutex**: Supports timeout-based locking
4. **std::recursive_timed_mutex**: Recursive + timeout

**Mostly use: std::mutex**

**CATEGORY B — Lock Ownership Wrappers**
These are RAII helpers that manage mutex locking.
1. **std::lock_guard**: Simple, strict, lightweight
2. **std::unique_lock**: Flexible, feature-rich
3. **std::scoped_lock**: Best for locking multiple mutexes safely

## std::lock_guard — What, How, Why
**What is it?**
- A small RAII wrapper around a mutex.

When you create it:
```c++
std::lock_guard<std::mutex> lock(mtx);
```
it does this:

- locks mtx immediately
- unlocks automatically when scope ends

**Why use it?**

Because it is:
- simple
- safe
- minimal overhead
- ideal for straightforward critical sections

**How it behaves**

Lock happens here:
```c++
std::lock_guard<std::mutex> lock(mtx);
```
Unlock happens automatically here:
```c++
} // scope ends
```

**When to use it?**

Use lock_guard when:
- you want to lock immediately
- you want to hold the lock until scope ends
- you do not need advanced behavior

This should be the default first choice.

**Limitation**

It cannot:
- unlock early
- relock
- defer locking
- work with condition variables
That’s where unique_lock comes in.

## std::unique_lock — What, How, Why

**What is std::unique_lock?**

std::unique_lock is also an RAII wrapper for mutexes, but with flexible ownership semantics.
That means: It manages the lock, but allows you to control when and how the lock is acquired and released.

It is more powerful than lock_guard.

**Why does it exist?**

Because many real-world synchronization problems need more than:
“lock now, unlock at scope end”

You may need to:
- lock later
- unlock early
- relock again
- transfer ownership
- wait on a condition variable
That is exactly why unique_lock exists.

**What extra features does it provide?**

unique_lock supports:
- immediate locking
- deferred locking
- try-lock
- timed locking
- manual unlock/relock
- movable ownership
- condition variable compatibility

## What is Deferred Locking (std::defer_lock)?

Normally:
```c++
std::unique_lock<std::mutex> lock(mtx);
```
means: lock mtx immediately

But sometimes you want to create the lock object without locking yet.
That is called deferred locking.

You do that with:
```c++
std::unique_lock<std::mutex> lock(mtx, std::defer_lock);
```
It means: “Construct the lock object, but do not acquire the mutex yet.”

Now you have:
- a lock object
- associated with the mutex
- but mutex is not locked yet

Then later you explicitly do:
lock.lock();

**Why would you ever want that?**

Because sometimes locking must happen later, not immediately.

Typical cases:
- prepare things first
- lock multiple mutexes safely
- separate construction from acquisition
- advanced synchronization flows

Example: 
```c++
#include <iostream>
#include <mutex>

std::mutex mtx;

int main()
{
    std::unique_lock<std::mutex> lock(mtx, std::defer_lock);

    std::cout << "Mutex not locked yet\n";
    // Do preparation

    lock.lock();
    std::cout << "Mutex locked now\n";

    lock.unlock();
    std::cout << "Mutex unlocked\n";
}
```

When is deferred locking useful?
Common use cases:
- Manual control
- Condition variable preparation
- Coordinating multiple locks
- Reducing lock duration

**Manual Unlock / Relock — Why important?**

Sometimes you want to:

- protect shared state briefly
- then release lock
- then do expensive work outside lock
- maybe relock later

-> With lock_guard, impossible.
-> With unique_lock, possible.

🔹 Example
```c++
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mtx;

void worker()
{
    std::unique_lock<std::mutex> lock(mtx);
    std::cout << "Protected section\n";

    lock.unlock();

    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "Long work outside lock\n";

    lock.lock();
    std::cout << "Protected section again\n";
}

int main()
{
    std::thread t(worker);
    t.join();
}
```

🔹 Why is this valuable?
Because holding locks during expensive work is bad.

Main thought should be: “Can I shorten the lock lifetime?”
That improves:
- concurrency
- throughput
- responsiveness

**Why std::condition_variable needs std::unique_lock**

This is a common interview question.
🔹 Why not lock_guard?

Because condition_variable::wait() must:
- unlock the mutex while waiting
- sleep
- wake up later
- relock the mutex
That requires a lock object that supports unlock/relock.
lock_guard cannot do that, but unique_lock can.

That is why:
```c++
std::condition_variable cv;
std::unique_lock<std::mutex> lock(mtx);
cv.wait(lock, predicate);
```
works.

And this:
```c++
std::lock_guard<std::mutex> lock(mtx); // WRONG for cv.wait
cv.wait(lock, predicate);
```
does not.

## std::scoped_lock — What, How, Why

**What is std::scoped_lock?**

It is an RAII lock wrapper introduced in C++17.
Main purpose: Safely lock one or more mutexes in a deadlock-safe way.

**Why was it introduced?**

Because locking multiple mutexes manually is dangerous.

Example:
One thread:
```c++
mtx1.lock();
mtx2.lock();
```
If another thread does:
```c++
mtx2.lock();
mtx1.lock();
```
you can **deadlock**.

So C++ introduced std::scoped_lock to make multi-lock acquisition safer and easier.

**How does it work?**

Single mutex:
```c++
std::scoped_lock lock(mtx);
```
works like a nicer lock_guard.

Multiple mutexes:
```c++
std::scoped_lock lock(mtx1, mtx2);
```
This locks both safely using deadlock avoidance internally.

🔹 Why use it?
Use scoped_lock when:
- locking multiple mutexes
- you want simpler syntax
- you want deadlock-safe multi-lock acquisition
🔹 Example
```c++
#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx1;
std::mutex mtx2;

void task()
{
    std::scoped_lock lock(mtx1, mtx2);
    std::cout << "Both mutexes locked safely\n";
}

int main()
{
    std::thread t1(task);
    std::thread t2(task);

    t1.join();
    t2.join();
}
```

## Lock Decision Table (Very Important)

✅ Use std::lock_guard when:
- one mutex
- immediate lock
- no early unlock needed
- no relock needed
- no condition variable
Best for:
- simple shared counter
- simple vector/map protection
- short critical sections

✅ Use std::unique_lock when:
- you need flexibility
- unlock/relock is needed
- deferred lock is needed
- condition_variable is used
- ownership transfer is needed
Best for:
- wait/notify patterns
-staged locking
- advanced thread coordination

✅ Use std::scoped_lock when:
- locking multiple mutexes
- avoiding deadlock
- writing modern clean code
Best for:
- multi-resource synchronization
- object swaps
- cross-structure operations

## Senior-Level Mental Model
Think:

- Step 1:
What shared resource am I protecting?
variable?
queue?
map?
logger?
sensor frame?
- Step 2:
How many mutexes are involved?
one → likely lock_guard or unique_lock
multiple → likely scoped_lock
- Step 3:
Do I need advanced lock control?
no → lock_guard
yes → unique_lock
- Step 4:
Is thread coordination needed?
yes → condition_variable + unique_lock

## Interview-Level Summary

If someone asks:

Q: **Explain the difference between lock_guard, unique_lock, and scoped_lock.**

*std::lock_guard* is a lightweight RAII wrapper for simple scoped locking of a single mutex.
*std::unique_lock* is a more flexible lock wrapper that supports deferred locking, manual unlock/relock, movable ownership, and is required for condition variables.
*std::scoped_lock* is a modern RAII wrapper that can lock one or multiple mutexes safely and is especially useful for deadlock-safe multi-mutex locking.

That is a proper answer.

**Prompt**
**Before we go to phase 3, follow the path an interviewer takes to ask about multithreading. Assume he start with simple code, ask to make it thread safe, asks to use from basic lock to advanced scoped_lock and ask to explain producer consumer problem. How do you do it? **

