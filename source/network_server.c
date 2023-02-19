/*
* Grupo SD-050
* João Santos nº 57103
* Paulo Bolinhas nº 56300
* Rui Martins nº 56283
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include "../include/message-private.h"
#include "../include/tree_skel.h"
#include "../include/network_server.h"
#include "../sdmessage.pb-c.h"

#define NCLIENTS 51 //um servidor + a variavel-1

#define TRUE 1
#define FALSE 0


int sockfd;
struct sockaddr_in serverSocket;
int countBufLen;

/* Função para preparar uma socket de receção de pedidos de ligação
 * num determinado porto.
 * Retornar descritor do socket (OK) ou -1 (erro).
 */
int network_server_init(short port) {

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { // SOCK_STREAM MEANS IT'S TCP
        printf("ERRO network_server_init(): Erro ao criar socket\n");
        return -1;
    }

    int optval = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0){
        printf("ERRO no setsockopt()");
        return -1;
    }

    serverSocket.sin_family = AF_INET;
    serverSocket.sin_port = htons(port);
    serverSocket.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind assigns the adress specified by the descriptor
    if (bind(sockfd, (struct sockaddr *)&serverSocket, sizeof(serverSocket)) < 0) {
        printf("ERRO network_server_init() : Erro no bind\n");
        close(sockfd);
        return -1;
    }

    // listen marks the socket as a passive one that is waiting to accept a incoming connection
    if (listen(sockfd, 0) < 0) {
        printf("ERRO network_server_init() : Erro no listen\n");
        close(sockfd);
        return -1;
    };

    printf("Servidor iniciado, à espera de dados:\n");
    return sockfd;
}

/* Esta função deve:
 * - Aceitar uma conexão de um cliente;
 * - Receber uma mensagem usando a função network_receive;
 * - Entregar a mensagem de-serializada ao skeleton para ser processada;
 * - Esperar a resposta do skeleton;
 * - Enviar a resposta ao cliente usando a função network_send.
 */
int network_main_loop(int listening_socket) {

    int connsockfd; // descriptor of the socket connected with the client socket
    struct sockaddr_in client_adr;
    socklen_t client_size = sizeof(struct sockaddr);
    int adjustArray = FALSE;

    struct pollfd allConnections[NCLIENTS];
    int nFileDesc = 1; //numero de file descriptors
    int kFileDesc; //numero de descritores com eventos ou erros

    for (int i = 0; i < NCLIENTS; i++) {
        allConnections[i].fd = -1;
    }

    //a posicao 0 fica reservada pra welcoming socket
    allConnections[0].fd = sockfd;  
    allConnections[0].events = POLLIN;
    
    MessageT *message = NULL;
    
    while ((kFileDesc = poll(allConnections, nFileDesc, -1)) >= 0 && toShutDown == FALSE) {

        if (kFileDesc > 0) {

            if ((allConnections[0].revents & POLLIN) && (nFileDesc < NCLIENTS)) {
                if ((allConnections[nFileDesc].fd = accept(allConnections[0].fd, (struct sockaddr *) &client_adr, &client_size)) > 0) {
                    printf("Cliente %d conectado.\n", allConnections[nFileDesc].fd);
                    allConnections[nFileDesc].events = POLLIN;
                    nFileDesc++;
                }
            }

            for (int i = 1; i < nFileDesc; i++) {

                if (allConnections[i].revents & POLLIN) {
                    if ((message = network_receive(allConnections[i].fd)) == NULL) {
                        printf("Cliente %d desconectado (dentro do pollin).\n", allConnections[i].fd);
                        close(allConnections[i].fd);
                        allConnections[i].fd = -1;
                        adjustArray = TRUE;
                    } else {
                    
                        if (invoke(message) == -1)
                            break;

                        if (network_send(allConnections[i].fd, message) == -1) {
                            printf("network_send falhou\n");
                            break;
                        }
                    }
                    
                    if ((allConnections[i].revents & POLLHUP) || (allConnections[i].revents & POLLERR) || (allConnections[i].revents & POLLNVAL)) {
                        printf("Cliente %d desconectado (fora do pollin).\n", allConnections[i].fd);
                        close(allConnections[i].fd);
                        allConnections[i].fd = -1;
                        adjustArray = TRUE;
                    }

                    if (adjustArray) {
                        adjustArray = FALSE;
                        for (int i = 0; i < nFileDesc; i++) {
                            if (allConnections[i].fd == -1) {
                                for(int j = i; j < nFileDesc; j++) {
                                    allConnections[j].fd = allConnections[j+1].fd;
                                }
                                i--;
                                nFileDesc--;
                            }
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < nFileDesc; i++)  {
        if(allConnections[i].fd >= 0)
            close(allConnections[i].fd);
    }

    if(message != NULL)
        message_t__free_unpacked(message, NULL);
    
    return 0;
}

/* Esta função deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura message_t.
 */
MessageT *network_receive(int client_socket) {
    int sizeRead;

    if (read_all(client_socket, &sizeRead, sizeof(int)) <= 0) {
        close(client_socket);
        return NULL;
    }

    sizeRead = ntohl(sizeRead); // convert network-byte order to host-byte order

    char* buffer = malloc(sizeRead);

    if(buffer == NULL) {
        free(buffer); 
        printf("buffer == NULL\n");
        return NULL;
    }

    MessageT *msg;

    if (read_all(client_socket, buffer, sizeRead) <= 0) {
        printf("Erro network_receive() - Erro ao receber dados do cliente\n");
        free(buffer);
        close(client_socket);
        return NULL;
    }

    msg = message_t__unpack(NULL, sizeRead, buffer);

    free(buffer);

    return msg;
}

/* Esta função deve:
 * - Serializar a mensagem de resposta contida em msg;
 * - Libertar a memória ocupada por esta mensagem;
 * - Enviar a mensagem serializada, através do client_socket.
 */
int network_send(int client_socket, MessageT *msg) {

    int sizeRead = message_t__get_packed_size(msg);
    char* buffer = malloc(sizeRead);

    if(buffer == NULL) {
        free(buffer); 
        printf("buffer == NULL\n");
        return -1;
    }

    message_t__pack(msg, buffer);
    int sizeNetWork = htonl(sizeRead);

    if (write_all(client_socket, &sizeNetWork, sizeof(int)) <= 0){
        printf("Erro network_send() - Erro ao receber dados do cliente\n");
        close(client_socket);
        free(buffer);
        return -1;
    }

    sizeNetWork = ntohl(sizeNetWork);

    if (write_all(client_socket, buffer, sizeNetWork) <= 0){ 
        printf("ERRO network_send() - Erro ao enviar resposta ao cliente\n");
        close(client_socket);
        free(buffer);
        return -1;
    }

    message_t__free_unpacked(msg, NULL);

    free(buffer);

    return 0;
}

/* A função network_server_close() liberta os recursos alocados por
 * network_server_init().
 */
int network_server_close(){
    int retClose = close(sockfd);
    printf("Servidor desligado! \n");
    return retClose;
}
