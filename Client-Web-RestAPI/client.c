#include "helpers.h"
#include "parson.h"
#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <ctype.h>

#define SERVER_PORT 8080
#define SERVER_IP "3.8.116.10"

int isNumber(char *s) 
{ 
    for (int i = 0; i < strlen(s); i++) 
        if (!isdigit(s[i])) 
            return 0; 
  
    return 1; 
} 

int main() {

	char *message;
    char *response;
    int sockfd;
    char *host = "ec2-3-8-116-10.eu-west-2.compute.amazonaws.com";
    char *command = malloc(sizeof(char));
    char **cookies = malloc(2 * sizeof(char *));

    while(1) {
    	
    	sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
		
		printf("Command: ");
		scanf("%s", command);    	

		if (!strcmp(command, "register")) {
			
			char *username= malloc (sizeof(char)), *password=malloc(sizeof(char));

			printf("username = ");
			scanf("%s", username);
			printf("password = ");
			scanf("%s", password);

			JSON_Value *root_value = json_value_init_object();
    		JSON_Object *root_object = json_value_get_object(root_value);
			json_object_set_string(root_object, "username", username);
			json_object_set_string(root_object, "password", password);

			char *serialized_string = json_serialize_to_string_pretty(root_value);

			message = compute_post_request(host, "/api/v1/tema/auth/register", "application/json", serialized_string, NULL, 0);
			puts(message);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			puts("\n");
			if (basic_extract_json_response(response) == NULL) {
				puts("OK");				
			} else {
				puts(basic_extract_json_response(response));
			}
			puts("\n");
			free(username);
			free(password);

		} else if (!strcmp(command, "login")) {

			char *username= malloc (sizeof(char)), *password=malloc(sizeof(char));

			printf("username = ");
			scanf("%s", username);
			printf("password = ");
			scanf("%s", password);

	   		JSON_Value *root_value = json_value_init_object();
   			JSON_Object *root_object = json_value_get_object(root_value);
			json_object_set_string(root_object, "username", username);
			json_object_set_string(root_object, "password", password);

			char *serialized_string = json_serialize_to_string_pretty(root_value);

			message = compute_post_request(host, "/api/v1/tema/auth/login", "application/json", serialized_string, NULL, 0);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			puts("\n");
			if (basic_extract_json_response(response) == NULL) {
				char *token = strstr(response, "connect.sid=");
				token = strtok(token, ";");
				puts(token);
				cookies[0] = token;				
			} else {
				puts(basic_extract_json_response(response));
			}
			puts("\n");
			free(username);
			free(password);

		} else if (!strcmp(command, "enter_library")) {

			message = compute_get_request(host, "/api/v1/tema/library/access", cookies, 1);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			puts("\n");
			
			char *token = basic_extract_json_response(response);
			puts(token);
			JSON_Value *root_value = json_parse_string(token);
			JSON_Object *root_object = json_value_get_object(root_value);
			if (json_object_get_string(root_object, "token") != NULL) {
				strcpy(token, json_object_get_string(root_object, "token"));
				cookies[1] = token;
			}
			puts("\n");

		} else if (!strcmp(command, "get_books")) {
			
			message = compute_get_request(host, "/api/v1/tema/library/books", cookies, 2);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			puts("\n");
			if (basic_extract_json_response(response) == NULL) {
				char *token = strstr(response, "[");
				puts(token);
			} else {
				puts(basic_extract_json_response(response));
			}
			puts("\n");			

		} else if (!strcmp(command, "get_book")) {
			
			char *id = malloc(sizeof(char));
			printf("id = ");
			scanf("%s", id);
			
			char *url = malloc(80 * sizeof(char));
			strcpy(url, "/api/v1/tema/library/books/");
			strcat(url, id);

			message = compute_get_request(host, url, cookies, 2);
			send_to_server(sockfd,message);
			response = receive_from_server(sockfd);
			puts("\n");
			if (basic_extract_json_response(response) == NULL) {
				char *token = strstr(response, "[{\"");
				puts(token);
			} else {
				puts(basic_extract_json_response(response));
			}
			puts("\n");
			free(id);
			free(url);

		} else if (!strcmp(command, "add_book")) {
			char *title = malloc(sizeof(char)), *author = malloc(sizeof(char));
			char *genre = malloc(sizeof(char)), *publisher = malloc(sizeof(char));
			char *page_count = malloc(sizeof(char));

			printf("title = ");
			scanf("%s", title);
			printf("author = ");
			scanf("%s", author);
			printf("genre = ");
			scanf("%s", genre);
			printf("publisher = ");
			scanf("%s", publisher);
			printf("page_count = ");
			scanf("%s", page_count);
			

			int count = atoi(page_count);

			while(!isNumber(page_count) || count <= 0) {
				printf("\nError!\nWrong type or Negative value for page_count. page_count = ");
				scanf("%s", page_count);
				count = atoi(page_count);
			}
			while(isNumber(author)) {
				printf("\nError!\nAuthor is a number. author = ");
				scanf("%s", author);
			}


			JSON_Value *root_value = json_value_init_object();
    		JSON_Object *root_object = json_value_get_object(root_value);
			json_object_set_string(root_object, "title", title);
			json_object_set_string(root_object, "author", author);
			json_object_set_string(root_object, "genre", genre);
			json_object_set_string(root_object, "publisher", publisher);
			json_object_set_number(root_object, "page_count", count);

			char *serialized_string = json_serialize_to_string_pretty(root_value);

			message = compute_post_request(host, "/api/v1/tema/library/books", "application/json", serialized_string, cookies, 2);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			puts("\n");
			if (basic_extract_json_response(response) == NULL) {
				puts("OK");				
			} else {
				puts(basic_extract_json_response(response));
			}
			puts("\n");

			free(title);
			free(genre);
			free(author);
			free(publisher);
			free(page_count);

		} else if (!strcmp(command, "delete_book")) {

			char *id = malloc(sizeof(char));
			printf("id = ");
			scanf("%s", id);
			
			char *url = malloc(80 * sizeof(char));
			strcpy(url, "/api/v1/tema/library/books/");
			strcat(url, id);

			message =  compute_delete_request(host, url, cookies, 2);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			puts("\n");
			if (basic_extract_json_response(response) == NULL) {
				puts("OK");				
			} else {
				puts(basic_extract_json_response(response));
			}
			puts("\n");

			free(id);
			free(url);

		} else if(!strcmp(command, "logout")) {
			
			message = compute_get_request(host, "/api/v1/tema/auth/logout", cookies, 1);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			puts("\n");
			if (basic_extract_json_response(response) == NULL) {
				puts("OK");				
			} else {
				puts(basic_extract_json_response(response));
			}
			puts("\n");
			cookies[1] = NULL;
		
		} else if (!strcmp(command, "exit")) {
			
			free(command);
			free(cookies);
			close_connection(sockfd);
			return 0;
		}	
    }
}
