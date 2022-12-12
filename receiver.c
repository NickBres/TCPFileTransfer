#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define SERVER_PORT 5300
#define SERVER_IP_ADDRESS "127.0.0.1"

#define BUFF_SIZE 1024

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock < 0)
    {
        printf("Error creating socket. errno: %d", errno);
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    int rval = inet_pton(AF_INET, SERVER_IP_ADDRESS, &serverAddress.sin_addr);
    if (rval <= 0)
    {
        printf("Error converting IP address.errno: %d", errno);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        printf("Error connecting to server. errno: %d", errno);
        return -1;
    }
    printf("Connected to server\n");

    int flag = 1;
    while (flag)
    {
        int fileSize = write_file(sock);
        printf("File received. Size: %d \n", fileSize);
        char ans[10] = {0};
        printf("Do you want to try again? (yes/): ");
        scanf("%s", ans);
        if (strcmp(ans, "yes"))
        {
            flag = 0;
            int sendCount = send(sock, "Exit", 4, 0);
            if (sendCount < 0)
            {
                printf("Error sending message. errno: %d\n", errno);
                return -1;
            }
            else if (sendCount == 0)
            {
                printf("Connection closed by server\n");
                break;
            }
            close(sock);
        }else{
            int sendCount = send(sock, "Again", 6, 0);
            if (sendCount < 0)
            {
                printf("Error sending message. errno: %d\n", errno);
                return -1;
            }
            else if (sendCount == 0)
            {
                printf("Connection closed by server\n");
                break;
            }
        }

    }
}

int write_file(int sockfd)
{
    FILE *fp;
    char buffer[BUFF_SIZE] = {0};

    fp = fopen("recv.png", "w");
    if (fp == NULL)
    {
        printf("Error opening file");
        return 1;
    }
    while (1)
    {
        int recvSize = recv(sockfd, buffer, BUFF_SIZE, 0);
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

        if (strcmp(buffer, "SEND ME A KEY") == 0)
        {
            createKey(buffer); // puts the key in the buffer
            int bytesSent = send(sockfd, buffer, 10, 0);
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
        }
        else if (strcmp(buffer, "FINISHED") == 0)
        {
            printf("File received successfully\n");
            break;
        }
        else
        {
            fwrite(buffer, 1, recvSize, fp);
            int ans = send(sockfd, buffer, recvSize, 0);
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

        bzero(buffer, BUFF_SIZE);
    }

    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp);
    rewind(fp);
    fclose(fp);
    return sz;
}

void createKey(char *buffer)
{
    int key = 3498 ^ 420;
    sprintf(buffer, "%d", key);
}
