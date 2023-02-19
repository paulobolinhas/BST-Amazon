/*
* Grupo SD-050
* João Santos nº 57103
* Paulo Bolinhas nº 56300
* Rui Martins nº 56283
*/

#ifndef _CLIENT_STUB_PRIVATE_H
#define _CLIENT_STUB_PRIVATE_H

#include "data.h"
#include "entry.h"
#include "zookeeper/zookeeper.h"
#include <netinet/in.h>

/* Estrutura que define a árvore de dados
*/
struct rtree_t {
    
    struct sockaddr_in socket;
    int descriptor;
    
    // char* currentNodeID; //guardar identificador do nó efemero do servidor
    char* nextNodePath; //guardar o mesmo que em cima mas para o próximo servidor na cadeia

    // struct sockaddr_in nextServerSocket; //guardar a socket do proximo servidor
    // int nextServerDescriptor;

};
extern struct rtree_t *remoteTree;


#endif