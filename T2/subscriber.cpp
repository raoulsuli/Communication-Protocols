#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include <netinet/tcp.h>


int main(int argc, char** argv) {
    DIE(strlen(argv[1]) > 10, "Subscriber ID should have maximum 10 characters.\n");
	DIE(argc != 4, "Usage: ./subscriber <ID> <IP_SERVER> <PORT_SERVER>.\n");

    char buffer[BUFLEN];
    int server_sock, n, flag = 1, ret;
    sockaddr_in serv_addr;
    fd_set fds, tmp_fds;

    // se creeaza socketul pentru conexiunea cu serverul
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    DIE(server_sock < 0, "Could not open server socket.\n");

    // se completeaza datele despre socketul TCP corespunzator conexiunii la server
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));

    ret = inet_aton(argv[2], &serv_addr.sin_addr);
    DIE(ret == 0, "Incorrect <IP_SERVER>. Conversion failed.\n");
    
    ret = connect(server_sock, (sockaddr*) &serv_addr, sizeof(serv_addr));
    DIE(ret < 0, "Could not connect to server.\n");

    ret = send(server_sock, argv[1], strlen(argv[1]) + 1, 0);
    DIE(ret < 0, "Unable to send ID to server.\n");

    // se dezactiveaza algoritmul lui Nagle pentru conexiunea la server
    setsockopt(server_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    // se seteaza file descriptorii socketilor pentru server si pentru stdin
    FD_ZERO(&fds);
    FD_SET(server_sock, &fds);
    FD_SET(0, &fds);

    while (true) {
        tmp_fds = fds;

		ret = select(server_sock + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "Could not select.\n");
        if (FD_ISSET(0, &tmp_fds)) {
            // s-a primit un mesaj de la stdin
            memset(buffer, 0, BUFLEN);
            fgets(buffer, BUFLEN - 1, stdin);
            if (strcmp(buffer, "exit\n") == 0) {
                break;
            }
            // s-a primit o comanda diferita de exit
           	n = send(server_sock, buffer, strlen(buffer), 0);
            DIE(n < 0, "Could not send message to server.\n");
        } else {
			// s-a primit mesaj de la server (de la un client UDP al serverului)
            memset(buffer, 0, BUFLEN);
            ret = recv(server_sock, buffer, BUFLEN, 0);
			if (ret == 0) {
				// serverul a inchis conexiunea
				break;
			}
			DIE(ret < 0, "Did not receive message from server.\n");
            // prelucreaza mesajul primit de la server din buffer ------------------------------------------
            if (strcmp(buffer, "exit") == 0) {
                DIE(true, "ID already used in server.\n");
            } else {
                printf("%s", buffer);
            }
        }
    }

    close(server_sock);

    return  0;
}
