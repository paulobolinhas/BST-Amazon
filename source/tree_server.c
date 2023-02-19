/*
* Grupo SD-050
* João Santos nº 57103
* Paulo Bolinhas nº 56300
* Rui Martins nº 56283
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "../include/network_server.h"
#include "../include/tree_skel.h"

void shutdown(){
    auxShutDown();
}

int main(int argc, char *argv[]) {

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, shutdown);

    if (argc != 3) {
        printf("ERRO MAIN SERVER - Eh necessario passar o numero de porto e o IP do ZooKeeper\n");
        return -1;
    }

    short port = (short)atoi(argv[1]);
    char* portServer = argv[1];
    char* zooKeeperIP = argv[2];
    int N = 1;

    if (port == 0) {
        printf("Porto invalido\n");
        return 0;
    }

    if (connectZooKeeper(portServer, zooKeeperIP) != 0) {
        printf("Erro ao connectar ao ZooKeeper\n");
        return 0;
    }

    int sockfd = network_server_init(port);
    tree_skel_init(N);
    
    network_main_loop(sockfd);

    auxShutDown();

}