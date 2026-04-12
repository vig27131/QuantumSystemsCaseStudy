// Updated task 3
// Task 3
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <queue>

// Libraries for actual UDP sending
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

class UDPSCHEDULER
{
    public:
        using Task = function<void()>;

        void sendUdpImmediately(const string& ip,const uint16_t& port,const string& mesg);
        void sendUdpAfterXSeconds(const string& ip, const uint16_t& port, const string& mesg, const uint8_t& delaySeconds);
        void sendUdpEveryXSeconds(const string& ip, const uint16_t& port, const string& mesg, const uint8_t& intervalSeconds);

        void run();
        void stop(const uint8_t& timeoutSeconds = 0);

    private:
        struct ScheduledTask
        {
            Task task;
            chrono::steady_clock::time_point whenToRun;
            uint8_t interval {0};
            bool operator<(const ScheduledTask& other) const
            {
                return whenToRun > other.whenToRun; // earlier whenToRun has higher priority
            }
        };

        void sendUdp(const string& ip,const uint16_t& port,const string& mesg);
        
        queue<Task> immediateQ;
        priority_queue<ScheduledTask> scheduledQ;

        atomic<bool> stopFlag {false};
};

void UDPSCHEDULER::sendUdp(const string& ip,const uint16_t& port,const string& mesg)
{
    cout << "Sending UDP message : " << mesg << "; destination : " << ip << ":" << port << endl;

    // Send the UDP message using sockets
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) 
    {
        cout << "Failed to create socket" << endl;
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = port;
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr); // Convert IP address from string to binary format

    // cout << inet_ntoa(addr.sin_addr) << endl; // Print the IP address in dotted-decimal format to verify correctness

    sendto(sock, mesg.c_str(), mesg.size(), 0,
           reinterpret_cast<sockaddr*>(&addr), sizeof(addr)); // Send the message to the specified IP and port

    close(sock);
}

void UDPSCHEDULER::sendUdpImmediately(const string& ip, const uint16_t& port, const string& mesg)
{
    immediateQ.push([this, ip, port, mesg]() {
        sendUdp(ip, port, mesg);
    });
    
}

void UDPSCHEDULER::sendUdpAfterXSeconds(const string& ip, const uint16_t& port, const string& mesg, const uint8_t& delaySeconds)
{
    ScheduledTask scheduledTask;
    scheduledTask.task = [this, ip, port, mesg](){
        sendUdp(ip, port, mesg);
    };
    scheduledTask.whenToRun = chrono::steady_clock::now() + chrono::seconds(delaySeconds);
    scheduledQ.push(scheduledTask);
}


void UDPSCHEDULER::sendUdpEveryXSeconds(const string& ip, const uint16_t& port, const string& mesg, const uint8_t& intervalSeconds)
{  
    ScheduledTask scheduledTask;
    scheduledTask.task = [this, ip, port, mesg](){
        sendUdp(ip, port, mesg);
    };
    scheduledTask.whenToRun = chrono::steady_clock::now() + chrono::seconds(intervalSeconds);
    scheduledTask.interval = intervalSeconds;
    scheduledQ.push(scheduledTask);
}

void UDPSCHEDULER::run()
{
    while(stopFlag == false)
    {
        chrono::time_point<chrono::steady_clock> startTime = chrono::steady_clock::now();
        while(!scheduledQ.empty() && (scheduledQ.top().whenToRun <= startTime))
        {
            scheduledQ.top().task();
            //Rechedule the periodic task
            if(scheduledQ.top().interval > 0)
            {
                ScheduledTask updatedTask = scheduledQ.top();
                updatedTask.whenToRun = chrono::steady_clock::now() + chrono::seconds(updatedTask.interval);
                scheduledQ.push(updatedTask);
                cout << "Scheduled using sendUdpEveryXSeconds" << endl << endl;
            }
            else
            {
                cout << "Scheduled using sendUdpAfterXSeconds" << endl << endl;
            }
            scheduledQ.pop();
        }

        if(!immediateQ.empty())
        {
            chrono::time_point<chrono::steady_clock> now = chrono::steady_clock::now();

            immediateQ.front()();
            immediateQ.pop();
            auto duration = (now - startTime);
            cout << "Scheduled using sendUdpImmediately " << endl << endl;
        }

        if((immediateQ.empty() && scheduledQ.empty()) || stopFlag)
        {
            break;
        }

        if(!scheduledQ.empty())
        {
            chrono::time_point nextTime = scheduledQ.top().whenToRun;
            if(nextTime > chrono::steady_clock::now())
            {
                this_thread::sleep_until(nextTime);
            }
            continue;
        }
    }
}

void UDPSCHEDULER::stop(const uint8_t& timeoutSeconds)
{
    cout << "Program will stop after " << static_cast<int>(timeoutSeconds) << " seconds" << endl << endl;
    std::this_thread::sleep_for(std::chrono::seconds(timeoutSeconds)); // Let the scheduler run for a while before stopping it
    stopFlag = true;
}

int main()
{
    string ip = "192.168.1.123";
    // string msg = "Hello";
    uint16_t port = 9090U;

    uint8_t programTimeout = 10U; // Control how long the application is supposed to run in seconds

    UDPSCHEDULER udploop;

    udploop.sendUdpImmediately(ip, port, "I print immediately");
    udploop.sendUdpAfterXSeconds(ip, port, "Delayed print after 5 seconds", 5);
    udploop.sendUdpEveryXSeconds(ip, port, "I print every 2 seconds", 2);
    udploop.sendUdpEveryXSeconds(ip, port, "I print every 5 seconds", 5);

    thread runThread([&udploop](){
        udploop.run();
    });

    thread stopThread([&udploop, &programTimeout](){
        udploop.stop(programTimeout); // Stops the program after programTimeout seconds
        // udploop.stop(); // Stops the program as soon as it starts
    });
    
    runThread.join();
    stopThread.join();
    
    return 0;
}
