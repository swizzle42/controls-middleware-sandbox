#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "controls_middleware/packet.h"
#include "controls_middleware/server.h"

int main() {
    // create a socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // print the socket FD
    std::cout << "server socket FD: " << server_socket << std::endl;

    // specify the address
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // bind the socket
    bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // listen to the assigned socket
    listen(server_socket, 5);

    // accept an incoming connection (blocking)
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    std::cout << "Waiting for inbound connection" << std::endl;
    int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
    std::cout << "Client connected. Assigned FD: " << client_socket << std::endl;

    // recieve data
    sensor_packet incoming_packet;
    read(client_socket, &incoming_packet, sizeof(incoming_packet));
    std::cout << "[RECV]" 
    << "\tdevice_id: " << incoming_packet.device_id 
    << "\tstatus: " << incoming_packet.status 
    << "\tvalue: " << incoming_packet.value 
    << std::endl;

    close(client_socket);
    close(server_socket);

    return 0;
}