#include "SocketClient.h"

int main()
{
    std::cout << "Current PID: " << getpid() << std::endl;
    SocketClient client;
    client.doConnect();
    std::string input;

    while (true)
    {
        std::cout << "Enter Message: ";
        std::getline(std::cin, input);
        client.sendMessage(input);
    }
    
}
