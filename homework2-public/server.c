/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP
 * Echo Server
 * server.c
 */
//#include <math.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include "common.h"
#include "helpers.h"

#define MAX_CONNECTIONS 32
//#define IP_SERVER "192.168.0.2"
#define IP_SERVER "192.168.10.134"
struct subscriber client[100];
struct subscriber clienti_out[100];

struct sockaddr_in cli_addr;
socklen_t cli_len = sizeof(cli_addr);
int newsockfd = 0;
int udpsockfd = 0;

// Primeste date de pe connfd1 si trimite mesajul receptionat pe connfd2
int receive_and_send(int connfd1, int connfd2, size_t len)
{
    int bytes_received;
    char buffer[len];

    // Primim exact len octeti de la connfd1
    bytes_received = recv_all(connfd1, buffer, len);
    // S-a inchis conexiunea
    if (bytes_received == 0)
    {
        return 0;
    }
    DIE(bytes_received < 0, "recv");

    // Trimitem mesajul catre connfd2
    int rc = send_all(connfd2, buffer, len);
    if (rc <= 0)
    {
        perror("send_all");
        return -1;
    }

    return bytes_received;
}

void run_chat_server(int listenfd)
{
    struct sockaddr_in client_addr1;
    struct sockaddr_in client_addr2;
    socklen_t clen1 = sizeof(client_addr1);
    socklen_t clen2 = sizeof(client_addr2);

    int connfd1 = -1;
    int connfd2 = -1;
    int rc;

    // Setam socket-ul listenfd pentru ascultare
    rc = listen(listenfd, 2);
    DIE(rc < 0, "listen");

    // Acceptam doua conexiuni
    printf("Astept conectarea primului client...\n");
    connfd1 = accept(listenfd, (struct sockaddr *)&client_addr1, &clen1);
    DIE(connfd1 < 0, "accept");

    printf("Astept connectarea clientului 2...\n");
    connfd2 = accept(listenfd, (struct sockaddr *)&client_addr2, &clen2);
    DIE(connfd2 < 0, "accept");

    while (1)
    {
        printf("Primesc de la 1 si trimit catre 2...\n");
        int rc = receive_and_send(connfd1, connfd2, sizeof(struct chat_packet));
        if (rc <= 0)
        {
            break;
        }

        printf("Primesc de la 2 si trimit catre 1...\n");
        rc = receive_and_send(connfd2, connfd1, sizeof(struct chat_packet));
        if (rc <= 0)
        {
            break;
        }
    }

    // Inchidem conexiunile si socketii creati
    close(connfd1);
    close(connfd2);
}

int putere( int base, int exponent) //implementare pow
{ //aceasta functie e pentru tipul float, care are o putere de 10
    long long int result = 1;
    for (unsigned int i = 0; i < exponent; ++i)
    {
        result *= base;
    }
    return result;
}
void run_chat_multi_server(int listenfd, int port, int udp_sock)
{
    //functia de baza din laborator
    struct pollfd poll_fds[MAX_CONNECTIONS];
    int num_sockets = 10;
    int rc;
    char buf[1600];
    struct chat_packet received_packet; //pachet
    //printf("%d\n",port);
    // Setam socket-ul listenfd pentru ascultare
    rc = listen(listenfd, MAX_CONNECTIONS);
    DIE(rc < 0, "listen");
    // Adaugam noul file descriptor (socketul pe care se asculta conexiuni) in
    // multimea poll_fds
    poll_fds[0].fd = listenfd; //listenfd este pentru clienti tcp
    poll_fds[0].events = POLLIN;
    poll_fds[2].fd = 0;
    poll_fds[2].events= POLLIN;
    int timerfd;
    timerfd = timerfd_create(0,  0); // CLOCK_REALTIME = 0

    struct itimerspec spec;
    spec.it_value.tv_sec = 0;
    spec.it_value.tv_nsec = 100000; ///imi setez un clk de 100 us
    spec.it_interval.tv_sec = 0;
    spec.it_interval.tv_nsec = 100000;
    timerfd_settime(timerfd, 0, &spec, NULL);
    poll_fds[3].fd = udp_sock; //pun si socketii udp
    poll_fds[3].events = POLLIN;
    poll_fds[1].fd = timerfd;
    poll_fds[1].events = POLLIN; //setez timerfd
    num_sockets = 4;

    while (1)
    {
        // Asteptam sa primim ceva pe unul dintre cei num_sockets socketi
        rc = poll(poll_fds, num_sockets, -1 );
        DIE(rc < 0, "poll");
        for (int i = 0; i < num_sockets; i++) //parcurg toti socketii
        {
            if (poll_fds[i].revents & POLLIN)
            {
                if(poll_fds[i].fd == 0)
                {
                    fgets(buf,sizeof(buf),stdin); //citesc input de la tastatura
                    if(strncmp(buf,"exit",4) == 0)
                    {
                        for (int i = 4; i < num_sockets; i++)
                        { //de la pozitia 4 pana la num_sockets sunt clienti tcp
                            struct chat_packet pachet_nou;
                            strcpy(pachet_nou.message, "exit server"); //daca serverul da exit, ies si clientii
                            send_all(poll_fds[i].fd, &pachet_nou, sizeof(pachet_nou));
                        }
                        for (int i = 0; i < num_sockets; i++)
                        { //dau close la poll_fds (toti socketii)
                            close(poll_fds[i].fd);
                        }

                        exit(0);
                    }
                }
                else if (poll_fds[i].fd == listenfd)
                {
                    // Am primit o cerere de conexiune pe socketul de listen, pe care
                    // o acceptam
                    newsockfd = accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
                    DIE(newsockfd < 0, "accept");

                    // Adaugam noul socket intors de accept() la multimea descriptorilor
                    // de citire
                    poll_fds[num_sockets].fd = newsockfd;
                    poll_fds[num_sockets].events = POLLIN;
                    num_sockets++;
                }
                else if (poll_fds[i].fd == udp_sock)
                {
                    double real;
                    struct chat_packet pachet; //daca clientul este udp
                    struct sockaddr_in from_sock;
                    char buf[1600];
                    memset(buf, 0, 1600);
                    struct sockaddr *sock_addr = (struct sockaddr *)&from_sock;
                    socklen_t sock_len = sizeof(from_sock);
                    udpsockfd = recvfrom(udp_sock, buf, 1551, 0, sock_addr, &sock_len);
                    //ma conectez la clientul udp folosind recvfrom
                    DIE(udpsockfd < 0, "accept");;
                    int numar = 0;
                    char buf2[1650]; //in buf2 retin mesajul pe care il voi afisa in clientii TCP
                    if(buf[50] == 0) //cazul INT
                    {
                        int8_t semn = (int8_t)buf[51];
                        uint32_t nr_retea;
                        memcpy(&nr_retea, &buf[52], sizeof(uint32_t));
                        nr_retea = ntohl(nr_retea); //numarul era in byte order
                        if(semn == 1)
                        {
                            numar=nr_retea*(-1); //inmultesc cu -1 pentru octetul de semn
                        }
                        else
                        {
                            numar= nr_retea;
                        }
                        pachet.tip=0; //pachetul are tip 0, ca in cerinta
                        sprintf(buf2, "%s - INT - %d", buf,numar);
                    }
                    if(buf[50] == 1) //cazul SHORT_REAL
                    {
                        uint16_t numar_real;
                        memcpy(&numar_real, &buf[51], sizeof(uint16_t));
                        numar_real=ntohs(numar_real);
                        real=(double)numar_real /(double) 100;
                        sprintf(buf2, "%s - SHORT_REAL - %.2lf", buf,real);
                        //il afisez in forma de nr real cu 2 zecimale
                        pachet.tip=1; //tip 1
                    }
                    if(buf[50] == 2) //cazul float
                    {
                        pachet.tip=2;
                        uint32_t nr_float;
                        memcpy(&nr_float,&buf[52],sizeof(uint32_t)); //in buf[52], am nr fara zecimale
                        uint8_t power;
                        memcpy(&power, &buf[56],sizeof(uint8_t)); //in buf[56], se afla puterea lui 10
                        uint8_t semn;
                        memcpy(&semn,&buf[51],sizeof(uint8_t)); //aici este semnul numarului
                        nr_float = ntohl(nr_float); //byte order
                        real=(double)nr_float/(double)putere(10,power);
                        if(semn == 1)
                            real=real * (-1); //inmultesc cu -1 pentru semn
                        sprintf(buf2, "%s - FLOAT - %.8g", buf,real);
                    }
                    if(buf[50] == 3) //cazul STRING
                    {
                        pachet.tip=3; //la string, nu prea sunt prelucrari de facit
                        sprintf(buf2, "%s - STRING - %s", buf,buf+51);
                    }
                    strcpy(pachet.message, buf2);
                    for (int j = 4; j < num_sockets; j++) //trimit mesajele clientilor TCP
                    {
                        rc = send_all(poll_fds[j].fd, &pachet, sizeof(pachet));
                    }
                }
                else if (poll_fds[i].fd == timerfd)
                {
                    uint64_t expirations;
                    DIE(read(timerfd, &expirations, sizeof(expirations)) == -1, "read");
                }
                else
                {
                    // Am primit date pe unul din socketii de client, asa ca le receptionam
                    int rc = recv_all(poll_fds[i].fd, &received_packet,
                                      sizeof(received_packet));
                    DIE(rc < 0, "recv");

                    if (rc == 0)
                    { //daca rc este 0, nu mai am conexiune pe acel client
                        close(poll_fds[i].fd);
                        for (int j = i; j < num_sockets - 1; j++)
                        {
                            poll_fds[j] = poll_fds[j + 1]; //il scot din lista de socketi
                        }

                        num_sockets--; //scade nr de socketi
                    }
                    else if (strncmp("Un nou client s-a conectat cu id", received_packet.message, 32) == 0)
                    { //daca primesc de la client mesaj de conectare, incepe cu aceasta adresare.
                        received_packet.tip=10;
                        char *id_start = strstr(received_packet.message, "Un nou client s-a conectat cu id ");
                        if (id_start != NULL)
                        {
                            id_start += strlen("Un nou client s-a conectat cu id ");
                            //extrag id-ul clientului nou conectat
                            // Verificam daca ID-ul este deja prezent în lista
                            int id_already_exists = 0;
                            for (int j = 0; j < num_sockets; j++)
                            {
                                if (strcmp(client[j].id, id_start) == 0)
                                {
                                    id_already_exists = 1; //exista deja un client cu acel id
                                    break;
                                }
                            }

                            if (!id_already_exists)
                            { //daca nu este niciun client conectat cu acel id
                                strcpy(client[num_sockets - 1].id, id_start);
                                int t=0;
                                while(strlen(clienti_out[t].id)!=0 && strcmp(clienti_out[t].id, client[num_sockets-1].id)!=0)
                                    t++; //aici caut in lista de clienti deconectati
                                struct chat_packet pachet;
                                int z=0;
                                if(strcmp(clienti_out[t].id, client[num_sockets-1].id) == 0)
                                { //daca clientul a mai fost inainte, trimit un mesaj clientului cu 
                                  //topicurile la care afost abonat
                                    strcpy(pachet.message, "subscribe_dupa_deconectare");
                                    while(strlen(clienti_out[t].topics[z].name)!=0)
                                    {
                                        strcat(pachet.message, " ");
                                        strcat(pachet.message, clienti_out[t].topics[z].name);
                                        z++; //concatenez topicurile clientului nou
                                    }
                                    send_all(poll_fds[i].fd, &pachet, sizeof(pachet)); //ii trimit mesaj
                                }
                                printf("New client %s connected from %s:%d.\n",  client[num_sockets - 1].id,
                                       inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

                                //afisez in server ca am un nou client
                            }
                            else
                            {
                                printf("Client %s already connected.\n",id_start); //clientul este deja conectat
                                struct chat_packet pachet_nou;
                                strcpy(pachet_nou.message, "exit client duplicat");
                                send_all(poll_fds[i].fd, &pachet_nou, sizeof(pachet_nou));
                                close(poll_fds[i].fd); //trimit mesaj clientului sa dea exit si inchid conexiunea
                                // Scoatem socket-ul de la indexul 'i' din multimea de descriptori
                                for (int j = i; j < num_sockets; j++)
                                {
                                    poll_fds[j] = poll_fds[j + 1];
                                }
                                num_sockets--;
                            }
                        }
                    }
                    if (strncmp(received_packet.message, "subscribe", 9) == 0)
                    {
                        // Cauta pozitia primei aparitii a sirului "subscribe"
                        received_packet.tip=11;
                        char *subscribe_position = strstr(received_packet.message, "subscribe");
                        //printf("TRECE\n");
                        if (subscribe_position != NULL)
                        {
                            // Avanseaza pointer-ul pentru a ignora "subscribe" si spatiile care urmeaza imediat dupa acesta
                            char *topic_start = subscribe_position + strlen("subscribe");
                            while (*topic_start == ' ')
                            {
                                topic_start++; // Avanseaza peste spatii
                            }
                            int y=4; //caut in socketii de la 4 la num_sockets
                            int x=0;
                            while(strcmp(client[y].id,received_packet.client.id) !=0 && strlen(client[y].id) != 0)
                                y++;
                            while(strlen(client[y].topics[x].name) != 0)
                                x++;
                            strcpy (client[y].topics[x].name, topic_start); //salvez topicul clientului
                        }
                    }
                    else
                    {
                        if (strncmp(received_packet.message, "exit",4) == 0)
                        { //aici tratez cazul cand primesc exit de la un client
                            received_packet.tip=12;
                            // Inchidem conexiunea cu clientul
                            printf("Client %s disconnected.\n", client[i].id);
                            char *remaining_message = received_packet.message + 5; //extrag ce a trimis clientul dupa exit
                            // Despartim mesajul in cuvinte si salvam fiecare parte in client.topics[x].name
                            char *token = strtok(remaining_message, " "); //fac un strtok pentru a vedea topicurile
                            int topic_index = 0;
                            int z=0;
                            while(strlen(clienti_out[z].id)!=0)
                                z++;
                            strcpy(clienti_out[z].id, client[i].id);
                            while (token != NULL && topic_index < 50)
                            {
                                // Copiem cuvantul in client.topics[x].name
                                strcpy(client[i].topics[topic_index].name, token);
                                strcpy(clienti_out[z].topics[topic_index].name, token);
                                topic_index++;

                                // Trecem la urmatorul cuvant
                                token = strtok(NULL, " ");
                            }
                            close(poll_fds[i].fd);
                            // Scoatem socket-ul clientului inchis din multimea de descriptori
                            for (int k = i; k < num_sockets; k++)
                            {
                                poll_fds[k] = poll_fds[k + 1];
                            }
                            client[i].online=0;
                            for (int k = i; k < num_sockets; k++)
                            {
                                strcpy(client[k].id, client[k + 1].id);
                            }
                            num_sockets--;
                            continue; // Trecem la urmatoarea iteratie a buclei fara a trimite mesajul catre ceilalti clienti
                        }
                        // Trimitem mesajul catre ceilalti clienti
                        for (int j = 4; j < num_sockets; j++)
                        {
                            rc = send_all(poll_fds[j].fd, &received_packet, sizeof(received_packet));
                        }
                    }
                }
            }
        }
    }
}
int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    if (argc != 2) //vreau 2 argumente
    {
        printf("\n Usage: %s <port>\n", argv[0]);
        return 1;
    }
    // Parsam port-ul ca un numar
    uint16_t port;
    port = atoi(argv[1]);
    int rc;

    // Obtinem un socket TCP pentru receptionarea conexiunilor
    const int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(listenfd < 0, "socket");

    // Completam in serv_addr adresa serverului, familia de adrese si portul
    // pentru conectare
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    // Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
    // rulam de 2 ori rapid
    // Vezi https://stackoverflow.com/questions/3229860/what-is-the-meaning-of-so-reuseaddr-setsockopt-option-linux
    const int enable = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    rc = inet_pton(AF_INET, IP_SERVER, &serv_addr.sin_addr.s_addr);
    DIE(rc <= 0, "inet_pton");

    // Asociem adresa serverului cu socketul creat folosind bind
    rc = bind(listenfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "bind");

    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0); //creez un socket udp
    DIE(udp_sock < 0, "problema socket UDP");

    struct sockaddr_in udp_add;
    udp_add.sin_family = AF_INET;
    udp_add.sin_port = htons(port); //pun portul
    udp_add.sin_addr.s_addr = INADDR_ANY;
    int ret = bind(udp_sock, (struct sockaddr *)&udp_add, sizeof(udp_add));
    DIE(ret < 0, "bind UDP");

    run_chat_multi_server(listenfd, port,udp_sock);

    // Inchidem listenfd
    close(listenfd);
    //inchidem si socketul de clienti udp
    close(udp_sock);

    return 0;
}
