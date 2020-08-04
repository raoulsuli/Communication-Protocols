#ifndef _HELPERS_H
#define _HELPERS_H 1

#include "helpers.cpp"

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

void DIE(bool, char *);
#define BUFLEN	1552	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	INT_MAX	// numarul maxim de clienti in asteptare

#endif
