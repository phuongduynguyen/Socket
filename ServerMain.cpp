#include "SocketServer.h"

int main()
{
    SocketServer::doInitialize("DuySocket");
    return SocketServer::getInstance().doStart();
}