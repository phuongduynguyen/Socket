#include "SocketClient.h"

void SocketClient::doConnect()
{
    if (m_connectingFlag) {
        return;
    }

    m_connectingFlag = true;
    std::function<void(void)> task = [this]() {
        m_peerSocket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
        std::cout << "doConnect() with peer socket: " << m_peerSocket << "\n";
        if (m_peerSocket < 0){
            std::cout << "Cant create socket - error: " << m_peerSocket << "\n";
            return;
        }

        while(m_connectingFlag) {
            struct sockaddr_un addr;
            int ret;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = static_cast<unsigned short int>(AF_UNIX);
            strncpy(addr.sun_path, m_unixAddr.c_str(), sizeof(addr.sun_path) - 1U);
            addr.sun_path[0] = '\0';
            
            ret = connect(m_peerSocket, (const struct sockaddr *) &addr, static_cast<unsigned int>(sizeof(addr)));
            if (ret < 0) {
                std::cout << "Cant connect socket - error: " << ret << "\n";
                if (m_connectingFlag) {
                    usleep(1000000U);
                }
            }
            else if (ret == 0) {
                mIsConnected = true;
                m_connectingFlag = false;
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
        ssize_t ret = write(m_peerSocket,messageBuf,static_cast<size_t>(msgLen+1));
        if (ret == -1) {
            mIsConnected = false;
            std::cout << "Cant write message, reconnect.... \n";
            doConnect();
        }
    }
}