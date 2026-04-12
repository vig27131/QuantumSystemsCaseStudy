#include <chrono>
#include <atomic>
#include <thread>
#include <functional>
#include <iostream>

using namespace std::chrono_literals;

void StartThread(
    std::thread& thread,
    std::atomic<bool>& running,
    const std::function<bool(void)>& Process,
    const std::chrono::seconds timeout)
{
    thread = std::thread(
        [&, Process, timeout]()
        {
            auto start = std::chrono::steady_clock::now();

            while (running.load())
            {
                bool aborted = Process();

                auto now = std::chrono::steady_clock::now();
                auto duration = now - start;

                if (aborted || duration > timeout)
                {
                    running.store(false);
                    break;
                }
            }
        });
}

int main()
{
    std::atomic<bool> running1 = true;
    std::atomic<bool> running2 = true;

    std::thread my_thread1, my_thread2;
    int loop_counter1 = 0, loop_counter2 = 0;

    StartThread(
        my_thread1,
        running1,
        [&]()
        {
            std::this_thread::sleep_for(2000ms);
            loop_counter1++;
            return false;
        },
        10s);

    StartThread(
        my_thread2,
        running2,
        [&]()
        {
            if (loop_counter2 < 5)
            {
                std::this_thread::sleep_for(2000ms);
                loop_counter2++;
                return false;
            }
            return true;
        },
        10s);

    my_thread1.join();
    my_thread2.join();

    std::cout << "C1: " << loop_counter1
              << " C2: " << loop_counter2 << std::endl;
}