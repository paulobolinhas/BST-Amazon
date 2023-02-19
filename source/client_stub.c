/*
* Grupo SD-050
* João Santos nº 57103
* Paulo Bolinhas nº 56300
* Rui Martins nº 56283
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/data.h"
#include "../include/entry.h"
#include "../include/client_stub.h"
#include "../include/client_stub-private.h"
#include "../include/network_client.h"
#include "../sdmessage.pb-c.h"


/* ZooKeeper Znode Data Length (1MB, the max supported) */
#define ZDATALEN 1024 * 1024

typedef struct String_vector zoo_string;
int is_connected2;
const char* chain_root2 = "/chain"; 
struct rtree_t *rtree;

struct rtree_t *head;
char* headID = NULL;

struct rtree_t *tail;
char* tailID = NULL;

static char *watcher_ctx = "ZooKeeper Data Watcher";
zoo_string* children_list;

static zhandle_t *zh;

void connection_watcher2(zhandle_t *zzh, int type, int state, const char *path, void *watcherCtx) {
    if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTED_STATE) {
			is_connected2 = 1; 
		} else {
			is_connected2 = 0; 
		}
	} 
}

/**
* Data Watcher function for /MyData node
*/
static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
	if (state == ZOO_CONNECTED_STATE) {
		if (type == ZOO_CHILD_EVENT) {

 			if (ZOK != zoo_wget_children(zh, chain_root2, child_watcher, watcher_ctx, children_list)) {
 				fprintf(stderr, "Error setting watch at %s!\n", chain_root2);
 			}

            rtree_connect_head(head);
            rtree_connect_tail(tail);

			fprintf(stderr, "\n=== Nova lista de Znodes=== [ %s ]", chain_root2); 
			for (int i = 0; i < children_list->count; i++)  {
				fprintf(stderr, "\n(%d): %s", i+1, children_list->data[i]);
			}
			fprintf(stderr, "\n=== Feito ===\n"); 
		 } 
	 }
}

char* getHeadID() {
    char* currentHeadID = strtok(children_list->data[0], "node");

    if (ZOK != zoo_wget_children(zh, chain_root2, child_watcher, watcher_ctx, children_list)) {
 		fprintf(stderr, "Error setting watch at %s!\n", chain_root2);
 	}
    
    for (int i = 1; i < children_list->count; i++) {

        char* current = strtok(children_list->data[i], "node");

        if (atoi(current) < atoi(currentHeadID))
            strcpy(currentHeadID, current);

    }
    
    return currentHeadID;

}

char* getTailID() {
    char* currentTailID = strtok(children_list->data[0], "node");

    if (ZOK != zoo_wget_children(zh, chain_root2, child_watcher, watcher_ctx, children_list)) {
 		fprintf(stderr, "Error setting watch at %s!\n", chain_root2);
 	}
    
    for (int i = 1; i < children_list->count; i++) {

        char* current = strtok(children_list->data[i], "node");

        if (atoi(current) > atoi(currentTailID))
            strcpy(currentTailID, current);

    }
    
    return currentTailID;
}

void rtree_connectZooKeeper(const char *address_port) {

    children_list =	(zoo_string *) malloc(sizeof(zoo_string));
    
	zh = zookeeper_init(address_port, connection_watcher2, 2000, 0, NULL, 0); // é preciso dar close
	if (zh == NULL)	{
		fprintf(stderr, "Error connecting to ZooKeeper server!\n");
	    exit(0);
	} else {
        is_connected2 = 1;
    }

    if (is_connected2) {
        if (ZOK != zoo_wget_children(zh, chain_root2, &child_watcher, watcher_ctx, children_list)) {
            fprintf(stderr, "Error setting watch at %s!\n", chain_root2);
        }
    }

    headID = malloc(sizeof(char*)*256);
    tailID = malloc(sizeof(char*)*256);

}

void rtree_connect_head(struct rtree_t* headTree) {
    
    char* currentHeadID = getHeadID();

    if (strcmp(headID, currentHeadID) != 0) {
        if (headTree->descriptor)
            close(headTree->descriptor);
        strcpy(headID, currentHeadID);
    } else {
        return head;
    }

    char* metaData = (char *)malloc(ZDATALEN * sizeof(char));
    char node_path[120] = "";
    strcat(node_path, "/chain/node");
    strcat(node_path, headID); 
    int dataSizeRead = ZDATALEN;

    if (zoo_get(zh, node_path, 0, metaData, &dataSizeRead, NULL) != ZOK) { // copia para o buffer o node_path
        printf("zoo_get failed\n");
        return NULL;
    }

    if (headTree == NULL) {
        printf("remote tree == NULL");
        free(headTree);
        return NULL;
    }

    char* hostnameHead = strtok((char *) metaData, ":");
    int portHead = atoi(strtok(NULL, ":"));

    headTree->socket.sin_family = AF_INET;
    headTree->socket.sin_port = htons(portHead);
    
    int sinAdressResultHead = inet_pton(AF_INET, hostnameHead, &headTree->socket.sin_addr);

    if (sinAdressResultHead == -1){
        printf("AF INET nao contem um endereco valido\n");
        return NULL;
    } else if (sinAdressResultHead == 0) {
        printf("hostnameHead nao contem um endereco valido\n");
        return NULL;
    }

    int connectResultHead = network_connect(headTree);
    if (connectResultHead == -1) {
        printf("A conexão à head falhou\\n");
        return NULL;
    } 

    head = headTree;
    printf("\n---Conexão estabelecida com a nova head: %s---\n", headID);

    free(metaData);

}

void rtree_connect_tail(struct rtree_t* tailTree) {

    char* currentTailID = getTailID();

    if (strcmp(tailID, currentTailID) != 0) {
        if (tailTree->descriptor)
            close(tailTree->descriptor);
        strcpy(tailID, currentTailID);
    } else {
        return tail;
    }

    char* metaData = (char *)malloc(ZDATALEN * sizeof(char));
    char node_path[120] = "";
    strcat(node_path, "/chain/node");
    strcat(node_path, tailID); 
    int dataSizeRead = ZDATALEN;

    if (zoo_get(zh, node_path, 0, metaData, &dataSizeRead, NULL) != ZOK) {
        printf("zoo_get failed\n");
        exit(0);
    }

    if (tailTree == NULL) {
        printf("remote tree == NULL");
        free(tailTree);
        return NULL;
    }

    char* hostnameHead = strtok((char *) metaData, ":");
    int portHead = atoi(strtok(NULL, ":"));

    tailTree->socket.sin_family = AF_INET;
    tailTree->socket.sin_port = htons(portHead);
    
    int sinAdressResultHead = inet_pton(AF_INET, hostnameHead, &tailTree->socket.sin_addr);

    if (sinAdressResultHead == -1){
        printf("AF INET nao contem um endereco valido\n");
        return NULL;
    } else if (sinAdressResultHead == 0) {
        printf("hostnameHead nao contem um endereco valido\n");
        return NULL;
    }

    int connectResultHead = network_connect(tailTree);
    if (connectResultHead == -1) {
        printf("A conexão à head falhou\\n");
        return NULL;
    } 

    tail = tailTree;
    printf("---Conexão estabelecida com a nova tail: %s---\n", tailID);

    free(metaData);

}

/* Função para estabelecer uma associação entre o cliente e o servidor, 
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna NULL em caso de erro.
 */
struct rtree_t *rtree_connect(const char *address_port) {
    
    char* hostname = strtok((char *) address_port, ":");
    int port = atoi(strtok(NULL, ":"));

    struct rtree_t *remoteTree = (struct rtree_t *) malloc (sizeof(struct rtree_t));

    if (remoteTree == NULL) {
        printf("remote tree == NULL");
        free(remoteTree);
        return NULL;
    }

    remoteTree->socket.sin_family = AF_INET;
    remoteTree->socket.sin_port = htons(port);
    
    int sinAdressResult = inet_pton(AF_INET, hostname, &remoteTree->socket.sin_addr);

    if (sinAdressResult == -1){
        printf("AF INET nao contem um endereco valido");
        return NULL;
    } else if (sinAdressResult == 0) {
        printf("hostname nao contem um endereco valido");
        return NULL;
    }

    int connectResult = network_connect(remoteTree);
    if (connectResult == -1) {
        printf("network_connnect falhou");
        free(remoteTree);
        return NULL;
    } 

    printf("Conexão estabelecida. \n");

    return remoteTree;
    
}

/* Termina a associação entre o cliente e o servidor, fechando a 
 * ligação com o servidor e libertando toda a memória local.
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtree_disconnect(struct rtree_t *headTree, struct rtree_t *tailTree) {
    
    if (network_close(headTree) == -1) {
        printf("network_close == -1");
        return -1;
    }

    if (network_close(tailTree) == -1) {
        printf("network_close == -1");
        return -1;
    }

    free(headID);
    free(tailID);

    free(headTree);
    free(tailTree);

    for (int i = 0; i < children_list->count; i++) {
        free(children_list->data[i]);
    }

    free(children_list->data);
    free(children_list);

    return 0;

}

/* Verifica se a operação identificada por op_n foi executada.
*/
int rtree_verify(struct rtree_t *rtree, int op_n) {

    MessageT msg;
    message_t__init(&msg);

    msg.opcode = MESSAGE_T__OPCODE__OP_VERIFY; 
    msg.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
    msg.op_n = op_n;

    MessageT *msgReceived = network_send_receive(rtree, &msg);
    if(msgReceived == NULL) {
        printf("Erro dentro do rtree_verify1\n");
        return -1;
    }

    if(msgReceived->opcode == MESSAGE_T__OPCODE__OP_ERROR && msgReceived->c_type == MESSAGE_T__C_TYPE__CT_NONE ){
        message_t__free_unpacked(msgReceived, NULL);
        return -1;
    }
    
    int result = msgReceived->verifiedresult;
    message_t__free_unpacked(msgReceived, NULL);

    return result;
}

/* Função para adicionar um elemento na árvore.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok, em adição/substituição) ou -1 (problemas).
 */
int rtree_put(struct rtree_t *rtree, struct entry_t *entry){
    
    MessageT msg;  
    message_t__init(&msg);

    msg.entry = (MessageT__EntryT *) malloc (sizeof(MessageT__EntryT));

    if(msg.entry == NULL) {
        free(msg.entry);
        printf("msg.entry == NULL\n");
        return -1;
    }
    message_t__entry_t__init(msg.entry);

    msg.entry->data = (MessageT__DataT *) malloc (sizeof(MessageT__DataT));
     if(msg.entry->data == NULL) {
        free(msg.entry->data);
        printf("msg.entry->data == NULL\n");
        return -1;
    }
    
    message_t__data_t__init(msg.entry->data);

    msg.opcode = MESSAGE_T__OPCODE__OP_PUT; 
    msg.c_type = MESSAGE_T__C_TYPE__CT_ENTRY; 

    msg.entry->data->data = strdup(entry->value->data);
    msg.entry->key = strdup(entry->key);
    msg.entry->data->datasize = entry->value->datasize;


    MessageT *msgReceived = network_send_receive(rtree, &msg);
    if(msgReceived == NULL){
        printf("msgReceived == NULL\n");
        return -1;
    }
    
    free(msg.entry->data->data);
    free(msg.entry->data);
    free(msg.entry->key);
    free(msg.entry);

    if(msgReceived->opcode == MESSAGE_T__OPCODE__OP_PUT + 1 && msgReceived->c_type == MESSAGE_T__C_TYPE__CT_RESULT){
        int res = msgReceived->verifiedresult;
        message_t__free_unpacked(msgReceived, NULL);
        return res;
    } else{
        message_t__free_unpacked(msgReceived, NULL);
        return -1;
    }

}
    

/* Função para obter um elemento da árvore.
 * Em caso de erro, devolve NULL.
 */
struct data_t *rtree_get(struct rtree_t *rtree, char *key) {

    MessageT msg;
    message_t__init(&msg);

    msg.opcode = MESSAGE_T__OPCODE__OP_GET; 
    msg.c_type = MESSAGE_T__C_TYPE__CT_KEY;

    msg.entry = (MessageT__EntryT *) malloc (sizeof(MessageT__EntryT));

    if(msg.entry == NULL) {
        free(msg.entry);
        printf("msg.entry == NULL\n");
        return NULL;
    }

    message_t__entry_t__init(msg.entry);

    msg.entry->key = key;

    msg.entry->data = (MessageT__DataT *) malloc (sizeof(MessageT__DataT));
    
    if(msg.entry->data == NULL) {
        free(msg.entry->data);
        printf("msg.entry->data == NULL\n");
        return NULL;
    }

    message_t__data_t__init(msg.entry->data);

    MessageT *msgReceived = network_send_receive(rtree, &msg);
    if(msgReceived  == NULL) {
        return NULL;
    }

    struct data_t *dataResult = NULL;
    if(msgReceived->opcode == MESSAGE_T__OPCODE__OP_GET+1 && msgReceived->c_type == MESSAGE_T__C_TYPE__CT_VALUE){
        int d_size = msgReceived->entry->data->datasize;
        char *data = msgReceived->entry->data->data;
        if(msgReceived->entry->data->data == NULL || strlen(msgReceived->entry->data->data) == 0){
            dataResult = data_create2(d_size, NULL); 
        }
        else{
            dataResult = data_create2(d_size, strdup(data)); 
        }
    }

    else{

        message_t__free_unpacked(msgReceived, NULL);
        return data_create2(0,NULL);
    }

    message_t__free_unpacked(msgReceived, NULL);

    return dataResult;
}

/* Função para remover um elemento da árvore. Vai libertar 
 * toda a memoria alocada na respetiva operação rtree_put().
 * Devolve: 0 (ok), -1 (key not found ou problemas).
 */
int rtree_del(struct rtree_t *rtree, char *key){

    MessageT msg;
    message_t__init(&msg);

    msg.opcode = MESSAGE_T__OPCODE__OP_DEL; 
    msg.c_type = MESSAGE_T__C_TYPE__CT_KEY; 

    msg.entry = (MessageT__EntryT *) malloc (sizeof(MessageT__EntryT));

    if(msg.entry == NULL) {
        free(msg.entry);
        printf("msg.entry == NULL\n");
        return -1;
    }

    message_t__entry_t__init(msg.entry);
    
    msg.entry->key = key; 

    MessageT *msgReceived = network_send_receive(rtree, &msg);
    if(msgReceived == NULL){
        return -1;
    }

    if(msgReceived->opcode == MESSAGE_T__OPCODE__OP_DEL+1 && msgReceived->c_type == MESSAGE_T__C_TYPE__CT_RESULT){
        int res = msgReceived->verifiedresult;
        free(msg.entry);
        message_t__free_unpacked(msgReceived, NULL);
        return res;
    }

    entry_destroy(msg.entry);
    message_t__free_unpacked(msgReceived, NULL);
    return -1;
}

/* Devolve o número de elementos contidos na árvore.
 */
int rtree_size(struct rtree_t *rtree) {

    MessageT msg;
    message_t__init(&msg);

    msg.opcode = MESSAGE_T__OPCODE__OP_SIZE; 
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE; 

    struct _MessageT *msgReceived = network_send_receive(rtree, &msg);
    if(msgReceived == NULL){
        return -1;
    }

    if(msgReceived->opcode == MESSAGE_T__OPCODE__OP_SIZE + 1 && msgReceived->c_type == MESSAGE_T__C_TYPE__CT_RESULT){
        int size = msgReceived->size;
        message_t__free_unpacked(msgReceived, NULL);
        return size;
    }  

    message_t__free_unpacked(msgReceived, NULL);
    return -1;
}

/* Função que devolve a altura da árvore.
 */
int rtree_height(struct rtree_t *rtree) {

    MessageT msg;
    message_t__init(&msg);

    msg.opcode = MESSAGE_T__OPCODE__OP_HEIGHT; 
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE; 

    struct _MessageT *msgReceived = network_send_receive(rtree, &msg);
    if(msgReceived == NULL){
        return -1;
    }

    if(msgReceived->opcode == MESSAGE_T__OPCODE__OP_HEIGHT + 1 && msgReceived->c_type == MESSAGE_T__C_TYPE__CT_RESULT){

        int height = msgReceived->height;
        message_t__free_unpacked(msgReceived, NULL);
        return height;
    }  

    message_t__free_unpacked(msgReceived, NULL);
    return -1;
}

/* Devolve um array de char* com a cópia de todas as keys da árvore,
 * colocando um último elemento a NULL.
 */
char **rtree_get_keys(struct rtree_t *rtree) {

    MessageT msg;
    message_t__init(&msg);

    msg.opcode = MESSAGE_T__OPCODE__OP_GETKEYS; 
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE; 

    struct _MessageT *msgReceived = network_send_receive(rtree, &msg);
    if(msgReceived == NULL){
        return NULL;
    }

     if(msgReceived->opcode == MESSAGE_T__OPCODE__OP_GETKEYS + 1 && msgReceived->c_type == MESSAGE_T__C_TYPE__CT_KEYS){
        int len = msgReceived->n_repeatedkeys + 1;
        char **fully_getKeysArray = malloc(len * sizeof(char*) + 1);

        if(fully_getKeysArray == NULL) {
            printf("Erro malloc. fully_getKeysArray == NULL");
            free(fully_getKeysArray);
            return NULL;
        }

        for(int i = 0; i < len-1; i++ ){
            fully_getKeysArray[i] = strdup(msgReceived->repeatedkeys[i]);
        }

        message_t__free_unpacked(msgReceived, NULL);
        fully_getKeysArray[len-1] = NULL;

        return fully_getKeysArray;
    }

}

/* Devolve um array de void* com a cópia de todas os values da árvore,
 * colocando um último elemento a NULL.
 */
void **rtree_get_values(struct rtree_t *rtree) {

    MessageT msg;
    message_t__init(&msg);

    msg.opcode = MESSAGE_T__OPCODE__OP_GETVALUES; 
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE; 

    struct _MessageT *msgReceived = network_send_receive(rtree, &msg);
    if(msgReceived == NULL){
        return NULL;
    }

     if(msgReceived->opcode == MESSAGE_T__OPCODE__OP_GETVALUES + 1 && msgReceived->c_type == MESSAGE_T__C_TYPE__CT_VALUES){
        char **fully_getValuesArray = NULL;
        int len = msgReceived->n_repeatedkeys + 1;

        fully_getValuesArray = malloc(len * sizeof(void*) + 1);

        for(int i = 0; i < len-1; i++ ){
            fully_getValuesArray[i] = msgReceived->repeatedkeys[i];
        }

        fully_getValuesArray[len-1] = NULL;

        return fully_getValuesArray;
    }
}
