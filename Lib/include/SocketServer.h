#ifndef SOCKET_SERVER
#define SOCKET_SERVER

#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string>
#include <functional>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <list>
#include <mutex>

class SocketServer final
{
    public:
        static void doInitialize();
        static SocketServer& getInstance();

        int doStart();
        void doClose();

    private:
        explicit SocketServer();
        ~SocketServer();

        void consumerHandler();

        const char* mSocketPath = "LogCat";
        std::vector<int> mClients;
        volatile bool mRunning;
        std::mutex mMutex;
        std::thread* mConsumerThread;
};

#endif // SOCKET_SERVER