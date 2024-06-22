#include "common.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>

/*
    TODO 1.1: Rescrieți funcția de mai jos astfel încât ea să facă primirea
    a exact len octeți din buffer.
*/
int recv_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_received = 0, aux = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;
  char *ptr = buff;

    while(bytes_remaining) {
        aux = recv(sockfd, ptr, bytes_remaining, 0);

        bytes_received += aux;
        bytes_remaining -= aux;
        ptr += aux;
    }

  return len;
}

/*
    TODO 1.2: Rescrieți funcția de mai jos astfel încât ea să facă trimiterea
    a exact len octeți din buffer.
*/

int send_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_sent = 0, aux = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;
  char *ptr = buff;

    while(bytes_remaining) {
        aux = send(sockfd, ptr, bytes_remaining, 0);

        bytes_sent += aux;
        bytes_remaining -= aux;
        ptr += aux;
    }

  return len;
}
