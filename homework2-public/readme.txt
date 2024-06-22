Readme Pcom 
Timp de lucru: 6 zile (35-40 de ore)

Pe local am passed la absolut toate testele (inclusiv quick flow). Am pornit de la 
laboratorul 7 de Pcom, unde aveam un fişier client.c şi unul server.c. Programul iniţial
putea să aibă mai mulţi clienţi (după ce am făcut laboratorul), folosind multiplexare. 
Ȋn laborator, foloseam un timerfd, pentru a trimite mesaje promoţionale o dată la 4 secunde.
Totuşi, la această temă n-am avut nevoie de timerfd.  Probleme:  -aveam momente cȃnd 
checkerul pica la ȋnchiderea serverului şi mi-a luat mult timp să mă prind de faptul că 
dădea segmentation fault (ȋn server, salvam toate mesajele ȋntr-o matrice cam mică ->maxim 
100 de mesaje). Mi-am dat seama că nu este nevoie să salvez neapărat mesajele şi că le pot 
trimite la client dacă am verificat că acesta a dat subscribe la topic.

	Implementare server:
	Am pornit de la scheletul din laboratorul 7. Ȋn main, am limitat intrarea (numărul 
de argumente la 2). Serverul se ruleaza ./server port. (unde port este o valoare de la 1024 
la 65535). Programul meu acceptă doar acest format. Mi-am creat un socket (listenfd), căruia 
ȋi dau bind şi un udpsock (socket pentru clienţii udp). Funcţia run_chat_multi_server primeşte
ca parametri cei 2 sockeţi (tcp şi udp) şi portul pe care lucrez.  Ȋn funcţia 
run_chat_multi_server, ȋmi setez valorile pentru poll_fds. Valoarea 0 este asignată socketului
care ascultă clienţii tcp. Valoarea 1 este pentru timerfd, setat la 0.01 secunde. Valoarea 2
este setată pe 0 (pe acest caz, citesc de la inputul serverului). Ultima valoare (3) este 
setată pentru udpsock (socketul udp dat ca parametru funcţiei run_chat_multi_server).

	Numărul iniţial de sockeţi este de 4 (ȋl incrementez cu 1 pentru fiecare client TCP
care se conectează). Următoarea parte este un while foarte mare ȋn care, practic, se 
ȋntȃmplă totul. Ȋn acest while, fac un for de la 0 la numărul de sockeţi. Primul lucru pe
care ȋl fac apoi este să verific dacă am input de la tastatură. Compar inputul cu “exit”.
Dacă este exit, atunci trebuie să ȋnchid serverul. Fac un for de la 4 la num_sockets (adică
un for pe toţi clienţii) şi trimit mesajul “exit server”.  Clientul va ştii să interpreteze 
acest mesaj şi va da exit(0). Ȋnchid şi sockeţii TCP folosind comanda close().
	
	Dacă nu citesc input, verific dacă primesc vreo cerere de conexiune TCP. Dacă da, ȋi dau 
accept. Adaug socketul la mulţimea de sockeţi deja existentă. Pentru o conexiune UDP, citesc
pachetul primit folosind comanda udpsockfd = recvfrom(udp_sock, buf, 1551, 0, sock_addr, 
&sock_len);  Numărul 1551 vine de la dimensiunea totală (50 pentru topic, 1 octet de tip şi 
1500 pentru payload). Iau fiecare caz ȋn parte şi ȋmi creez ȋntr-un buffer mesajele pe care 
vreau să le transmit clienţilor.  
1. Cazul INT: Verific dacă tipul de date este un întreg 
(INT). Extrag semnul și numărul din buffer, convertindu-le în formatul corect și adăugându-le
într-un nou buffer (buf2). Tipul de pachet este setat la 0.  
2. Cazul SHORT_REAL: Verific dacă tipul de date este un număr real scurt (SHORT_REAL). Extrag 
numărul și îl convertesc în formatul corect, apoi îl adaug în bufferul buf2. Tipul de pachet 
este setat la 1.  
3. Cazul FLOAT: Verific dacă tipul de date este un număr real (FLOAT). Extrag numărul, puterea 
și semnul din buffer, îl convertesc în formatul corect și îl adaug în bufferul buf2. Tipul 
de pachet este setat la 2.  
4. Cazul STRING: Verific dacă tipul de date este un șir de caractere (STRING). Extrage șirul 
din buffer și îl adaug în bufferul buf2. Tipul de pachet este setat la 3. 

    Ȋn continuare, vine cea mai dificilă parte a temei, şi anume wildcards. La această parte, 
am implementat funcţia split_string care ia un string (un topic) şi returnează o matrice 
alocată dinamic (care ȋmi sparge stringul după caracterul ‘/’). Ȋn matrice, se află stringul 
spart (m-am complicat mult cu funcţia, puteam să folosesc şi strtok). Aplic această funcţie 
pentru fiecare topic al fiecărui client şi pentru mesajul pe care vreau să ȋl trimit
(din care extrag topicul). Eu practic compar 2 topicuri prin separarea lor prin caracterul 
‘/’. Verific ȋntȃi dacă am din start doar caracterul ‘*’ ȋn client.topics[a].name. Dacă da,
atunci am găsit o potrivire. Verific apoi şi dacă se potrivesc fără wildcards (clasicul strcmp
ȋntre cele 2 stringuri). Dacă niciuna dintre cele 2 varinate nu este bună, fac un while (cȃt 
timp cele 2 matrice mai au coloane). Verific cazul ȋn care ȋn primul string tokenul curent nu
este nici “*” sau “+” şi nici nu se potriveşte cu tokenul curent din al doilea string 
(nepotrivire din start). Dacă cele 2 stringuri urmează să se termine amȃndouă şi ȋn primul 
string am “+”, am o potrivire şi trimit mesajul către client. Mai există cazul ȋn care se 
termină ambele stringuri şi au amȃndouă ultimul token comun. (potrivire).  Cea mai grea parte 
este pentru caracterul *, deoarece poate face potrivire pe mai multe nivele. Fac un while cȃt 
timp al doilea string mai are tokenuri. Avansez prin al doilea string astfel.  Am abordat şi 
cazul cȃnd primul string se ȋncheie cu *. Ȋn acest caz, e evident că dacă le-am potrivit 
pȃnă ȋn acel punct, se vor potrivi şi mai departe. Verific dacă mesajul trimis de client este
subscribe <topic>.  Dacă da, atunci iau din  pachet id-ul clientului (la structura de pachete
am adăugat şi un client, cu id, topics şi online). Ȋi adaug topicul la clientul corespunzător.

    Ȋn cazul ȋn care clientul este deja abonat la acel topic, dau mesajul Subscribed to topic “.
Dacă mesajul primit de la client este exit, atunci clientul este eliminat, este adăugat ȋn
vectorul de clienti_out (cu id şi topic). Trebuie cumva să ȋi salvez topicurile, fiindcă se
poate reconecta oricȃnd. (socketul ȋl elimin oricum). Dacă mesajul trimis de client către 
server este unsubscribe <topic> (primesc şi datele clientului la pachet cu datele trimise),
identific clientul şi caut printre topicurile la care este abonat. Dacă găsesc topicul, ȋl
elimin. Dacă nu, afişez mesajul “Topic <nume_topic> doesn’t exist.”. Aceste printuri nu ȋmi
afectează testele, deoarece nu ajung niciodată ȋn teste ȋn situaţia să le afişez. (sunt
pentru debugging mai mult).

    Dacă mesajul de la un client nou ȋncepe aşa: "Un nou client s-a conectat cu id", ȋl adaug 
la lista de clienţi cu id-ul său. Verific dacă se află şi printre clienţii vechi (deja 
conectaţi). Există 3 cazuri aici:
-clientul este complet nou ( şi ȋl adaug la vectorul de clienţi).
-clientul este deja conectat (şi dau mesaj “Client <id> already connected.”. Trimit clientului 
respectiv mesajul “exit client duplicat”.
-clientul este nou, dar a mai fost conectat ȋnainte. Ȋn acest caz, trimit către client mesajul 
“subscribe_dupa_deconectare <topics>” şi ȋi pun toate topicurile din vechiul client (am făcut 
acest lucru ca să funcţioneze quick flow).


Desigur, iată textul formatat la 90 de caractere pe rând:

Implementare client (subscriber.c) 
    Am folosit funcţia run_client din laboratorul 7. (de la asta am plecat pentru a modela un 
client TCP). Ca la server, citesc de latastatură inputul. Dacă inputul este exit, trimit mesaj 
serverului şi dau exit. (EXIT(0)). Dacă ceea ce citesc de la tastatură are subscribe, trimit 
serverului mesajul subscribe <topic>, ȋmpreună cu clientul ataşat, iar serverul va ştii săseteze 
ȋn vectorul de clienţi respectivul topic. Ȋn cazul ȋn care ceea ce citesc de la tastatură ȋncepe 
cu unsubscribe, trimit serverului mesaj să dea unsubscribe la topic, ȋmpreună cu clientul ataşat. 
Dacă mesajul ȋncepe de forma “subscribe_după_deconectare”, extrag toate topicurile şi le adaug 
şi ȋn client (local, am o structură de tip client, ȋn care reţin id şi topicuri), pe care le
trimit apoi ataşate la diferite pachete. Există şi cazul ȋn care clientul este duplicat (dacă 
primesc mesajul “exit client duplicat”, afişez ȋn client “Client <id> already connected.”). Ȋn 
partea de main, mă asigur că primesc de la utilizator 4 argumente: ./subscriber <id> <ip> <port>. 
Ȋmi creez un socket pentru client, şi apoi dau connect. Apelez funcţia run_client cu parametrul
sockfd(socketul clientului TCP), şi care face tot ceea ce am descris mai sus.

    Ȋn fişierul common.c, am send_all şi recv_all ca funcţii, care au rolul de a trimite tot 
bufferul pe care ȋl primesc ca parametru. (se asigură că tot este primit/trimis). Ȋn fişierul 
helpers.h am structurile pe care le folosesc. Principala este cea pentru client, pe care am 
denumit-o subscriber. Ea conţine un vector de topicuri, un id (prin care identific clientul), 
socketul pe care lucrez, un flag pentru a vedea dacă clientul este online sau nu, o lungime a 
topicurilor (numărul de topicuri la care este abonat la un moment dat) şi un port.
    Ca structuri, mai folosesc chat_packet (cel din laborator), ȋn care am o lungime, un buffer cu 
payload-ul (mesajul efectiv), un tip (0, 1, 2 sau 3 ȋn funcţie de ce trimit) şi un client de tip 
subscriber (la fiecare mesaj, trebuie să ştiu cine este expeditorul).
