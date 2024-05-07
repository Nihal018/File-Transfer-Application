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
#include <time.h>

#define BUFFER_SIZE 1024
#define TFTP_PACKET 516
#define MODE "octet"

#define OP_RRQ 1
#define OP_WRQ 2
#define OP_DATA 3
#define OP_ACK 4
#define OP_ERROR 5

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res;
    int serverSocket, recvbytes, readBytes;
    char buffer[TFTP_PACKET];
    short blockno = 1; // For the first data packet, block number starts from 1
    int elementsWritten, receivedBlockNo;
    size_t packet_len;
    char *filename;
    struct timespec start, end;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <ServerPort>\n", argv[0]);
        exit(1);
    }

    char *port = argv[1];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &hints, &res) != 0)
    {
        perror("getaddrinfo failed");
        exit(EXIT_FAILURE);
    }

    serverSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (serverSocket == -1)
    {
        perror("Error creating client socket");
        exit(EXIT_FAILURE);
    }

    if (bind(serverSocket, res->ai_addr, res->ai_addrlen) == -1)
    {
        close(serverSocket);
        perror("server: bind");
        return 1;
    }

    recvbytes = recvfrom(serverSocket, buffer, TFTP_PACKET, 0, res->ai_addr, &res->ai_addrlen);
    if (recvbytes < 0)
    {
        perror("Recvfrom failed");
        exit(EXIT_FAILURE);
    }

    strcpy(filename, (char *)buffer + 2);

    int fp = open(filename, O_RDWR | O_CREAT, 0777);
    if (fp == -1)
    {
        perror("File opening failed");
        return 1;
    }

    packet_len = 4 + 512; // Calculate packet length

    while (1)
    {

        // Prepare the DATA  packet
        short opcode = htons(OP_DATA); // DATA opcode in network byte order
        memset(buffer, 0, TFTP_PACKET);
        memcpy(buffer, &opcode, sizeof(short));
        memcpy(buffer + 2, &blockno, sizeof(short)); // File name

        readBytes = read(fp, buffer + 4, packet_len - 4);
        if (readBytes < 512)
        {
            printf("This will be the last data packet sent");
            if (sendto(serverSocket, buffer, packet_len, 0, res->ai_addr, res->ai_addrlen) < 0)
            {
                perror("sendto failed");
                exit(EXIT_FAILURE);
            }
        }

        if (sendto(serverSocket, buffer, packet_len, 0, res->ai_addr, res->ai_addrlen) < 0)
        {
            perror("sendto failed");
            exit(EXIT_FAILURE);
        }
    }

    printf("File transfer complete.\n");

    close(fp);
    close(serverSocket);
    freeaddrinfo(res);

    return 0;
}
