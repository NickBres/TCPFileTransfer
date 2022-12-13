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

#define SERVER_PORT 5300
#define BUFF_SIZE 1024
#define CC_1 "cubic"
#define CC_2 "reno"

int main()
{
  //----------------Read file--------------------
  printf("1.Read file\n");
  FILE *fp = fopen("./book.txt", "r"); // open file
  if (fp == NULL)
  {
    printf("Error opening file\n");
    return -1;
  }
  fseek(fp, 0L, SEEK_END); // go to the end of the file
  int sz = ftell(fp); // get the size of the file
  printf("  File size: %d\n", sz); 
  int flag = 1;

  //----------------------------------------------

  signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE

  int listeningSocket = -1;

  if ((listeningSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) // create listen socket
  {
    printf("Error creating socket : %d", errno);
    return -1;
  }

  int enableReuse = 1;
  if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0) // set socket options
  {
    printf("Error setting socket options : %d", errno);
    return -1;
  }

  struct sockaddr_in serverAddress;
  memset(&serverAddress, 0, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  serverAddress.sin_port = htons(SERVER_PORT);

  if (bind(listeningSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // bind socket
  {
    printf("Error binding socket : %d", errno);
    return -1;
  }
  // printf("Binding successful\n");

  if (listen(listeningSocket, 1) < 0) // listen on socket
  {
    printf("Error listening on socket : %d", errno);
    return -1;
  }

  struct sockaddr_in clientAddress;
  socklen_t clientAddressLength = sizeof(clientAddress);

  while (1) // accept connections
  {

    printf("2.Waiting for connections\n");

    memset(&clientAddress, 0, sizeof(clientAddress));
    clientAddressLength = sizeof(clientAddress);

    int clientSocket = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLength); // accept connection
    if (clientSocket < 0)
    {
      printf("Error accepting connection : %d", errno);
      return -1;
    }
    printf("  Connection accepted\n");

    while (flag) // send file until user deciedes to stop
    {
      if (setsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, CC_1, 6) < 0) // set congestion control
      {
        printf("Error setting socket options : %d", errno);
        return -1;
      }
      printf("  Congestion control changed to %s\n", CC_1);
      fseek(fp, 0L, SEEK_SET); // go to the beginning of the file
   

      int sent = send_file(fp, clientSocket, sz / 2, 0); // sending first part of the file to client
      printf("3.First part sent: %d\n", sent);

      //--checking key and send second part of the file---
      printf("4.Waiting for KEY\n");
      int ans = send(clientSocket, "SEND ME A KEY", 14, 0); // send key request

      char buffer[10] = {0};
      int bytesReceived = recv(clientSocket, buffer, 10, 0); // receive key
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
      if (checkKey(buffer)) // check key
      {
        if (setsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, CC_2, 5) < 0) // set congestion control
        {
          printf("Error setting socket options : %d", errno);
          return -1;
        }
        printf("5.Congestion control changed to %s\n", CC_2);
        sent = send_file(fp, clientSocket, sz, sz / 2); // send second part of the file
        printf("6.Second part sent: %d\n", sent);
      }
      int fnsh = send(clientSocket, "FINISHED", 9, 0); //send message that all file is sent
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
      printf("7.Need to send again? (yes/*)\n"); // ask user if he wants to send file again
      scanf("%s", msg);
      if (strcmp(msg, "yes") == 0) // if yes, send OK message
      {
        flag = 1;
      }
      else // if not, send exit message
      {
        flag = 0;
        strcpy(msg, "Exit");
      }
      sent = send(clientSocket, msg, 10,  0); // send message
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
      bzero(msg, 10); // clear buffer
      recv(clientSocket, msg, 10, 0); // receive ack
      if (strcmp(msg, "OK"))
      {
        printf("%s\n", msg);
        perror("[-]ACK not received correctly");
        exit(1);
      }
    }
    printf("Closing connection\n");
    close(clientSocket); // close connection
    fclose(fp); // close file
  }
  close(listeningSocket); // close socket
  return 0;
};

int send_file(FILE *fp, int sockfd, int size, int count)
{
  char data[BUFF_SIZE] = {0}; // buffer for data
  char ansData[BUFF_SIZE] = {0};  // buffer for ack
  int bytesToRead = min(BUFF_SIZE, size - count); // bytes to read. if size of file is less than buffer size, read only size of file

  while (count < size && fread(data, bytesToRead, 1, fp) != NULL) // run until all file is sent
  {
    int sendSize = send(sockfd, data, bytesToRead, 0); // send file data
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
    else if (memcmp(ansData, data, bytesToRead) != 0) // if ack is not the same as data, exit
    {
      printf("[-]Error in sending file. Answer not the same as data.\n");
      exit(1);
    }
    else // everything is ok
    {
      count += bytesToRead;
    }

    bzero(data, BUFF_SIZE); // clear buffers
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
  sprintf(textKey, "%d", myKey); // convert int to string
  if (strcmp(key, textKey) != 0) // compare keys
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
