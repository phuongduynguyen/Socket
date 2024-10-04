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
    /**
     *  socket(int __domain, int __type, int __protocol) : Create instance socket from OS
     *  - __domain: AF_UNIX - UNIX DOMAIN SOCKET (tranport via file)
     *  - __type: SOCK_SEQPACKET - Before transport, Client and Server must have been established connection, Data sent over this socket is guaranteed to arrive at its destination in the exact order in which it was sent.
     *
    */
    int serverSocket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (serverSocket < 0) {
        std::cout << "Cant open server socket\n";
        return -2;
    }

    /**
     * fcntl(int __fd, int __cmd, ...) : Change the properties of a socket or file
     * - __fd: File descriptor for socket instance
     * - __cmd: Set flags for file descriptor (O_NONBLOCK)
     * - O_NONBLOCK: Specifies that the socket will operate in asynchronous (non-blocking) mode. When the socket is in this mode, calls to functions like recv or send will not block the program. 
     *               Instead, if there is no data to read or data cannot be sent immediately, these functions will return immediately with an error code (usually EAGAIN or EWOULDBLOCK).
     */
    ret = fcntl(serverSocket, F_SETFD, O_NONBLOCK);
    if (ret == ERROR) {
        std::cout << "Cant setup operation mode for server socket\n";
        return -3;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = static_cast<unsigned short int>(AF_UNIX);
    strncpy(address.sun_path, mSocketPath.c_str(), sizeof(address.sun_path) - 1U);
    address.sun_path[0]='\0';

    /**
     * int bind(int __fd, const sockaddr *__addr, socklen_t __len): Bind a socket to a specific address.
     * - __fd: File descriptor for socket instance
     * - __addr : Get the address of the address variable, which is a variable of type sockaddr_un, containing the address information for the Unix socket.
     *                                        (const struct sockaddr *) converts the data type of &address from sockaddr_un to sockaddr. This is necessary because the bind function receives a pointer to sockaddr, not sockaddr_un.
     *                                        This type conversion allows bind to work with different types of addresses (like IPv4, IPv6, Unix Domain, etc.) without knowing the details.
     */
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

    /**
     * int listen(int __fd, int __n): Marks the server socket as a listening socket, able to accept connections from clients.
     * - __fd: The file descriptor of the socket you want to listen on (in this case mClients.front()).
     * - __n: Backlog - The maximum number of waiting connections that a socket can hold. If the number of waiting connections exceeds this value, new connections will be refused until the current connection is processed.
     */
    ret = listen(mClients.front(), 1);
    if(ret == ERROR) {
        std::cout << "Cant listen socket client\n";
        return;
    }

    constexpr int maxEvents = 32;
    struct epoll_event event, events[maxEvents];
    int listenSocket, connectionSocket, nfds, epollFd;

    /**
     * int epoll_create1(int __flags): Create an epoll instance in a Linux system, allowing monitoring of multiple sockets or file descriptors for events such as reads, writes, errors, etc.
     * - __flags: The flags parameter can be used to specify different options for the epoll instance.
     */
    epollFd = epoll_create1(0);
    if (epollFd == ERROR) {
        std::cout << "Cant open an epoll file descriptor\n";
        return;
    }

    /*
    * EPOLLIN: Events with data to read
    * EPOLLOUT: Events that are writable, etc.
    */
    event.events = EPOLLIN;
    event.data.fd = mClients.front();
    
    /**
     * int epoll_ctl(int __epfd, int __op, int __fd, epoll_event *): Add a file descriptor to the created epoll instance
     * - __epfd: This is the file descriptor of the epoll instance you created earlier using epoll_create1. It is used to identify the epoll instance you want to operate on.
     * -__op: This is a command that tells epoll_ctl that you want to add a file descriptor to the epoll instance. It is one of the values ​​that can be used to specify the action (other values ​​include EPOLL_CTL_MOD for modify and EPOLL_CTL_DEL for delete).
     * - __fd: The descriptor file of the server socket (or a client socket) that you want to monitor. This is the file descriptor that you want to add to the epoll instance
     * - epoll_event *: This is a pointer to a variable of type epoll_event, which defines the properties of the file descriptor you are adding to epoll.
     */
    if(epoll_ctl(epollFd,EPOLL_CTL_ADD,mClients.front(), &event) == ERROR) {
        std::cout << "Cant control interface for an epoll file descriptor\n";
        return;
    }

    while (true)
    {
        std::vector<int> closeLists;

        /**
         * int epoll_wait(int __epfd, epoll_event *__events, int __maxevents, int __timeout): Tell the epoll instance to wait for and return any events that occur on the file descriptors you added earlier.
         * - __epfd: This is the file descriptor of the epoll instance you created earlier using epoll_create1. It is used to identify the epoll instance you want to wait for events on.
         * - __events: Events is an array (or vector) of epoll_event structures, each of which contains information about a file descriptor you are monitoring and the events you want to receive.
         * - __maxevents: This is the maximum number of events you want epoll_wait to receive in a single call. If more events occur than this value, epoll_wait only returns the first maxEvents event.
         * - __timeout: -1 means wait forever until have event come
         */
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




