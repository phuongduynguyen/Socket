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
        void doConnect();
        void sendMessage(const std::string& message);

    private:
        std::string m_unixAddr = "LogCat";
        volatile int m_peerSocket = -1;
        volatile bool mIsConnected = false;
        volatile bool m_connectingFlag = false;
};
#endif