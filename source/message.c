/*
* Grupo SD-050
* João Santos nº 57103
* Paulo Bolinhas nº 56300
* Rui Martins nº 56283
*/

#include <stdio.h>

#include "../include/message-private.h"
#include "../sdmessage.pb-c.h"
#include <netinet/in.h>
#include <errno.h>

int write_all(int sock, char *buf, int len) {

  int bufsize = len;

  while(len>0) {

    int res = write(sock, buf, len);

    if(res<=0) {
      if(errno==EINTR) continue;
      printf("write failed:");
      return res;
    }

    buf += res;
    len -= res;
  }

  return bufsize;
}

int read_all(int sock, char *buf, int len){

  int lengthRead = 0;
  int res;

  while(lengthRead < len){
    res = read(sock, buf + lengthRead, len - lengthRead);
    
    if(res <= 0)
      return res;

    lengthRead += res;
  }

  return lengthRead;
}