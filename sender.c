#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#define SERVER_PORT 5300
#define BUFF_SIZE 1024

int main()
{
  //----------------Read file--------------------
  printf("1.Read file\n");
  FILE *fp = fopen("./book.txt", "r");
  if (fp == NULL)
  {
    printf("Error opening file\n");
    return -1;
  }
  fseek(fp, 0L, SEEK_END);
  int sz = ftell(fp);
  printf("  File size: %d\n", sz);
  int flag = 1;

  //----------------------------------------------

  signal(SIGPIPE, SIG_IGN);

  int listeningSocket = -1;

  if ((listeningSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("Error creating socket : %d", errno);
    return -1;
  }

  int enableReuse = 1;
  if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0)
  {
    printf("Error setting socket options : %d", errno);
    return -1;
  }

  struct sockaddr_in serverAddress;
  memset(&serverAddress, 0, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  serverAddress.sin_port = htons(SERVER_PORT);

  if (bind(listeningSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
  {
    printf("Error binding socket : %d", errno);
    return -1;
  }
  // printf("Binding successful\n");

  if (listen(listeningSocket, 1) < 0)
  {
    printf("Error listening on socket : %d", errno);
    return -1;
  }

  struct sockaddr_in clientAddress;
  socklen_t clientAddressLength = sizeof(clientAddress);

  while (1)
  {

    printf("2.Waiting for connections\n");

    memset(&clientAddress, 0, sizeof(clientAddress));
    clientAddressLength = sizeof(clientAddress);

    int clientSocket = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);
    if (clientSocket < 0)
    {
      printf("Error accepting connection : %d", errno);
      return -1;
    }
    printf("  Connection accepted\n");

    while (flag)
    {
      if (setsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, "cubic", 6) < 0)
      {
        printf("Error setting socket options : %d", errno);
        return -1;
      }
      printf("  Congestion control changed to cubic\n");
      fseek(fp, 0L, SEEK_SET);
      // sending first part of the file to client

      int sent = send_file(fp, clientSocket, sz / 2, 0);
      printf("3.First part sent: %d\n", sent);

      //--checking key and sending second part of the file---
      printf("4.Waiting for KEY\n");
      int ans = send(clientSocket, "SEND ME A KEY", 14, 0);

      char buffer[10] = {0};
      int bytesReceived = recv(clientSocket, buffer, 10, 0);
      if (bytesReceived < 0)
      {
        printf("Error receiving message: %s, errno: %d", strerror(errno), errno);
      }
      else if (bytesReceived == 0)
      {
        printf("Connection closed by server\n");
      }
      else
      {
        printf("  Key received\n");
      }
      if (checkKey(buffer))
      {
        if (setsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, "reno", 5) < 0)
        {
          printf("Error setting socket options : %d", errno);
          return -1;
        }
        printf("5.Congestion control changed to reno\n");
        sent = send_file(fp, clientSocket, sz, sz / 2);
        printf("6.Second part sent: %d\n", sent);
      }
      int fnsh = send(clientSocket, "FINISHED", 9, 0);
      if (fnsh < 0)
      {
        perror("[-]Error in sending file.");
        exit(1);
      }
      else if (fnsh == 0)
      {
        printf("Connection closed by server\n");
        exit(1);
      }
      char msg[10] = {0};
      printf("7.Need to send again? (yes/*)\n");
      scanf("%s", msg);
      if (strcmp(msg, "yes") == 0)
      {
        flag = 1;
      }
      else
      {
        flag = 0;
        strcpy(msg, "Exit");
      }
      sent = send(clientSocket, msg, 10, 0);
      if (sent < 0)
      {
        perror("[-]Error in sending file.");
        exit(1);
      }
      else if (sent == 0)
      {
        printf("Connection closed by server\n");
        exit(1);
      }
      bzero(msg, 10);
      recv(clientSocket, msg, 10, 0);
      if (strcmp(msg, "OK"))
      {
        printf("%s\n", msg);
        perror("[-]ACK not received correctly");
        exit(1);
      }
    }
    printf("Closing connection\n");
    close(clientSocket);
    fclose(fp);
  }
  close(listeningSocket);

  return 0;
};

int send_file(FILE *fp, int sockfd, int size, int count)
{
  char data[BUFF_SIZE] = {0};
  char ansData[BUFF_SIZE] = {0};
  int bytesToRead = min(BUFF_SIZE, size - count);

  while (count < size && fread(data, bytesToRead, 1, fp) != NULL)
  {
    int sendSize = send(sockfd, data, bytesToRead, 0);
    if (sendSize < 0)
    {
      perror("[-]Error in sending file.");
      exit(1);
    }
    else if (sendSize == 0)
    {
      printf("Connection closed by server\n");
      exit(1);
    }

    int ans = recv(sockfd, ansData, bytesToRead, 0); // check ACK. in this case ack must be same as sent data
    if (ans < 0)
    {
      perror("[-]Error in receiving file.");
      exit(1);
    }
    else if (ans == 0)
    {
      printf("Connection closed by server\n");
      exit(1);
    }
    else if (memcmp(ansData, data, bytesToRead) != 0)
    {
      printf("[-]Error in sending file. Answer not the same as data.\n");
      exit(1);
    }
    else // everything is ok
    {
      count += bytesToRead;
    }

    bzero(data, BUFF_SIZE);
    bzero(ansData, BUFF_SIZE);
    bytesToRead = min(BUFF_SIZE, size - count); // check if we need to read less than BUFF_SIZE
  }
  return count;
}

int min(int a, int b)
{
  if (a < b)
  {
    return a;
  }
  else
  {
    return b;
  }
}

int checkKey(char *key)
{
  int myKey = 3498 ^ 420;
  char textKey[10];
  sprintf(textKey, "%d", myKey);
  if (strcmp(key, textKey) != 0)
  {
    printf("  Wrong key!\n");
    printf("  Key: %s\nmyKey: %s\n", key, textKey);
    return 0;
  }
  else
  {
    printf("  Correct key!\n");
    return 1;
  }
};

void printArray(char *arr, int size)
{
  for (int i = 0; i < size; i++)
  {
    printf("%c", arr[i]);
  }
  printf("\n");
}
