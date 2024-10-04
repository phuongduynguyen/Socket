#include "SocketServer.h"

int main()
{
    SocketServer::doInitialize();
    return SocketServer::getInstance().doStart();
}