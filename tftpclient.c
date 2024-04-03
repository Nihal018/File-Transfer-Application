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
    unsigned char response[TFTP_PACKET], check[BUFFER_SIZE];
    struct timespec start, end;
    unsigned char payload[TFTP_PACKET]; 
    char * mode;
    int num_packets;

    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <ServerIP> <ServerPort> <P> <TTL> <NumPackets>\n", argv[0]);
        exit(1);
    }

    char *serverIP = argv[1];
    char *port = argv[2];
    char *filename=argv[3] ;

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

    short opcode= 1;  // first write implement for the reading files . RRQ

    memset(response, 0, BUFFER_SIZE);
    memcpy(response,&opcode,sizeof(short));
    memcpy(response+2,filename,sizeof(filename));
    memcpy(response+2+sizeof(filename)+1,MODE,sizeof(MODE));

    sendto(clientSocket,response,TFTP_PACKET,0,res->ai_addr,res->ai_addrlen);

    whiel(1){
        recvbytes = recvfrom(clientSocket, response, BUFFER_SIZE, 0, NULL, NULL);
        if(recvbytes==516){
            if()

        }

    }




    




    for (int i = 0; i < numPackets; i++)
    {
        memset(response, 0, BUFFER_SIZE);
     
        int sequenceNumber = i;
        memcpy(response, &sequenceNumber, sizeof(int));
        response[4] = TTL;
        short payloadLength = P;
        memcpy(response + 5, &payloadLength, sizeof(short));
        memcpy(response + 7, payload, P);       

        clock_gettime(CLOCK_MONOTONIC, &start);

        
        sendto(clientSocket, response, 7 + P, 0, res->ai_addr, res->ai_addrlen);

     
        recvbytes = recvfrom(clientSocket, response, BUFFER_SIZE, 0, NULL, NULL);
        
        
        response[recvbytes] = '\0'; 

        if (strcmp((char*)response + 7, "MALFORMED PACKET") == 0) 
        {
            printf("MALFORMED PACKET\n");
        }
        else
        {
            clock_gettime(CLOCK_MONOTONIC, &end);

            double time_taken;
            time_taken = (end.tv_sec - start.tv_sec) * 1e9;
            time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;

            printf("Packet %d RTT: %f seconds\n", i, time_taken);
        }

        
    }








    // Initializing payload with 'a'
    memset(payload, 'a', P); 


    close(clientSocket);
    return 0;
}
