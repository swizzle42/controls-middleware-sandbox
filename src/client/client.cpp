#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "controls_middleware/packet.h"
#include "client.h"

int client() {
    // create a socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    // specify the address
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // send a connection request
    connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // create data
    sensor_packet outgoing_packet;
    outgoing_packet.device_id = 42;
    outgoing_packet.status = 1;
    outgoing_packet.value = 123.4;

    // send data
    send(client_socket, &outgoing_packet, sizeof(outgoing_packet), 0);

    // close the client socket
    close(client_socket);

    return 0;
}

