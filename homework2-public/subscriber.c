/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP si mulplixare
 * client.c
 */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "helpers.h"
#include "common.c"
struct subscriber client;
void trim(char *s)
{
    int len = strlen(s);
    char *start = s;
    char *end = s + len - 1;

    // Elimina spatiile de la inceputul sirului
    while (isspace(*start))
    {
        start++;
    }

    // Elimina spatiile de la sfarsitul sirului
    while ((end >= start) && isspace(*end))
    {
        *end = '\0';
        end--;
    }

    // Rearanjeaza sirul pentru a elimina spatiile
    if (start > s)
    {
        memmove(s, start, (end - start) + 2);
    }
}
char **split_string(const char *str)
{
    // Alocam initial un spatiu pentru matricea de tokenuri
    char **tokens = (char **)malloc(50 * sizeof(char *));
    if (tokens == NULL)
    {
        printf("Nu s-a putut aloca memorie pentru tokenuri.\n");
        exit(EXIT_FAILURE);
    }

    int index = 0; // Indexul pentru tokenul curent
    int start = 0; // Indexul de start al atomului lexical curent

    // Parcurgem sirul caracter cu caracter
    for (int i = 0; str[i] != '\0'; i++)
    {
        // Daca intalnim caracterul '/', inseamna ca am gasit un atom lexical
        if (str[i] == '/')
        {
            // Alocam spatiu pentru atomul lexical curent si il copiem in matricea de tokenuri
            tokens[index] = (char *)malloc((i - start + 1) * sizeof(char));
            strncpy(tokens[index], str + start, i - start);
            tokens[index][i - start] = '\0'; // Adaugam terminatorul de sir
            index++; // Trecem la urmatorul token
            start = i + 1; // Actualizam indexul de start pentru urmatorul atom lexical
        }
    }

    // Alocam spatiu pentru ultimul atom lexical si îl copiem în matricea de tokenuri
    tokens[index] = (char *)malloc((strlen(str) - start + 1) * sizeof(char));
    strcpy(tokens[index], str + start);

    // Adaugam un element NULL la sfarsitul listei de tokenuri
    tokens[index + 1] = NULL;

    // Returnam matricea de tokenuri
    return tokens;
}


void run_client(int sockfd)
{
    //salvez caracterinsticile clientului
    client.online = 1;
    client.sock = sockfd;
    client.topics_length = 0;
    char buf[MSG_MAXSIZE + 1];
    struct chat_packet sent_packet;
    struct chat_packet recv_packet;
    memset(buf, 0, MSG_MAXSIZE + 1);
    sprintf(sent_packet.message, "Un nou client s-a conectat cu id %s", client.id);
    //trimit pachet catre server ca am conectat un nou client
    sent_packet.client = client; //la pachet atasez si clientul cu toate caracteristicile
    send_all(sockfd, &sent_packet, sizeof(sent_packet));
    memset(buf, 0, MSG_MAXSIZE + 1);
    memset(sent_packet.message, 0, MSG_MAXSIZE + 1);
    struct pollfd poll_fds[2];
    int num_sockets = 2; //salvez numarul initial de socketi, unul pentru citire 
    //si altul pentru comunicare cu severul
    int rc;

    poll_fds[0].fd = 0;
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = sockfd;
    poll_fds[1].events = POLLIN;


    while (1)
    {
        // Asteptam sa primim ceva pe unul dintre cei num_sockets socketi
        rc = poll(poll_fds, 2, -1);
        DIE(rc < 0, "poll");

        for (int i = 0; i < num_sockets; i++)
        {
            if (poll_fds[i].revents & POLLIN)
            {
                client.online = 1;
                if (poll_fds[i].fd == 0) //daca este 0, astept sa primesc input de la tastatura
                {
                    fgets(buf, sizeof(buf), stdin); //citesc inputul
                    sent_packet.len = strlen(buf) + 1;
                    strcpy(sent_packet.message, buf);
                    if(strncmp(buf, "exit",4) == 0) //daca clientul da exit
                    {
                        sent_packet.tip = 'e';
                        // Concatenam toate topicurile clientului în buf
                        strcat(buf, " "); // Adaugam un spatiu între mesaj si topicuri pentru a le separa
                        for (int j = 0; j < client.topics_length; j++)
                        {
                            strcat(buf, client.topics[j].name);
                            strcat(buf, " "); // Adaugam un spatiu între fiecare topic
                        }
                        sent_packet.len = strlen(buf) + 1;
                        strcpy(sent_packet.message, buf);
                        if(send_all(sockfd, &sent_packet, sizeof(sent_packet)) < 0)
                        {
                            perror("[SUBSCRIBER] Unable to send exit message\n");
                            exit(0);
                        } //trimit un pachet cu exit si toate topicurile clientului
                        client.online = 0;
                        exit(0);
                    }
                    else if (strncmp(buf, "subscribe", 9) == 0) //daca clientul da subscribe
                    {
                        char *subscribe_ptr = strstr(buf, "subscribe ");
                        if (subscribe_ptr != NULL)
                        {
                            // Avansam pointer-ul pana la sfarsitul comenzii "subscribe"
                            subscribe_ptr += strlen("subscribe ");

                            // Extragem restul datelor din buffer dupa comanda "subscribe"
                            char rest_of_buffer[51]; // BUFSIZE este dimensiunea buffer-ului
                            strcpy(rest_of_buffer, subscribe_ptr);
                            size_t newline_index = strcspn(rest_of_buffer, "\n");
                            if (newline_index < sizeof(rest_of_buffer))
                            {
                                rest_of_buffer[newline_index] = '\0';
                            }
                            int x=0;
                            while(x < client.topics_length && strcmp(rest_of_buffer, client.topics[x].name) != 0)
                                x++; //aici caut topicul respectiv daca nu i-am dat deja subscribe
                            if(x < client.topics_length)
                            {  //daca am deja topicul, nu mai dau o data subscribe
                                printf("Already subscribed to topic %s\n", client.topics[x].name);
                            }
                            else
                            { //daca nu am dat subscribe la topic, trimit mesaj catre server
                                client.topics_length++;
                                strcpy(client.topics[x].name, rest_of_buffer);
                                sent_packet.client = client;
                                send_all(sockfd, &sent_packet, sizeof(sent_packet));
                                printf("Subscribed to topic %s\n", rest_of_buffer);
                            }
                        }
                    }
                    else if (strncmp(buf, "unsubscribe", 11) == 0)
                    {
                        sent_packet.client = client;
                        char *unsubscribe_ptr = strstr(buf, "unsubscribe ");
                        if (unsubscribe_ptr != NULL)
                        {
                            // Avansam pointer-ul pana la sfarsitul comenzii "unsubscribe"
                            unsubscribe_ptr += strlen("unsubscribe ");
                            // Extragem restul datelor din buffer dupa comanda "unsubscribe"
                            char rest_of_buffer[51]; // Aici poti ajusta dimensiunea în functie de nevoi
                            strcpy(rest_of_buffer, unsubscribe_ptr);
                            size_t newline_index = strcspn(rest_of_buffer, "\n");
                            if (newline_index < sizeof(rest_of_buffer))
                            {
                                rest_of_buffer[newline_index] = '\0';
                            }
                            int x = 0;
                            while(strlen (client.topics[x].name) != 0 && strcmp(client.topics[x].name, rest_of_buffer) != 0)
                                x++;
                            if(strlen (client.topics[x].name) == 0) //daca nu exista topicul
                                printf("Topic %s doesn't exist\n", rest_of_buffer);
                            else
                            { //daca gasesc topicul, dau unsubscribe
                                printf("Unsubscribed from topic %s\n", rest_of_buffer);
                                client.topics_length--;
                                x++;
                                while(strlen(client.topics[x].name) != 0)
                                {
                                    strcpy(client.topics[x-1].name, client.topics[x].name);
                                    x++; //elimin topicul
                                }
                                send_all(sockfd, &sent_packet, sizeof(sent_packet));
                            }
                        }
                    }
                    else
                    {
                        // Trimitem pachetul la server.
                        sent_packet.client = client;
                        send_all(sockfd, &sent_packet, sizeof(sent_packet));
                    }
                }
                else
                {
                    // Am primit date pe unul din socketii de client, asa ca le receptionam
                    int rc = recv_all(poll_fds[i].fd, &recv_packet,
                                      sizeof(recv_packet));
                    DIE(rc < 0, "recv");
                    if(strncmp(recv_packet.message, "subscribe_dupa_deconectare",26)==0)
                    {
                        //daca dau subscribe pe acelasi client dupa deconectare
                        char *token;
                        char *delimiter = " ";
                        char words[300][51]; // Ajusteaza dimensiunea vectorului conform nevoilor tale
                        int word_count = 0;
                        token = strtok(recv_packet.message + 27, delimiter);

                        // Iteram prin mesaj si salvam fiecare cuvant în vector
                        while (token != NULL && word_count < 100)
                        {
                            strcpy(words[word_count++], token);
                            token = strtok(NULL, delimiter);
                        }

                        // Afisam fiecare cuvant separat
                        for (int x = 0; x < word_count; x++)
                        {
                            strcpy(client.topics[x].name, words[x]);
                            client.topics_length++; //salvez topicuri in clientul nou
                        }
                    }
                    if(strncmp(recv_packet.message, "exit client duplicat",21) == 0)
                    {
                        printf("Client %s already connected.\n",client.id);
                        exit(0); //daca clientul este duplicat, ma deconectez
                    }
                    if(strncmp(recv_packet.message,"exit server",11) == 0)
                    {
                        printf("Am inchis serverul!\n"); //daca se inchide serverul, inchid si clientii
                        exit(0);
                    }
                    if(strncmp(recv_packet.message, "unsubscribe", 11) != 0 && strncmp(recv_packet.message, "Un nou client s-a conectat cu id", 31)!=0
                     && strncmp(recv_packet.message,"subscribe_dupa_deconectare",26)!=0)
                    {
                        char copy_of_message[1600];
                        strcpy(copy_of_message, recv_packet.message); // Copiaza sirul original în copie

                        char *separator_position = strchr(copy_of_message, '-');

                        if (separator_position != NULL)
                        {
                            *separator_position = '\0'; // Taie sirul înainte de caracterul '-'
                        }
                        trim(copy_of_message);
                        char copy_of_message2[sizeof(copy_of_message)];
                        strcpy(copy_of_message2,copy_of_message);
                        int a = 0;
                        int match = 1; //match pe wildcard 
                        while (a < client.topics_length && strlen(client.topics[a].name) != 0)
                        {
                            char **token1 = split_string(client.topics[a].name); //sparg cele 2 stringuri
                            char **token2 = split_string(copy_of_message2);
                            int x = 0;
                            if(strcmp(client.topics[a].name, "*") == 0)
                            { //daca clientul este abonat la *, e potrivire
                                match = 0;
                                break;
                            }
                            if(strcmp(client.topics[a].name, copy_of_message2) ==0)
                            {
                                match = 0;
                                break; //daca cele 2 sunt egale fara wildcard, iarasi potrivire
                            }
                            int y = 0;
                            while(token1[x]!=NULL && token2[y]!=NULL)
                            {
                                if(strcmp(token1[x],token2[y]) != 0 && strcmp(token1[x],"+")!=0 && strcmp(token1[x],"*") != 0)
                                {
                                    //nu se potrivesc fiindca sunt diferite ca stringuri si nu am nici + sau *
                                    match =1;
                                    break;
                                }
                                if(strcmp(token1[x],"+") == 0 && token2[y+1] == NULL && token1[x+1] == NULL)
                                {
                                    //se potrivesc (pe ultimul nivel compar + cu un string)
                                    match = 0;
                                    break;
                                }
                                if(strcmp(token1[x], token2[y]) == 0 && token2[y+1] == NULL && token1[x+1] == NULL)
                                {
                                    //se potrivesc pana la capat
                                    match = 0;
                                    break;
                                }
                                if(strcmp(token1[x],"*") == 0)
                                {
                                    //daca am *, ma pot duce pe oricate nivele cu whule
                                    while(token2[y]!=NULL && token1[x+1]!=NULL && strcmp(token1[x+1],token2[y])!=0)
                                    {
                                        y++;
                                    }
                                    y--;
                                }
                                if(token1[x+1] == NULL && strcmp(token1[x],"*") == 0)
                                {
                                    //daca se termina cu *, este direct potrivire
                                    match = 0;
                                    break;
                                }
                                x++;
                                y++;
                            }
                            if(token1[x] == NULL && token2[x] == NULL)
                                match = 0; //potrivire
                            a++;
                        }

                        if (match == 0)
                        {
                            printf("%s\n", recv_packet.message);
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
    if (argc != 4)
    {
        printf("\n Usage: %s <ID client> <ip> <port>\n", argv[0]);
        return 1;
    }
    // Parsam port-ul ca un numar
    uint16_t port;
    int rc = sscanf(argv[3], "%hu", &port);
    strcpy(client.id, argv[1]);
    DIE(rc != 1, "Given port is invalid");
    client.port = port;
    // Obtinem un socket TCP pentru conectarea la server
    const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    // Completam in serv_addr adresa serverului, familia de adrese si portul
    // pentru conectare
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
    DIE(rc <= 0, "inet_pton");

    // Ne conectam la server
    rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "connect");

    run_client(sockfd);

    // Inchidem conexiunea si socketul creat
    close(sockfd);

    return 0;
}
