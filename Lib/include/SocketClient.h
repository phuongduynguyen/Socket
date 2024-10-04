#ifndef SOCKET_CLIENT
#define SOCKET_CLIENT

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

class SocketClient
{
    public:
        void doConnect(const std::string& socketName);
        void sendMessage(const std::string& message);

    private:
        std::string mSocketPath;
        volatile int mPeerSocket = -1;
        volatile bool mIsConnected = false;
        volatile bool mIsConnecting = false;
};
#endif