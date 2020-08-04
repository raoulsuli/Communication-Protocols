#include <stdio.h>
#include <stdlib.h>
#include<bits/stdc++.h>
#include <iterator> 
#include <map> 
#include <string.h>
#include <math.h>
  
using namespace std;

void DIE(bool assertion, const char *call_description) {		
	if (assertion) {				
		fprintf(stderr, "(%s, %d): ",
				__FILE__, __LINE__);
		perror(call_description);	
	    exit(EXIT_FAILURE);	
	}									
}

typedef struct udp_message_node {
	char topic[51];
	char type[11];
	char *payload;
	udp_message_node *next;
}udp_message_node;

typedef struct topic_list_in_client {
	char name[51];
	bool sf;
	topic_list_in_client *next;
}topic_list_in_client;

typedef struct string_list {
	char *value;
	string_list *next;
}string_list;

typedef struct client {
	char id[11];
	bool online;
	topic_list_in_client *topics;
	string_list *pending_messages;
}client;

typedef struct client_list {
	client *subscriber;
	client_list *next;
}client_list;

typedef struct server_message {
	char type;
	char topic[51];
	bool sf;
}server_message;

// functie folositoare pentru debugging
void print_client(client *subscriber) {
	printf("Id: %s\n", subscriber->id);
	if(subscriber->online == true) {
		printf("Online\n");
	} else {
		printf("Not Online\n");
	}
	printf("Pending messages:\n");
	string_list *aux = subscriber->pending_messages;
	while(aux != NULL) {
		printf("%s\n", aux->value);
		aux = aux->next;
	}
	printf("Topics and sf:\n");
	topic_list_in_client *aux2 = subscriber->topics;
	while(aux2 != NULL) {
		printf("%s %d\n", aux2->name, aux2->sf);
		aux2 = aux2->next;
	}
}

string_list *add_string_to_list(char *value, string_list *list) {
	string_list *new_node = (string_list *)malloc(sizeof(string_list));
	new_node->value = (char *)calloc(strlen(value) + 1, sizeof(char));
	strcpy(new_node->value, value);
	new_node->next = list;
	return new_node;
}

void free_string_list(string_list *list) {
	string_list *aux = list, *prev_aux = list;
	while(aux != NULL) {
		aux = aux->next;
		free(prev_aux->value);
		free(prev_aux);
		prev_aux = aux;
	}
}

topic_list_in_client *add_topic(char *name, bool sf, topic_list_in_client *topics) {
	// mesaj de subscribe
	printf("subscribe %s\n", name);

	// verifica daca topicul exista deja
	topic_list_in_client *aux = topics;
	while(aux != NULL) {
		if (strcmp(aux->name, name) == 0) {
			aux->sf = sf;
			return topics;
		}
		aux = aux->next;
	}

	// topicul nu a fost gasit, deci se creeaza un nou element si se adauga in lista
	topic_list_in_client *new_topics = (topic_list_in_client *)malloc(sizeof(topic_list_in_client));
	new_topics->name[0] = '\0';
	strcpy(new_topics->name, name);
	new_topics->sf = sf;
	new_topics->next = topics;
	return new_topics;
}

topic_list_in_client *remove_topic(char *name, topic_list_in_client *topics) {
	// mesaj unsubscribe
	printf("unsubscribe %s\n", name);

	if (topics == NULL) {
		return NULL;
	}

	topic_list_in_client *aux = topics, *prev_aux = topics;
	// caz cap de lista
	if (strcmp(aux->name, name) == 0) {
		aux = aux->next;
		free(prev_aux);
		return aux;
	}

	// cauta in lista de topicuri
	while(aux != NULL) {
		if (strcmp(aux->name, name) == 0) {
			prev_aux->next = aux->next;
			free(aux);
			break;
		}
		prev_aux = aux;
		aux = aux->next;
	}
	return topics;
}

client_list *add_client_to_clients(map<int, client *> &clients, char *id, int socket, client_list *all_clients) {
	// cauta client in lista
	client_list *aux = all_clients;
	while (aux != NULL) {
		if (strcmp(aux->subscriber->id, id) == 0) {
			aux->subscriber->online = true;
			clients.insert(make_pair(socket, aux->subscriber));
			return all_clients;
		}
		aux = aux->next;
	}

	// clientul nu a fost gasit, deci il cream
	client *new_client = (client*)malloc(sizeof(client));
	new_client->id[0] = '\0';
	strcpy(new_client->id, id);
	new_client->online = true;
	clients.insert(make_pair(socket, new_client));
	client_list *new_node = (client_list *)malloc(sizeof(client_list));
	new_node->subscriber = new_client;
	new_node->next = all_clients;
	return new_node;
}

client *get_client(client_list *list, char *id) {
	// cauta client dupa id
	client_list *aux = list;
	while(aux != NULL) {
		if (strcmp(aux->subscriber->id, id) == 0) {
			return aux->subscriber;
		}
		aux = aux->next;
	}
	return NULL;
}

bool decode_message(server_message *message, char *buffer) {
	char *token = strtok(buffer, " ");
	int nr_param = 1;

	while(token != NULL) {
		if (nr_param == 1) {
			if (strcmp(token, "subscribe") != 0 && strcmp(token, "unsubscribe") != 0) {
				printf("%s", "Invalid command from subscriber.\n");
				return false;
			}
			message->type = token[0];
		} else if (nr_param == 2) {
			message->topic[0] = '\0';
			strcpy(message->topic, token);
		} else if (nr_param == 3) {
			if(strcmp(token, "1\n") == 0) {
				message->sf = true;
			} else if (strcmp(token, "0\n") == 0) {
				message->sf = false;
			} else {
				printf("%s", "Invalid command from subscriber.\n");
				return false;
			}

			if (message->type == 'u') {
				printf("%s", "Invalid command from subscriber.\n");
				return false;
			}
		} else {
			printf("%s", "Invalid command from subscriber.\n");
			return false;
		}
		nr_param ++;
		token = strtok(NULL, " ");
	}
	if (nr_param == 3 && message->type == 's') {
		printf("%s", "Invalid command from subscriber.\n");
		return false;
	}
	return true;
}

client_list *add_client_to_list(client_list *list, client *subscriber) {
	client_list *new_node = (client_list *)malloc(sizeof(client_list));
	new_node->subscriber = subscriber;
	new_node->next = list;
	return new_node;
}

client_list *delete_client_from_list(client_list *list, client *subscriber) {
	client_list *aux = list, *prev_aux = list;
	while(aux != NULL) {
		if (strcmp(aux->subscriber->id, subscriber->id) == 0) {
			if (aux == list) {
				aux = aux->next;
				free(list);
				return aux;
			}
			prev_aux->next = aux->next;
			free(aux);
			return list;
		}
		prev_aux = aux;
		aux = aux->next;
	}
	return list;
}

bool check_unique_id(map<int, client *> clients, char *id) {
	map<int, client *>::iterator itr;
	for (itr = clients.begin(); itr != clients.end(); ++itr) { 
       	if(strcmp(itr->second->id, id) == 0) {
			   return false;
		   }
    }
	return true;
}

bool is_valid_udp_message(char *buffer) {
	if (buffer[50] != 0) {
		if (buffer[50] != 1) {
			if (buffer[50] != 2) {
				if (buffer[50] != 3) {
					return false;
				}
			}
		}
	}
	return true;
}

bool check_sf_active(client *subscriber, char *name) {
	topic_list_in_client *aux = subscriber->topics;
	while(aux != NULL) {
		if (strcmp(name, aux->name) == 0) {
			if (aux->sf == 1) {
				return true;
			} else {
				return false;
			}
		}
		aux = aux->next;
	}
	return false;
}

bool check_subscription(client *subscriber, char *name) {
	topic_list_in_client *aux = subscriber->topics;
	while (aux != NULL) {
		if (strcmp(aux->name, name) == 0) {
			return true;
		}
		aux = aux->next;
	}
	return false;
}

udp_message_node *decode_udp_message(char *buffer) {
	udp_message_node *new_message = (udp_message_node *)malloc(sizeof(udp_message_node));
	new_message->topic[0] = '\0';
	new_message->type[0] = '\0';
	strncpy(new_message->topic, buffer, 50);
	double real_num;
	uint8_t test = buffer[50];
	
	if (test == 0) {
		strcpy(new_message->type, "INT");
		long long num = ntohl(*(uint32_t*)(buffer + 52));
		if (buffer[51] == 1) {
			num *= -1;
		}
		new_message->payload = (char *)calloc(11, sizeof(char));
		sprintf(new_message->payload, "%lld", num);
	} else if (test == 1) {
		strcpy(new_message->type, "SHORT_REAL");
        real_num = htons(*(uint16_t*)(buffer + 51));
        real_num /= 100;
		new_message->payload = (char *)calloc(20, sizeof(char));
        sprintf(new_message->payload, "%.2f", real_num);
	} else if (test == 2) {
		strcpy(new_message->type, "FLOAT");
		real_num = ntohl(*(uint32_t*)(buffer + 52));
        real_num /= pow(10, *(buffer + 56));

        if (buffer[51] > 0) {
            real_num *= -1;
        }
		new_message->payload = (char *)calloc(40, sizeof(char));
        sprintf(new_message->payload, "%lf", real_num);
	} else {
		strcpy(new_message->type, "STRING");
		new_message->payload = (char *)calloc(1501, sizeof(char));
        strcpy(new_message->payload, buffer + 51);
	}
	return new_message;
}

void free_client(client *client) {
	string_list *aux = client->pending_messages, *prev_aux = client->pending_messages;
	while(aux != NULL) {
		aux = aux->next;
		free(prev_aux->value);
		free(prev_aux);
		prev_aux = aux;
	}
	topic_list_in_client *aux2 = client->topics, *prev_aux2 = client->topics;
	while(aux2 != NULL) {
		aux2 = aux2->next;
		free(prev_aux2);
		prev_aux2 = aux2;
	}
}

void free_clients(client_list *clients) {
	client_list *aux = clients, *prev_aux = clients;
	while(aux != NULL) {
		aux = aux->next;
		free_client(prev_aux->subscriber);
		free(prev_aux);
		prev_aux = aux;
	}
}