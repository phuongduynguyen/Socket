#include "SocketServer.h"
#define ERROR -1

static SocketServer* gInstance = nullptr;

static std::unordered_map<int, std::string> gSignal = {
       {SIGHUP   , "SIGHUP"},
       {SIGINT   , "SIGINT"},
       {SIGQUIT  , "SIGQUIT"},
       {SIGILL   , "SIGILL"},
       {SIGTRAP  , "SIGTRAP"},
       {SIGABRT  , "SIGABRT"},
       {SIGIOT   , "SIGIOT"},
       {SIGBUS   , "SIGBUS"},
       {SIGFPE   , "SIGFPE"},
       {SIGKILL  , "SIGKILL"},
       {SIGUSR1  , "SIGUSR1"},
       {SIGSEGV  , "SIGSEGV"},
       {SIGUSR2  , "SIGUSR2"},
       {SIGPIPE  , "SIGPIPE"},
       {SIGALRM  , "SIGALRM"},
       {SIGTERM  , "SIGTERM"},
       {SIGSTKFLT, "SIGSTKFLT"},
       {SIGCHLD  , "SIGCHLD"},
       {SIGCLD   , "SIGCLD"},
       {SIGCONT  , "SIGCONT"},
       {SIGSTOP  , "SIGSTOP"},
       {SIGTSTP  , "SIGTSTP"},
       {SIGTTIN  , "SIGTTIN"},
       {SIGTTOU  , "SIGTTOU"},
       {SIGURG   , "SIGURG"},
       {SIGXCPU  , "SIGXCPU"},
       {SIGXFSZ  , "SIGXFSZ"},
       {SIGVTALRM, "SIGVTALRM"},
       {SIGPROF  , "SIGPROF"},
       {SIGWINCH , "SIGWINCH"},
       {SIGIO    , "SIGIO"},
       {SIGPOLL  , "SIGPOLL"},
       {SIGPWR   , "SIGPWR"},
       {SIGSYS   , "SIGSYS"}
};

static std::string getSignal(int signal)
{
    std::string signalStr = "Undefined";
    std::unordered_map<int, std::string>::iterator foundItem = gSignal.begin();
    while (foundItem != gSignal.end())
    {
        if(foundItem->first == signal) {
            signalStr = foundItem->second;
            break;
        }
        foundItem++;
    }
    return signalStr;
}

static void terminateSignalHandler(int signal) {
    std::cout << "terminateSignalHandler signal [" << getSignal(signal) << "] called\n";
    switch (signal)
    {
        case SIGINT: {
            SocketServer::getInstance().doClose();
            break;
        }
        
        default: {
            break;
        }
    }
}

void SocketServer::doInitialize(const std::string& socketName)
{
    if (gInstance == nullptr) {
        gInstance = new SocketServer(socketName);
    }
}

SocketServer& SocketServer::getInstance()
{
    if (gInstance == nullptr) {
        throw std::runtime_error("Must initialize first");
    }
    return *gInstance;
}

SocketServer::SocketServer(const std::string& socketName) : mSocketPath(socketName)
{

}

SocketServer::~SocketServer()
{
    if (mRunning) {
        return;
    }
    
}

int SocketServer::doStart()
{
    std::cout << "Starting server socket: " << std::string(mSocketPath) << "\n";
    struct sockaddr_un address;
    int ret = 0;
    int serverSocket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (serverSocket < 0) {
        std::cout << "Cant open server socket\n";
        return -2;
    }

    ret = fcntl(serverSocket, F_SETFD, O_NONBLOCK);
    if (ret == ERROR) {
        std::cout << "Cant setup operation mode for server socket\n";
        return -3;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = static_cast<unsigned short int>(AF_UNIX);
    strncpy(address.sun_path, mSocketPath.c_str(), sizeof(address.sun_path) - 1U);
    address.sun_path[0]='\0';
    ret = bind(serverSocket, (const struct sockaddr *) &address, static_cast<int>(sizeof(address)));
    
    if (ret == -1) {
        std::cout << "Unable to bind address for the server socket\n";
        (void)close(serverSocket);
        return -4;
    }

    mClients.emplace_back(serverSocket);
    std::cout << "Starting server socket: " << std::string(mSocketPath) << "\n";
    mConsumerThread = new std::thread(&SocketServer::consumerHandler, this);
    mConsumerThread->join();

    for (size_t i = 0; i < mClients.size(); i++)
    {
        close(mClients[static_cast<int>(i)]);
    }

    delete mConsumerThread;
    std::cout << "Finishing\n";
    (void)close(serverSocket);
    return 0;
}

void SocketServer::doClose()
{
    mRunning = false;
    if (mConsumerThread != nullptr) {
        std::unordered_map<int, std::string>::iterator foundItem = gSignal.begin();
        while (foundItem != gSignal.end())
        {
            pthread_kill(mConsumerThread->native_handle(),foundItem->first);
            foundItem++;
        }
    }
    if (gInstance != nullptr) {
        delete gInstance;
        gInstance = nullptr;
    }
    
    std::_Exit(EXIT_SUCCESS);
}

void SocketServer::consumerHandler()
{
    int ret;
    struct sigaction sigAction = {{0}};
    sigAction.sa_flags = 0;
    sigAction.sa_handler = &terminateSignalHandler;
    std::unordered_map<int, std::string>::iterator foundItem = gSignal.begin();
    while (foundItem != gSignal.end())
    {
        if (sigaction(foundItem->first, &sigAction, NULL) == ERROR) {
            std::cout << "Cant setup working thread for signal: " << getSignal(foundItem->first) << "\n";
        }
        foundItem++;
    }

    ret = listen(mClients.front(), 1);
    if(ret == ERROR) {
        std::cout << "Cant listen socket client\n";
        return;
    }

    constexpr int maxEvents = 32;
    struct epoll_event event, events[maxEvents];
    int listenSocket, connectionSocket, nfds, epollFd;

    epollFd = epoll_create1(0);
    if (epollFd == ERROR) {
        std::cout << "Cant open an epoll file descriptor\n";
        return;
    }

    event.events = EPOLLIN;
    event.data.fd = mClients.front();
    
    if(epoll_ctl(epollFd,EPOLL_CTL_ADD,mClients.front(), &event) == ERROR) {
        std::cout << "Cant control interface for an epoll file descriptor\n";
        return;
    }

    while (true)
    {
        std::vector<int> closeLists;
        nfds = epoll_wait(epollFd, &events[0], maxEvents, -1);
        std::cout << "epoll_wait ... nfds: " <<  nfds << "\n";
        if (nfds == ERROR) {
            std::cout << "Cant wait for an I/O event on an epoll file descriptor\n";
            return;
        }

        for (size_t i = 0; i < nfds; i++)
        {
            int clientSocket;
            if (events[i].data.fd == mClients.front()) {
                clientSocket = accept(mClients.front(), nullptr, nullptr);
                if (clientSocket == ERROR) {
                    std::cout << "Establishing a connection failed\n";
                    continue;
                }

                if (mClients.size() >= static_cast<size_t>(maxEvents)) {
                    std::cout << "Number of concurrent connections exceeds\n";
                    continue;
                }

                ret = fcntl(clientSocket,F_SETFD,O_NONBLOCK);

                if (ret == ERROR) {
                    std::cout << "Unable to setup operation mode for the new client socket\n";
                    close(clientSocket);
                    continue;
                }

                event.events = (static_cast<uint32_t>(EPOLLIN)) | (static_cast<uint32_t>(EPOLLET));
                event.data.fd = clientSocket;

                if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == ERROR) {
                    std::cout << "Unable to setup operation mode for the new client socket\n";
                    close(clientSocket);
                    continue;
                }
                
                std::cout << "Adding Peer clients: " << clientSocket << "\n";
                mClients.emplace_back(clientSocket);
            }
            else if (events[i].events & static_cast<uint32_t>(EPOLLIN) != 0) {
                clientSocket = events[i].data.fd;
                static char tempBuf[4096];
                int len;
                do {
                    len = static_cast<int>(read(clientSocket, tempBuf, sizeof(tempBuf)));
                    if (len == 0) {
                        break;
                    }
                    else if (len == -1) {
                        closeLists.emplace_back(clientSocket);
                        break;
                    }
                    else {
                        //Do nothing
                    }
                    tempBuf[len-1] = '\0';
                    std::cout << "[Client: " << clientSocket << "] " <<"- data: " << std::string(tempBuf) << "\n";
                }
                while (1);
            }
            else {
                //Do nothing
            }      
        }

        if (closeLists.size() > 0) {
            std::vector<int> newSockets;
            for (size_t i = 0; i < mClients.size(); i++)
            {
                bool removeFlg = false;
                for (size_t j = 0; j < closeLists.size(); j++)
                {
                    if (mClients[i] == closeLists[j]) {
                        std::cout << "Removing Client: " << mClients[i] << "\n";
                        removeFlg = true;
                    }
                }
                if (!removeFlg) {
                    newSockets.emplace_back(mClients[i]);
                }
            }
            closeLists.clear();
            mClients = newSockets;
        }
    }
    close(epollFd);
}




