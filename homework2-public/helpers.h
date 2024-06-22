#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */

//trimisa de client udp la server
typedef struct structura_udp {

	char topic[50];
	uint8_t type;
	char buff[1501];
} structura_udp;

typedef struct topic{
	char name[51];
	int sf;
} topic;

//transmisa de server spre client tcp
typedef struct structura_tcp {
  char tip[11];
	char buffer[1501];
	char ipv4[16];
	uint16_t port;

	char topic[51];
} mesaj_tcp;

//trimis subscriber la server

// folosita in server.c si stocheaza datele despre un client (tcp sau udp)
typedef struct subscriber{
  struct topic topics[100];
	char id[10];
	int sock;
  	int unsent_length;
	struct structura_tcp unsent[100];
	int online;
	int topics_length;
	int port;
} subscriber;
struct chat_packet {
  uint16_t len;
  char message[MSG_MAXSIZE + 1];
  char tip;
  struct subscriber client;
};
#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#endif

