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
#define BUFF_SIZE 32768
#define CC_1 "cubic"
#define CC_2 "reno"
#define TRUE 1
#define FALSE 0

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
  int sz = ftell(fp);      // get the size of the file
  printf("  File size: %d\n", sz);
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

  if (listen(listeningSocket, 1) < 0) // listen on socket
  {
    printf("Error listening on socket : %d", errno);
    return -1;
  }

  struct sockaddr_in clientAddress;
  socklen_t clientAddressLength = sizeof(clientAddress);

  int flag = TRUE;
  while (TRUE) // accept connections
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
      char smallBuffer[10] = {0}; // buffer to accept and send small messages
      fseek(fp, 0L, SEEK_SET);    // go to the beginning of the file

      ccChange(clientSocket, CC_1);                      // change congestion control
      int sent = send_file(fp, clientSocket, sz / 2, 0); // sending first part of the file to client
      printf("3.First part sent: %d\n", sent);

      //----------------checking key and send second part of the file----------------
      printf("4.Waiting for KEY\n");
      int ans = send(clientSocket, "SEND ME A KEY", 14, 0); // send key request

      int bytesReceived = recv(clientSocket, smallBuffer, 10, 0); // receive key
      if (bytesReceived < 0)
      {
        printf("Error receiving message: %s, errno: %d", strerror(errno), errno);
      }
      else if (bytesReceived == 0)
      {
        printf("Connection closed by server\n");
      }
      if (checkKey(smallBuffer)) // check key
      {
        ccChange(clientSocket, CC_2);                   // change congestion control
        sent = send_file(fp, clientSocket, sz, sz / 2); // send second part of the file
        printf("6.Second part sent: %d\n", sent);
      }

      int fnsh = send(clientSocket, "FINISHED", 9, 0); // send message that all file is sent
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
      bzero(smallBuffer, 10);
      bytesReceived = recv(clientSocket, smallBuffer, 3, 0); // receive ack
      if (strcmp(smallBuffer, "OK") != 0)
      {
        printf("  ack is wrong\n");
        exit(1);
      }
      //--------------------------------------------------------------------------------
      printf("7.Need to send again? (yes/*)\n"); // ask user if he wants to send file again
      scanf("%s", smallBuffer);
      if (strcmp(smallBuffer, "yes") == 0) // if yes, send OK message
      {
        flag = TRUE;
      }
      else // if not, send exit message
      {
        flag = 0;
        strcpy(smallBuffer, "Exit");
      }
      sent = send(clientSocket, smallBuffer, 10, 0); // send message
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
      bzero(smallBuffer, 10);                 // clear buffer
      recv(clientSocket, smallBuffer, 10, 0); // receive ack
      if (strcmp(smallBuffer, "OK"))
      {
        printf("%s\n", smallBuffer);
        perror("[-]ACK not received correctly");
        exit(1);
      }
    }
    printf("Closing connection\n");
    close(clientSocket); // close connection
    fclose(fp);          // close file
  }
  close(listeningSocket); // close socket
  return 0;
};

int send_file(FILE *fp, int sockfd, int size, int count)
{
  char data[BUFF_SIZE] = {0};                     // buffer for data
  char ansData[BUFF_SIZE] = {0};                  // buffer for ack
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

void ccChange(int sockfd, char *CC)
{

  char buffer[10] = {0};
  int check = send(sockfd, "CC", 3, 0); // send CC
  if (check < 0)
  {
    perror("[-]Error in sending file.");
    exit(1);
  }
  else if (check == 0)
  {
    printf("Connection closed by server\n");
    exit(1);
  }
  check = recv(sockfd, buffer, 3, 0); // receive ack
  if (strcmp(buffer, "OK") != 0)      // if ack is not OK, exit
  {
    printf("  ack is wrong\n");
    exit(1);
  }
  bzero(buffer, 10);
  strcpy(buffer, CC); // copy CC name to buffer
  int len = strlen(buffer);

  check = send(sockfd, buffer, len, 0); // send CC
  if (check < 0)
  {
    perror("[-]Error in sending file.");
    exit(1);
  }
  else if (check == 0)
  {
    printf("Connection closed by server\n");
    exit(1);
  }
  char ack[3] = {0};
  int bytesReceived = recv(sockfd, ack, 3, 0); // receive ack
  if (strcmp(ack, "OK") != 0)                  // if ack is not OK, exit
  {
    printf("  ack is wrong\n");
    exit(1);
  }
  
  if (setsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, buffer, len) < 0) // change CC
  {
    perror("[-]Error in setting socket options");
    exit(1);
  }
    printf("  CC changed to %s\n", buffer);
};
