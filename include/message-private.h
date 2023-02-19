/*
* Grupo SD-050
* João Santos nº 57103
* Paulo Bolinhas nº 56300
* Rui Martins nº 56283
*/

#ifndef _MESSAGE_PRIVATE_H
#define _MESSAGE_PRIVATE_H


#include "../sdmessage.pb-c.h"
#include <netinet/in.h>
#include <unistd.h>

int write_all(int sock, char *buf, int len);

int read_all(int sock, char *buf, int len);

#endif