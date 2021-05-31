#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include <netinet/tcp.h>


int main(int argc, char *argv[])
{
	DIE(argc != 2, "Usage: ./server <PORT_DORIT>\n");
	int newsockfd, portno, flag = 1;
	char *buffer = (char *)calloc(BUFLEN, sizeof(char));
	struct sockaddr_in udp_addr, tcp_addr, new_tcp;
	int n, i, ret;
	socklen_t udp_socklen = sizeof(sockaddr), tcp_socklen = sizeof(sockaddr);

	int tcp_sock, udp_sock;
	fd_set active_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea active_fds

	// se verifica portul ales pentru a nu fi rezervat
	portno = atoi(argv[1]);
	DIE(portno <= 1024, "Port number should be greater than 1024.\n");

	// se creeaza socketul TCP
    tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sock < 0, "Could not create tcp socket.\n");

	// se creeaza socketul UDP
    udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(udp_sock < 0, "Could not create udp socket.\n");

	// se completeaza addr-ul udp
	memset((char *) &udp_addr, 0, sizeof(udp_addr));
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(portno);
	udp_addr.sin_addr.s_addr = INADDR_ANY;
	tcp_addr.sin_family = AF_INET;
	tcp_addr.sin_port = htons(portno);
	tcp_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(udp_sock, (struct sockaddr *) &udp_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "Could not bind udp socket.\n");

	ret = bind(tcp_sock, (struct sockaddr *) &tcp_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "Could not bind tcp socket.\n");

	ret = listen(tcp_sock, MAX_CLIENTS);
	DIE(ret < 0, "Could not listen\n");

	// se goleste multimea de descriptori de citire (active_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&active_fds);
	FD_ZERO(&tmp_fds);
	// adauga socket tcp
	FD_SET(tcp_sock, &active_fds);
	// adauga socket udp
	FD_SET(udp_sock, &active_fds);
	// adauga socket-ul pentru stdin
	FD_SET(0, &active_fds);
	// seteaza file descriptorul maxim
	fdmax = tcp_sock;

	// evidenta subscriberilor online (socket - pointer la client)
	map<int, client*>online_clients;

	// lista tuturor clientilor (online + offline)
	client_list *all_clients = NULL;

	server_message s_message;
	bool exit_loop = false;
	while (exit_loop == false) {
		tmp_fds = active_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "Could not select.\n");

		memset(buffer, BUFLEN - 1, 0);

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == tcp_sock) {
					// a venit o cerere de conexiune pe socketul tcp
					// pe care serverul o accepta
					newsockfd = accept(i, (struct sockaddr *) &new_tcp, &tcp_socklen);
					DIE(newsockfd < 0, "Could not accept.\n");

					// se dezactiveaza algoritmul lui Nagle pentru conexiunea la clientul TCP
                    setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

					ret = recv(newsockfd, buffer, BUFLEN - 1, 0);
					DIE(ret < 0, "Did not recevie a client ID.\n");

					// verifica daca id-ul clientului e unic
					bool id_is_unique = check_unique_id(online_clients, buffer);

					if (id_is_unique == true) {
						// se adauga noul socket intors de accept() la multimea descriptorilor activi
						FD_SET(newsockfd, &active_fds);

						// se verifica daca noul socket este mai mare decat cel maxim
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}
						all_clients = add_client_to_clients(online_clients, buffer, newsockfd, all_clients);
						client *new_client = get_client(all_clients, buffer);

						// trimite mesaje in asteptare
						string_list *pending_message = new_client->pending_messages;
						string_list *prev_message = pending_message;
						while (pending_message != NULL) {
							send(newsockfd, pending_message->value, strlen(pending_message->value), 0);
							pending_message = pending_message->next;
							prev_message = pending_message;
							DIE(ret < 0, "Could not send UDP message to tcp client.\n");
						}
						free_string_list(new_client->pending_messages);
						new_client->pending_messages = NULL;
					} else {
						printf("Client ID %s already used.\n", buffer);
						memset(buffer, BUFLEN, 0);
						strcpy(buffer, "exit");
						ret = send(newsockfd, buffer, sizeof(buffer), 0);
						DIE(ret < 0, "Did not send exit message to tcp client.\n");
						continue;
					}

					printf("New client %s connected from %s:%d.\n", buffer, inet_ntoa(new_tcp.sin_addr), ntohs(new_tcp.sin_port));
				} else if (i == udp_sock) {
					// a venit un mesaj de la un client udp
					ret = recvfrom(udp_sock, buffer, BUFLEN, 0, (sockaddr*) &udp_addr, &udp_socklen);
					DIE(ret < 0, "Nothing received from UDP socket.\n");
					if (is_valid_udp_message(buffer) == true) {
						udp_message_node *new_udp_message = decode_udp_message(buffer);
						memset(buffer, BUFLEN, 0);
						sprintf(buffer, "%s:%d - %s - %s - %s", inet_ntoa(udp_addr.sin_addr),
						ntohs(udp_addr.sin_port), new_udp_message->topic, new_udp_message->type, new_udp_message->payload);

						// trimite mesajul catre toti subscriberii
						// trimite catre clientii online care au subscribe
						strcat(buffer, "\n");
						for (auto itr = online_clients.begin(); itr != online_clients.end(); ++itr) {
							if (check_subscription(itr->second, new_udp_message->topic) == true) {
								ret = send(itr->first, buffer, strlen(buffer), 0);
								DIE(ret < 0, "Could not send UDP message to tcp client.\n");
							}
						}
						// retine pentru toti clientii offline cu sf = 1
						client_list *aux = all_clients;
						while(aux != NULL) {
							if (aux->subscriber->online == false) {
								if (check_sf_active(aux->subscriber, new_udp_message->topic) == true) {
									aux->subscriber->pending_messages = add_string_to_list(buffer, aux->subscriber->pending_messages);
								}
							}
							aux = aux->next;
						}
						// elibereaza spatiu mesaj
						free(new_udp_message->payload);
						free(new_udp_message);
					} else {
						printf("Invalid message from udp client.\n");
					}
				} else if (i == 0) {
					// s-a primit o comanda de la stdin
					fgets(buffer, BUFLEN - 1, stdin);

                    if (!strcmp(buffer, "exit\n")) {
                        exit_loop = true;
                        break;
                    } else {
                        printf("Please enter a valid command.\n");
                    }
				} else {
					// s-a primit o comanda de la un client tcp
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, BUFLEN - 1, 0);
					DIE(n < 0, "No data received from suscriber.\n");

					if (n == 0) {
						// conexiunea s-a inchis
						auto itr = online_clients.find(i);
						// scoate clientul din map-ul socket - client (elibereaza socket-ul)
						itr->second->online = false;
						printf("Client %s disconnected.\n", itr->second->id);
						online_clients.erase(i);
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &active_fds);
						close(i);
					} else {
						// s-a primit o comanda de subscribe sau unsubscribe
						DIE(ret < 0, "Did not recevie a client ID.\n");
						if (decode_message(&s_message, buffer) == false) {
							continue;
						}
						auto itr = online_clients.find(i);
						client *subscriber = itr->second;
						if (s_message.type == 's') {
							// subscribe
							subscriber->topics = add_topic(s_message.topic, s_message.sf, subscriber->topics);
						} else {
							s_message.topic[strlen(s_message.topic) - 1] = '\0';
							// unsubscribe
							subscriber->topics = remove_topic(s_message.topic, subscriber->topics);
						}
					}
				}
			}
		}
	}

	// inchide socketii
	for (i = 0; i <= fdmax; i ++) {
		if (FD_ISSET(i, &active_fds)) {
            close(i);
        }
	}

	// free memory
	free_clients(all_clients);
	free(buffer);

	return 0;
}
