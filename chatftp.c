#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 1024
#define TFTP_PACKET 516
#define MODE "octet"

int main(int argc, char *argv[]) {
    struct addrinfo hints, *res;
    int clientSocket, recvbytes;
    unsigned char response[TFTP_PACKET];
    short blockno = 1;
    int elementsWritten;

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <ServerIP> <ServerPort> <Filename>\n", argv[0]);
        exit(1);
    }

    char *serverIP = argv[1];
    char *port = argv[2];
    char *filename = argv[3];

    FILE *fd = fopen(filename, "wb");
    if (!fd) {
        perror("Cannot open file");
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // Use IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM; // Datagram socket

    if (getaddrinfo(serverIP, port, &hints, &res) != 0) {
        perror("getaddrinfo failed");
        exit(EXIT_FAILURE);
    }

    clientSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (clientSocket == -1) {
        perror("Error creating client socket");
        exit(EXIT_FAILURE);
    }

    // Prepare the RRQ packet
    short opcode = htons(1); // RRQ opcode in network byte order
    size_t packet_len = 2 + strlen(filename) + 1 + strlen(MODE) + 1; // Dynamic packet length calculation
    memset(response, 0, TFTP_PACKET);
    memcpy(response, &opcode, sizeof(short));
    strcpy((char *)response + 2, filename); // Ensure we include the null terminator
    strcpy((char *)response + 3 + strlen(filename), MODE); // Ensure we include the null terminator

    // Send the RRQ packet
    if (sendto(clientSocket, response, packet_len, 0, res->ai_addr, res->ai_addrlen) < 0) {
        perror("sendto failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        recvbytes = recvfrom(clientSocket, response, TFTP_PACKET, 0, NULL, NULL);
        if (recvbytes < 0) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }
        
        // Check for DATA packet
        memcpy(&opcode, response, sizeof(short));
        opcode = ntohs(opcode); // Convert opcode to host byte order
        if (opcode == 3) { // DATA packet
            elementsWritten = fwrite(response + 4, 1, recvbytes - 4, fd);
            if (elementsWritten < recvbytes - 4) {
                fprintf(stderr, "Error writing to file\n");
                break;
            }
            
            // Send ACK
            memset(response, 0, TFTP_PACKET);
            opcode = htons(4); // ACK opcode
            memcpy(response, &opcode, sizeof(short));
            memcpy(response + 2, &blockno, sizeof(short)); // block number already in network byte order
            if (sendto(clientSocket, response, 4, 0, res->ai_addr, res->ai_addrlen) < 0) {
                perror("sendto failed");
                exit(EXIT_FAILURE);
            }
            
            if (recvbytes < 516) { // Last packet
                break;
            }
            blockno++; // Prepare for the next block
        }
    }

    fclose(fd);
    close(clientSocket);
    freeaddrinfo(res); // Free the linked list allocated by getaddrinfo()

    return 0;
}
