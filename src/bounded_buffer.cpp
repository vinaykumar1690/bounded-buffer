#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <csignal>

std::atomic<bool> shutdown_flag{false};
std::atomic<bool> producer_done{false};
volatile std::sig_atomic_t sigint_received = 0;

extern "C" void handle_sigint(int) {
    sigint_received = 1;
}

class BoundedBuffer {
    private:
        const size_t cap;
        std::queue<int> q;
        std::mutex mtx;
        std::condition_variable prod_c;
        std::condition_variable cons_c;
        int count = 0;

    public:
        BoundedBuffer(int capacity) : cap{static_cast<size_t>(capacity)} {}

        int size() {
            return static_cast<int>(q.size());
        }

        int val() {
            return q.front();
        }

        // Try to produce one item. Returns false if shutdown requested.
        bool produce() {
            std::unique_lock<std::mutex> lock(mtx);

            // Wait until there's space or a shutdown is requested
            prod_c.wait_for(lock, std::chrono::milliseconds(100), [this]() {
                return q.size() < cap || shutdown_flag.load();
            });

            if (shutdown_flag.load()) return false;

            q.push(count);
            count += 1;

            lock.unlock();

            cons_c.notify_one();
            return true;
        }

        // Consume one item. Returns -1 if no more items will be produced and buffer empty.
        int consume(std::string_view name) {
            std::unique_lock<std::mutex> lock(mtx);

            // Wait until there's data or shutdown
            cons_c.wait_for(lock, std::chrono::milliseconds(100), [this]() {
                return !q.empty() || shutdown_flag.load();
            });

            if (q.empty()) {
                if (shutdown_flag.load() && producer_done.load()) {
                    return -1; // signal to exit
                }
                return static_cast<int>(q.size());
            }

            int val = q.front();
            q.pop();
            int sz = static_cast<int>(q.size());

            lock.unlock();

            prod_c.notify_all();

            std::cout << name << " Consumed: " << val << " | Buffer Size: " << sz << "\n";

            return sz;
        }

        void notify_all() {
            prod_c.notify_all();
            cons_c.notify_all();
        }
};

int main() {

    BoundedBuffer buf(10);

    // install signal handler to set a flag (signal-safe)
    std::signal(SIGINT, handle_sigint);

    // watcher thread will convert signal-safe flag into atomic shutdown and notify buffers
    std::thread watcher([&buf]() {
        while(!shutdown_flag.load()) {
            if (sigint_received) {
                shutdown_flag.store(true);
                std::cout << "received SIGINT. buf size: " << buf.size() << " buf val: " << buf.val();
                buf.notify_all();
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    std::thread producer([&buf]() {
        while(!shutdown_flag.load()) {
            if (!buf.produce()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        producer_done.store(true);
        buf.notify_all();
    });

    std::thread consumer1([&buf]() {
        while(true) {
            int sz = buf.consume("cons1");
            if (sz == -1) break; // no more items
            if (sz == 0) std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    });
    std::thread consumer2([&buf]() {
        while(true) {
            int sz = buf.consume("cons2");
            if (sz == -1) break; // no more items
            if (sz == 0) std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });

    producer.join();
    consumer1.join();
    consumer2.join();
    // ensure watcher exits
    watcher.join();

    return 0;
}