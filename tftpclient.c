#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <netdb.h>

#define BUFFER_SIZE 1024
#define TFTP_PACKET 516
#define MODE "octet"

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res;
    int clientSocket, recvbytes;
    unsigned char response[TFTP_PACKET];
    struct timespec start, end;
    unsigned char payload[TFTP_PACKET];
    char *mode;
    int num_packets;
    short blockno = 1;
    int elementsWritten;

    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <ServerIP> <ServerPort> <P> <TTL> <NumPackets>\n", argv[0]);
        exit(1);
    }

    char *serverIP = argv[1];
    char *port = argv[2];
    char *filename = argv[3];

    FILE *fd = fopen(filename, "wb");

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    getaddrinfo(serverIP, port, &hints, &res);

    clientSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (clientSocket == -1)
    {
        perror("Error creating client socket");
        return -1;
    }

    printf("Client: socket has been created\n");

    short opcode = 1; // first write implement for the reading files . RRQ

    memset(response, 0, TFTP_PACKET);
    memcpy(response, &opcode, sizeof(short));

    memcpy(response + 2, filename, strlen(filename));
    // Add a null byte after the filename
    response[2 + strlen(filename)] = '\0';
    // Copy the mode
    memcpy(response + 3 + strlen(filename), MODE, strlen(MODE));
    // Add a final null byte after the mode
    response[3 + strlen(filename) + strlen(MODE)] = '\0';

    size_t packet_len = 2 + strlen(filename) + 1 + strlen(MODE) + 1;
    sendto(clientSocket, response, packet_len, 0, res->ai_addr, res->ai_addrlen);

    blockno = 0;

    while (1)
    {
        recvbytes = recvfrom(clientSocket, response, TFTP_PACKET, 0, NULL, NULL);
        if (recvbytes == 516)
        {
            memcpy(&opcode, response, sizeof(short));
            if (opcode == 3)
            {
                elementsWritten = fwrite(response + 4, sizeof(unsigned char), recvbytes - 4, fd);
                if (elementsWritten != recvbytes - 4)
                {
                    // Handle error
                    printf("Error writing to file.\n");
                }

                memset(response, 0, BUFFER_SIZE);
                opcode = 4;
                memcpy(response, &opcode, sizeof(short));
                blockno++;
                memcpy(response + 2, &blockno, sizeof(blockno));
                sendto(clientSocket, response, 4, 0, res->ai_addr, res->ai_addrlen);
            }
        }
        else
        {
            // end of transmission
            elementsWritten = fwrite(response + 4, sizeof(unsigned char), recvbytes - 4, fd);
            if (elementsWritten != recvbytes - 4)
            {
                // Handle error
                printf("Error writing to file.\n");
            }

            memset(response, 0, BUFFER_SIZE);
            opcode = 4;
            memcpy(response, &opcode, sizeof(short));
            blockno++;
            memcpy(response + 2, &blockno, sizeof(blockno));
            sendto(clientSocket, response, 4, 0, res->ai_addr, res->ai_addrlen);
            break;
        }
    }
    fclose(fd);

    close(clientSocket);
    return 0;
}
