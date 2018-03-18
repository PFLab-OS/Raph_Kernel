#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

#define ECHOMAX 255 /* Longest string to echo */

void DieWithError(const char *errorMessage) {
  perror(errorMessage);
  exit(1);
}

int main(int argc, char *argv[]) {
  int sock;                        /* Socket */
  struct sockaddr_in echoServAddr; /* Local address */
  struct sockaddr_in echoClntAddr; /* Client address */
  unsigned int cliAddrLen;         /* Length of incoming message */
  char echoBuffer[ECHOMAX];        /* Buffer for echo string */
  unsigned short echoServPort;     /* Server port */
  int recvMsgSize;                 /* Size of received message */

  echoServPort = 5621;

  /* Create socket for sending/receiving datagrams */
  if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    DieWithError("socket() failed");

  /* Construct local address structure */
  memset(&echoServAddr, 0, sizeof(echoServAddr)); /* Zero out structure */
  echoServAddr.sin_family = AF_INET;              /* Internet address family */
  echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
  echoServAddr.sin_port = htons(echoServPort);      /* Local port */

  /* Bind to the local address */
  if (bind(sock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
    DieWithError("bind() failed");

  for (;;) /* Run forever */
  {
    /* Set the size of the in-out parameter */
    cliAddrLen = sizeof(echoClntAddr);

    /* Block until receive message from a client */
    if ((recvMsgSize =
             recvfrom(sock, echoBuffer, ECHOMAX, 0,
                      (struct sockaddr *)&echoClntAddr, &cliAddrLen)) < 0)
      DieWithError("recvfrom() failed");

    printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));
    /*
    if (connect(sock, (struct sockaddr *)&echoClntAddr, sizeof(echoClntAddr)))
      DieWithError("connect() failed");
      */
    /* Send received datagram back to the client */
    if (sendto(sock, echoBuffer, recvMsgSize, 0,
               (struct sockaddr *)&echoClntAddr,
               sizeof(echoClntAddr)) != recvMsgSize)
      DieWithError("sendto() sent a different number of bytes than expected");
  }
  /* NOT REACHED */
}
