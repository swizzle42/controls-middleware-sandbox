#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
    // create a socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // print the socket FD
    std::cout << "server socket FD: " << serverSocket << std::endl;

    // specify the address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // bind the socket
    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    // listen to the assigned socket
    listen(serverSocket, 5);

    return 0;
}