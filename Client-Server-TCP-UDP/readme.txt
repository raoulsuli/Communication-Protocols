Tema 2 – PC

Descriere generala:

	Tema consta in implementarea unei aplicatii de tip server – client unde
clientii  sunt subscriberi ce pot fi abonati la diferite canale. Evidenta
acestor abonari este tinuta de server, care gestioneaza clientii tcp
(subscriberii) si clientii udp (cei care trimit mesaje pe diferite canale). Un
client se poate abona sau dezabona la topicuri inexistente fara a cauza erori.
Daca un client este abonat la un topic atunci acesta va primi imedia mesajele
trimise pentru abonatii acelui topic daca acesta este online. Daca acesta este
offline va primi toate mesajele pe canalul respectiv daca opteaza pentru
optiunea store&forward odata ce va fi online.

Observatii legate de afisari:

	Daca rularea executabilelor (server si subscriber) se face cu argumente
eronate atunci programul se va opri si va fisa un mesaj de eroare. Acelasi lucru
se intampla daca de pe oricare dintre ele nu se trimite sau nu se primeste un
numar corect de bytes. Daca un subscriber da o comanda corecta de subscribe sau
unsubscribe se va afisa mesajul cerut in enunt in consola corespunzatoare
serverului dupa ce comanda a ajuns la server. Cand se conecteaza un subscriber
cu id disponibil se va afisa mesajul cerut in enunt, altfel se va afisa un mesaj
ce sugereaza faptul ca id-ul este deja utilizat. In subscriber se vor afisa toate
mesajele primite de la server (de la clientul TCP chiar) sub formatul cerut
indiferent daca acestea au fost trimise cat timp clientul a fost online sau cat
timp clientul era offline, dar era abonat la topicul respectiv si avea store&
forward-ul activ. Cazurile in care nu se reuseste deschiderea unui socket sau nu
se reuseste sa se faca bind pe acesta sunt tratate cu mesaje de eroare si cu
inchiderea aplicatiei.

Indicatii rulare si comenzi:
	./server <PORT_SERVER>
	./subscriber <ID> 127.0.0.1 <PORT_SERVER>
	Subscribe: subscribe <topic> <sf> ENTER
	Unsubscribe: unsubscribe <topic> ENTER
	Exit: exit ENTER (si in server si in suscriber)
	Unde: sf = 1 sau 0

Surse:

server.cpp

	Initial se creeaza socketii pentru udp si tcp, se face bind pe acestia,
file-descriptorii tcp, udp si stdin (0) sunt adaugati in multimea de descriptori.
Apoi, se intra in bucla (while) din care se iese doar daca s-a primit comanda
“exit” de la tastatura. Aici parcurg cu un for toti file-descriptorii care se
afla in multimea de file-descriptori si tratez fiecare caz in parte.
	Primul caz este gestionarea unui nou client tcp. Verific mai intai in
multimea de clienti (lista) daca el se afla deja (daca se afla ii schimb starea
in online) si il adaug in map-ul <socket, client> care tine evidenta clientilor
online. Apoi, verific daca are mesaje in asteptare de pe orice topic (pending)
si ii trimit toate mesajele gasite.
	Al doilea caz este gestionarea unui client udp. Aici prelucrez mesajul
primit in buffer folosind structura udp_message si trimit mesajul prelucrat
conform enuntului tututor clientilor online abonati la topicul corespunzator.
	Al treilea caz este primirea unei comenzi de la tastatura. Singura
comanda valida este “exit”. Orice alta comanda este tratata cu un mesaj de
comanda eronata.
	Ultimul caz consta in primirea unei comenzi de la un client tcp
(subscriber) online. Singurele comenzi valide sunt subscribe sau unsubscribe.
In cazul unuui subscribe, topicul pe care s-a facut subscribe este adaugat in
lista de topicuri a unui client (in structura client) cu sf-ul corespunzator,
iar in caz contrar acesta este sters din lista.

subscriber.cpp

	Aici se deschide doar socket-ul pe care se comunica cu serverul si se
realizeaza conexiunea la server cu functia connect. Singurii file-descriptori
sunt: cel corespunzator stdin-ului si cel corespunzator socketului conexiunii
cu serverul. In bucla infinita se verifica daca s-a primit o comanda de la
tastatura sau uun mesaj de la server. Comanda exit este singura acceptata de
la tastatura. De la server se poate primi comanda exit doar in cazul in care
subscriber-ul a incercat sa se autentifice cu un id deja existent in server.
Mesajele ce vin de la server sunt cele care trebuie afisate si sunt cele
primite de la clientii UDP pentru un numic topic.

helpers.cpp

	Acest fisier contine structurile folosite pentru prelucrarea clientilor,
mesajelor de tip subscribe si mesajelor provenite de la clientii UDP si
functiilor folosite pentru modularizarea fisierului server.cpp. Clientul
va tine id static, o variabila booleana pentru a sti daca el este online sau
nu, o lista de mesaje in asteptare care vor fi trimise catre client cand acesta
intra pe server (in cazul in care aceste mesaje exista, bineinteles) si o lista
de forma topic-sf pentru a sti la ce topicuri este abonat un subscriber si daca
acesta doreste sa primeasca mesaje si daca este deconectat de la server.
Functiile au nume sugestiv si sunt folosite pentru decodificarea mesajelor udp si
tcp, pentru stergerea sau adaugarea de elemente din listele de topicuri sau de
mesaje in asteptare. O observatie importanta este faptul ca un subscribe inseamna
adaugarea unui nod nume_topic – sf in lista de topicuri daca nu exista sau
actualizarea sf-ului daca acesta exista, iar unsubscribe este echivalent cu stergerea
nodului din lista de topicuri.

