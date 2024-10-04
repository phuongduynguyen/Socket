#include "SocketClient.h"

void SocketClient::doConnect(const std::string& socketName)
{
    if (mIsConnecting) {
        return;
    }
    mSocketPath = socketName;
    mIsConnecting = true;
    std::function<void(void)> task = [this]() {
        mPeerSocket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
        std::cout << "doConnect() with peer socket: " << mPeerSocket << "\n";
        if (mPeerSocket < 0){
            std::cout << "Cant create socket - error: " << mPeerSocket << "\n";
            return;
        }

        while(mIsConnecting) {
            struct sockaddr_un addr;
            int ret;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = static_cast<unsigned short int>(AF_UNIX);
            strncpy(addr.sun_path, mSocketPath.c_str(), sizeof(addr.sun_path) - 1U);
            addr.sun_path[0] = '\0';
            
            ret = connect(mPeerSocket, (const struct sockaddr *) &addr, static_cast<unsigned int>(sizeof(addr)));
            if (ret < 0) {
                std::cout << "Cant connect socket - error: " << ret << "\n";
                if (mIsConnecting) {
                    usleep(1000000U);
                }
            }
            else if (ret == 0) {
                mIsConnected = true;
                mIsConnecting = false;
                break;
            }
        }
    };
    std::thread* worker = new std::thread(task);
    worker->join();
}

void SocketClient::sendMessage(const std::string& message)
{
    if (!mIsConnected) {
        return;
    }
    
    std::cout << "sendMessage: " << message << "\n";
    static char messageBuf[4096];
    static char tmp[32];

    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    struct tm _time;
    localtime_r(&ts.tv_sec,&_time);
    strftime(tmp, sizeof(tmp), "%F %T", &_time);

    int msgLen = snprintf(messageBuf,sizeof(messageBuf),"[%s.%06ld] Message: %s\n", tmp, ts.tv_nsec / 1000,
                        message.c_str());

    if (msgLen > 0) {
        ssize_t ret = write(mPeerSocket,messageBuf,static_cast<size_t>(msgLen+1));
        if (ret == -1) {
            mIsConnected = false;
            std::cout << "Cant write message, reconnect.... \n";
            doConnect(mSocketPath);
        }
    }
}