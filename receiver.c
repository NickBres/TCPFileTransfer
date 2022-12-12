#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <time.h>

#define SERVER_PORT 5300
#define SERVER_IP_ADDRESS "127.0.0.1"

#define BUFF_SIZE 1024

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // create socket

    if (sock < 0)
    {
        printf("Error creating socket. errno: %d", errno);
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    int rval = inet_pton(AF_INET, SERVER_IP_ADDRESS, &serverAddress.sin_addr); // convert IP address
    if (rval <= 0)
    {
        printf("Error converting IP address.errno: %d", errno);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // connect to server
    {
        printf("Error connecting to server. errno: %d", errno);
        return -1;
    }
    printf("1.Connected to server\n");

    int flag = 1;
    while (flag) // loop until server says to exit
    {
        double avgTime = 0;
        if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, "cubic", 6) < 0) // change congestion control algorithm
        {
            printf("Error setting socket options : %d", errno);
            return -1;
        }
        printf("    Congestion control algorithm: Cubic\n");

        int fileSize = write_file(sock, &avgTime); // start to recieve file and write it
        printf("10.File received. Size: %d AvgTimeForHalf: %f\n", fileSize, avgTime);


        char ans[10] = {0};
        printf("    Waiting for server to approve and finish\n");
        int recvSize = recv(sock, ans, 10, 0); // receive approval from server
        if (recvSize < 0)
        {
            printf("Error receiving message. errno: %d\n", errno);
            return -1;
        }
        else if (recvSize == 0)
        {
            printf("Connection closed by server\n");
            break;
        }else if(strcmp(ans, "Exit") == 0){ // server says to exit
            printf("    Approved.\n");
            flag = 0;
        }
        send(sock, "OK", 2, 0);// send ACK to server
    }
    printf("    Connection closed\n");
    close(sock); // close socket
    return 0;
}

int write_file(int sockfd, double *avgTime)
{
    FILE *fp;
    char buffer[BUFF_SIZE] = {0};
    clock_t start, end;
    double cpu_time_used_1, cpu_time_used_2;

    fp = fopen("recv.txt", "w"); // create file
    if (fp == NULL)
    {
        printf("Error opening file");
        return 1;
    }
    start = clock(); // start timer to measure time for first part
    while (1)
    {
        int recvSize = recv(sockfd, buffer, BUFF_SIZE, 0); // receive data from server
        if (recvSize < 0)
        {
            printf("Error receiving message. errno: %d\n", errno);
            return -1;
        }
        else if (recvSize == 0)
        {
            printf("Connection closed by server\n");
            break;
        }

        if (strcmp(buffer, "SEND ME A KEY") == 0) // server asks for key
        {
            printf("3.First part recieved\n");
            createKey(buffer); // puts the key in the buffer
            int bytesSent = send(sockfd, buffer, 10, 0); // send key to server

            end = clock(); // stop timer
            cpu_time_used_1 = ((double)(end - start)) / CLOCKS_PER_SEC; // calculate time for first part
            printf("4.Time taken: %f\n", cpu_time_used_1);

            if (bytesSent < 0)
            {
                printf("Error sending message. errno: %d\n", errno);
                return -1;
            }
            else if (bytesSent == 0)
            {
                printf("Connection closed by server\n");
                break;
            }
            printf("6.Key sent\n");
            if (setsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, "reno", 6) < 0) // change congestion control algorithm
            {
                printf("Error setting socket options : %d", errno);
                return -1;
            }
            printf("    Congestion control algorithm: Reno\n");
            
            start = clock(); // start timer for second part
        }
        else if (strcmp(buffer, "FINISHED") == 0) // server says that it finished to send the file
        {
            end = clock(); // stop timer for second part
            cpu_time_used_2 = ((double)(end - start)) / CLOCKS_PER_SEC; // calculate time for second part
            printf("7.Second part recieved\n");
            printf("8.Time taken: %f\n", cpu_time_used_2);
            break;
        }
        else // its file data
        {
            fwrite(buffer, 1, recvSize, fp); // write data to file
            int ans = send(sockfd, buffer, recvSize, 0); // send ACK to server. ACK is the same data that we received
            if (ans < 0)
            {
                printf("Error sending message. errno: %d\n", errno);
                return -1;
            }
            else if (ans == 0)
            {
                printf("Connection closed by server\n");
                break;
            }
        }

        bzero(buffer, BUFF_SIZE); // clear buffer
    }

    fseek(fp, 0L, SEEK_END); // go to end of file
    int sz = ftell(fp); // get file size
    rewind(fp); // go to start of file
    fclose(fp); // close file
    *avgTime = (cpu_time_used_1 + cpu_time_used_2) / 2; // calculate average time for half
    return sz;
}

void createKey(char *buffer)
{
    int key = 3498 ^ 420;
    sprintf(buffer, "%d", key); // put key in buffer
}
