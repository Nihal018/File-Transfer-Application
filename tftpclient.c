#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define TFTP_PACKET 516
#define MODE "octet"

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res;
    int clientSocket, recvbytes;
    unsigned char response[TFTP_PACKET];
    short blockno = 0; // Initialized to 0 for the first ACK
    int elementsWritten;
    unsigned char lastAckPacket[4];
    size_t lastAckPacketLen = 0;

    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <ServerIP> <ServerPort> <Filename>\n", argv[0]);
        exit(1);
    }

    char *serverIP = argv[1];
    char *port = argv[2];
    char *filename = argv[3];

    FILE *fd = fopen(filename, "wb");
    if (!fd)
    {
        perror("Cannot open file");
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(serverIP, port, &hints, &res) != 0)
    {
        perror("getaddrinfo failed");
        exit(EXIT_FAILURE);
    }

    clientSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (clientSocket == -1)
    {
        perror("Error creating client socket");
        exit(EXIT_FAILURE);
    }

    // Set the socket timeout for recvfrom
    struct timeval timeout;
    timeout.tv_sec = 0; // 5 seconds timeout
    timeout.tv_usec = 500000;

    if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Prepare the RRQ packet
    short opcode = htons(1); // RRQ opcode in network byte order
    size_t packet_len = 2 + strlen(filename) + 1 + strlen(MODE) + 1;
    memset(response, 0, TFTP_PACKET);
    memcpy(response, &opcode, sizeof(short));
    strcpy((char *)response + 2, filename);
    strcpy((char *)response + 3 + strlen(filename), MODE);

    // Send the RRQ packet
    if (sendto(clientSocket, response, packet_len, 0, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("sendto failed");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        recvbytes = recvfrom(clientSocket, response, TFTP_PACKET, 0, NULL, NULL);
        if (recvbytes < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                printf("Timeout reached. Resending last ACK.\n");
                if (lastAckPacketLen > 0)
                {
                    if (sendto(clientSocket, lastAckPacket, lastAckPacketLen, 0, res->ai_addr, res->ai_addrlen) < 0)
                    {
                        perror("sendto failed");
                        exit(EXIT_FAILURE);
                    }
                }
                continue;
            }
            else
            {
                perror("recvfrom failed");
                exit(EXIT_FAILURE);
            }
        }

        memcpy(&opcode, response, sizeof(short));
        opcode = ntohs(opcode);
        if (opcode == 3)
        { // DATA packet
            short receivedBlockNo;
            memcpy(&receivedBlockNo, response + 2, sizeof(short));
            receivedBlockNo = ntohs(receivedBlockNo);

            if (receivedBlockNo == blockno + 1)
            {
                elementsWritten = fwrite(response + 4, 1, recvbytes - 4, fd);
                if (elementsWritten < recvbytes - 4)
                {
                    fprintf(stderr, "Error writing to file\n");
                    break;
                }

                blockno = receivedBlockNo; // Update block number for ACK

                // Prepare ACK packet
                memset(lastAckPacket, 0, sizeof(lastAckPacket));
                opcode = htons(4);                 // ACK opcode
                short netBlockNo = htons(blockno); // Convert block number to network byte order
                memcpy(lastAckPacket, &opcode, sizeof(short));
                memcpy(lastAckPacket + 2, &netBlockNo, sizeof(netBlockNo)); // Include block number
                lastAckPacketLen = 4;

                // Send ACK packet
                if (sendto(clientSocket, lastAckPacket, lastAckPacketLen, 0, res->ai_addr, res->ai_addrlen) < 0)
                {
                    perror("sendto failed");
                    exit(EXIT_FAILURE);
                }

                if (recvbytes < 516)
                { // Last packet
                    printf("File transfer complete.\n");
                    break;
                }
            }
        }
    }

    fclose(fd);
    close(clientSocket);
    freeaddrinfo(res);

    return 0;
}
