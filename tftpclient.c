#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define TFTP_PACKET 516
#define MODE "octet"

#define OP_RRQ 1
#define OP_WRQ 2
#define OP_DATA 3
#define OP_ACK 4
#define OP_ERROR 5

int main(int argc, char *argv[]) {
    struct addrinfo hints, *res;
    int clientSocket, recvbytes;
    char buffer[TFTP_PACKET];
    short blockno = 1; // For the first data packet, block number starts from 1
    int elementsWritten,receivedBlockNo;
    size_t packet_len;

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <ServerIP> <ServerPort> <Filename>\n", argv[0]);
        exit(1);
    }

    char *serverIP = argv[1];
    char *port = argv[2];
    char *filename = argv[3];

    int fp = open(filename, O_RDWR | O_CREAT, 0777);
    if (fp == -1) {
        perror("File opening failed");
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

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
    short opcode = htons(OP_RRQ); // RRQ opcode in network byte order
    packet_len = 2 + strlen(filename) + 1 + strlen(MODE) + 1; // Calculate packet length
    memset(buffer, 0, TFTP_PACKET);
    memcpy(buffer, &opcode, sizeof(short));
    strcpy((char *)buffer + 2, filename); // File name
    strcpy((char *)buffer + 3 + strlen(filename), MODE); // Mode

    // Send the RRQ packet
    if (sendto(clientSocket, buffer, packet_len, 0, res->ai_addr, res->ai_addrlen) < 0) {
        perror("sendto failed");
        exit(EXIT_FAILURE);
    }

    do {
        recvbytes = recvfrom(clientSocket, buffer, TFTP_PACKET, 0, res->ai_addr, &res->ai_addrlen);
        if (recvbytes < 0) {
            perror("Recvfrom failed");
            exit(EXIT_FAILURE);
        }

        // Check the opcode of the received packet
        opcode = ntohs(*(short *)buffer);

        if (opcode == OP_DATA) {
            // Extract block number from the received packet
            receivedBlockNo = ntohs(*(short *)(buffer + 2));
            
            // Check if the block number is the expected one
            if (receivedBlockNo == blockno) {

                write(fp, buffer + 4, recvbytes - 4);

                // Prepare and send ACK packet
                *(short *)buffer = htons(OP_ACK);    
                // Block number is already correctly placed and in network byte order
                if (sendto(clientSocket, buffer, 4, 0, res->ai_addr, res->ai_addrlen) < 0) {
                    perror("Sendto failed");
                    exit(EXIT_FAILURE);
                }

                // Increment block number for the next expected packet
                blockno++;
            }
        } else if (opcode == OP_ERROR) {
            // Error handling
            printf("Error Packet received\n");
            break;
        }
    } while (recvbytes == 516); // 512 bytes of data + 4 bytes header

    printf("File transfer complete.\n");

    close(fp);
    close(clientSocket);
    freeaddrinfo(res);

    return 0;
}
