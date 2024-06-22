# Protocoale de comunicatii
# Laborator 7 - TCP
# Echo Server
# Makefile

CFLAGS = -Wall -g -Wno-error=unused-variable

# Portul pe care asculta serverul
PORT = 30000

# Adresa IP a serverului
IP_SERVER = 192.168.0.2

all: server subscriber

common.o: common.c

# Compileaza server.c
server: server.c common.o

# Compileaza client.c
client: subscriber.c common.o

.PHONY: clean run_server run_client

# Ruleaza serverul
run_server:
	./server ${IP_SERVER} ${PORT}

# Ruleaza clientul
run_client:
	./subscriber ${IP_SERVER} ${PORT}

clean:
	rm -rf *.o *.dSYM
