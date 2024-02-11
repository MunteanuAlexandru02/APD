README Tema 3 APD
Munteanu Alexandru-Constantin 331CC

Timp de implementare: destul de mult

Dificultati intampinate:
    - Gasirea unei modalitati de a trimite informatii
intr-un mod cat mai OK
    - Debug cand aveam erori in MPI_Send si MPI_Recv,
in special spre finalul temei cand aveam destul de multe
linii.

Detalii implementare:

    Pentru a reusi sa trimit informatii de la un client
la altul si de la tracker la clienti am structurat 
mesajele sub forma unor pachete, astfel:

+++++++++++++++++++++++++++++
+         Rank: INT         +
+++++++++++++++++++++++++++++
+      Message Type: INT    +
+++++++++++++++++++++++++++++
+   INFO: Depinde de mesaj  +
+++++++++++++++++++++++++++++

Tracker:
    Primesc de la fiecare client numarul de fisiere,
numele acestora, dar si numarul de hash-uri prezente.
Informatiile acestea sunt stocate intr-o vector de
tipul struct file_swarm, fiecare fisier reprezentand
un element. Daca exista 2 sau mai multi clienti care au
acelasi fisier, voi adauga in vectorul de clienti din
fiecare file_swarm inca o intrare cu rank-ul clientului.
    Dupa ce primeste toate fisierele trimite mesaje de
ACK clientilor dupa care devine un "server" ascultand
mereu dupa noi mesaje.
    Tracker-ul va primi 4 tipuri de mesaje, care sunt
descrise si cerinta, anume:

    - REQUIRE_FILE: un client cere clientii care au un
anumit fisier

    - REFRESH_FILE_INFO: un client si-a actualizat
fisierele pe care le detine sau numarul de hash-uri
dintr-un fisier

    - FINISHED_ONE_FILE: un client a descarcat toate
hash-urile dintr-un fisier, am decis sa nu marchez
explicit clientul ca seed, deoarece acesta este marcat
automat ca peer cand primeste un hash pe care il doreste,
iar pentru implemntare mea, diferenta dintre seed si 
peer este data doar de numarul de hash-uir detinute si
nu afecteaza buna functionare a programului.

    - FINISHED_ALL_FILES: clientul este marcat ca
"terminat" si i se transmite acestuia un mesaj pentru a 
inchide thread-ul de download. Pentru a vedea cand un 
toti clientii si-au terminat toate descarcarile, am 
decis sa las aceasta implementare in seama tracker-ului,
astfel ca acesta va verifica de fiecare data cand un 
client termina toate fisierele daca toti clientii au 
terminat de descarcat, cand toti clientii au terminat,
tracker-ul transmite mesaje clientilor si se inchide.

Peer:
    Am parsat fisierele in{rank}.txt folosind functia
parse file si am stocat informatiile pe care le voi 
transmite tracker-ului, cand primesc un mesajul de ACK
de la tracker, porneste cele doua thread-uri si devine
un "server", ascultand mereu dupa mesaj noi de la tracker
pentru a da join la thread-uri.

Download thread:
    Pentru fiecare fisier:

    - Trimit mesaje la fiecare 10 hash-uri descarcate
catre tracker pentru a primi clientii care au un fisier, 
chiar daca acestia nu au descarcat fisierul in 
totalitate.

    - Trimit mesaje la fiecare 5 hash-uri descarcate 
catre tracker pentru a actualiza numarul de hash-uri pe 
care le detin, astfel putand si trimite hash-uri catre 
alti clienti din cel curent.

    - Pentru a alege clientul de la care descarc am
folosind functia rand() si o verificare daca numarul de
hash-uri pe care le are clientul de la care vreau sa 
descarc este mai mare decat cel pe care le are clientul
curent. Am ales sa descarc hash-urile in ordine pentru
a evita momentele cand rand() returneaza hash-uri pe 
care deja le am, considerand ca este mai eficienta 
implementarea mea :P.

    - De fiecare data cand primesc un hash, ma folosesc
tot de MPI_Send si MPI_Recv pentru a trimite un hash 
catre acelasi client, dar pe celalalt thread.


Upload thread:
    Primeste 4 tipuri de mesaje:
    - FINISHED_EVERYTHING: se inchide

    - FINISHED_ONE_FILE: creaza fisierul de output si 
scrie in el.

    - UPDATE_HASHES_DB: pentru un fisier scrie hash-ul
primit intr-un vector pentru a fi dat altor clienti.

    - REQUIRE_HASH: trimite altui client hash-ul cerut.
