/*
* Grupo SD-050
* João Santos nº 57103
* Paulo Bolinhas nº 56300
* Rui Martins nº 56283
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "../sdmessage.pb-c.h"
#include "../include/tree.h"
#include "../include/tree_skel.h"
#include "../include/network_server.h"
#include "../include/client_stub-private.h"
#include "../include/client_stub.h"
#include <zookeeper/zookeeper.h>


#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/ioctl.h>
#include <net/if.h>


#define TRUE 1
#define FALSE 0

struct tree_t *tree;
struct rtree_t* next_server = NULL;
char nextServerpath[120] = "/chain/";
int nextServerID;

char* current_path;

int toShutDown = FALSE;

int last_assigned;
struct request_t *queue_head = NULL;
struct op_proc *processingOPs;
int threadsNumber;

pthread_mutex_t procOpMutex;
pthread_mutex_t queueMutex;
pthread_mutex_t treeMutex;

pthread_cond_t queueNotEmpty;
pthread_t *threads;

/* ZooKeeper Znode Data Length (1MB, the max supported) */
#define ZDATALEN 1024 * 1024

typedef struct String_vector zoo_string;
int is_connected;
const char* chain_root = "/chain"; 
static char *watcher_ctx = "ZooKeeper Data Watcher";
static zhandle_t *zh;
static zoo_string* children_list;

/* Caso erro mensagem
*/
void errorCase (MessageT *msg){
    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
}

/*Função auxiliar que termina as threads e o programa
*/
void auxShutDown() {
    toShutDown = TRUE;

    pthread_mutex_lock(&queueMutex);
    pthread_cond_broadcast(&queueNotEmpty);  
    pthread_mutex_unlock(&queueMutex);

    for (int i=0; i < threadsNumber; i++){	
		if (pthread_join(threads[i], NULL) != 0){
			perror("Erro no join.\n");
			exit(0);
		}
	}
    tree_skel_destroy();
    network_server_close();
    exit(0);
}

/*
* Watcher function for connection state change events
*/
void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context) {
	if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTED_STATE) {
			is_connected = 1; 
		} else {
			is_connected = 0; 
		}
	}
}

/**
* Children Watcher
*/
static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
    printf("\nLista de Znodes alterada, imprimindo a nova...\n");
	if (state == ZOO_CONNECTED_STATE)	 {
		if (type == ZOO_CHILD_EVENT) {
	 	   /* Get the updated children and reset the watch */ 
 			if (ZOK != zoo_wget_children(zh, chain_root, child_watcher, watcher_ctx, children_list)) {
 				fprintf(stderr, "Error setting watch at %s!\n", chain_root);
 			}

            updateNextServer();

			fprintf(stderr, "\n=== Nova Lista de Znodes=== [ %s ]", chain_root); 
			for (int i = 0; i < children_list->count; i++)  {
				fprintf(stderr, "\n(%d): %s", i+1, children_list->data[i]);
			}
			fprintf(stderr, "\n=== Feito ===\n");
		 } 
	 }
}

int updateNextServer() {

    if (children_list->count == 1) {
        next_server = NULL;
        return 0;
    }

    int j = 0;
    int finalNextNodeID = NULL;
    int finalNodePos = 0;
    
    char* currentZNodeID = strtok(current_path, "/chain/node");

    for (j = 0; j < children_list->count; j++) { 
        
        int current = atoi(strtok(children_list->data[j], "node"));

        if (current == atoi(currentZNodeID))
            continue;

        if (finalNextNodeID == NULL && current > atoi(currentZNodeID)) {
            finalNextNodeID = current;
            finalNodePos = j;
            continue;
        }

        if (finalNextNodeID != NULL && current > atoi(currentZNodeID) && current < finalNextNodeID) { // o maior dos mais pequenos
            finalNextNodeID = current;
            finalNodePos = j;
        }
    }

    if (atoi(currentZNodeID) < finalNextNodeID && finalNextNodeID != nextServerID) { // encontra novo server e da close ao ultimo
        if (next_server) {
            close(next_server->descriptor);
            free(next_server);
        }

        next_server = (struct rtree_t *) malloc (sizeof(struct rtree_t));

        char* metaData = (char *)malloc(ZDATALEN * sizeof(char));

        strcpy(nextServerpath, "/chain/");
        strcat(nextServerpath, children_list->data[finalNodePos]);
        int dataSizeRead = ZDATALEN;

        if (zoo_get(zh, nextServerpath, 0, metaData, &dataSizeRead, NULL) != ZOK) {
            printf("MetaData: %s\n", metaData);
            printf("zoo_get failed\n");
            exit(0);
        }
        
        if (dataSizeRead == -1) {
            printf("DataSizeRead == -1\n");
            exit(0);
        }

        char* hostname = strtok((char *) metaData, ":");
        int port = atoi(strtok(NULL, ":"));

        next_server->socket.sin_family = AF_INET;
        next_server->socket.sin_port = htons(port);
        
        int sinAdressResult = inet_pton(AF_INET, hostname, &next_server->socket.sin_addr);

        if (sinAdressResult == -1){
            printf("AF INET nao contem um endereco valido");
            return -1;
        } else if (sinAdressResult == 0) {
            printf("hostname nao contem um endereco valido");
            return -1;
        }

        next_server->descriptor = socket(AF_INET, SOCK_STREAM, 0);
        if (next_server->descriptor == -1) {
            printf("descriptor == -1\n");
            return -1;
        }

        while(1) {
            if (connect(next_server->descriptor, (struct sockaddr*) &next_server->socket, sizeof(next_server->socket)) != -1)
                break;
        }
        
        nextServerID = atoi(strtok(nextServerpath, "/chain/node"));
        printf("Conexão com o servidor seguinte estabelecida. \n");

        free(metaData);
    } else if (finalNextNodeID == 0) {
        printf("Não existe um servidor mais alto\n");
        next_server = NULL;
    } else {
        printf("O servidor seguinte não foi alterado\n");
    }
    
}

int connectZooKeeper(char* portServer, char* zooKeeperIP) {

    children_list =	(zoo_string *) malloc(sizeof(zoo_string));

    zh = zookeeper_init(zooKeeperIP, connection_watcher, 2000, 0, NULL, 0);
	if (zh == NULL)	{
		fprintf(stderr, "Error connecting to ZooKeeper server!\n");
	    return -1;
	} else {
        is_connected = 1;
    }

    if (ZNONODE == zoo_exists(zh, chain_root, 0, NULL)) {
        if (ZOK == zoo_create(zh, chain_root, NULL, -1, & ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0)) {
            fprintf(stderr, "%s created!\n", chain_root);
        } else {
            fprintf(stderr,"Error Creating %s!\n", chain_root);
            return -1;
        } 
    }
    
    struct hostent *hostent;
    char buffer_host[256];
    char* myIP;
    gethostname(buffer_host,sizeof(buffer_host));
    hostent = gethostbyname(buffer_host);
    myIP = inet_ntoa(*((struct in_addr*) hostent->h_addr_list[0]));
    strcat(myIP,":");
    strcat(myIP,portServer);    

    int new_path_len = 1024;
    current_path = malloc(new_path_len);

    if (ZOK != zoo_create(zh, "/chain/node", NULL, -1, & ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL | ZOO_SEQUENCE, current_path, new_path_len)) {
        fprintf(stderr, "Error creating znode from path /chain/node\n");
        return -1;
    }

    if (zoo_set(zh, current_path, myIP, strlen(myIP), -1) != ZOK) { // escreve no path o que tem no buffer
        fprintf(stderr, "Error in write at: %s\n", current_path);
        return -1;
    }

    fprintf(stderr, "No efemero criado! ZNode path: %s\n", current_path);

    if (ZOK != zoo_wget_children(zh, chain_root, &child_watcher, watcher_ctx, children_list)) {
        fprintf(stderr, "Error setting watch at %s!\n", chain_root);
    }

    if (updateNextServer() == -1) {
        printf("---Update_next server deu erro no connect zookeeper---\n");
        return -1;
    }

    fprintf(stderr, "\n=== Nova Lista de Znodes=== [ %s ]", chain_root); 
    for (int i = 0; i < children_list->count; i++)  {
        fprintf(stderr, "\n(%d): %s", i+1, children_list->data[i]);
    }
    fprintf(stderr, "\n=== Feito ===\n");

    return 0;
}

/* Inicia o skeleton da árvore.
* O main() do servidor deve chamar esta função antes de poder usar a
* função invoke(). 
* A função deve lançar N threads secundárias responsáveis por atender 
* pedidos de escrita na árvore.
* Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY)
*/
int tree_skel_init(int N) {
    
    threadsNumber = N;
    last_assigned = 1;

    if (pthread_mutex_init(&procOpMutex,NULL) != 0) {
        printf("Mutex procOpMutex nao criado");
        return -1;
    }
    
    if (pthread_mutex_init(&queueMutex,NULL) != 0) {
        printf("Mutex procOpMutex nao criado");
        return -1;
    }

    if (pthread_mutex_init(&treeMutex,NULL) != 0) {
        printf("Mutex procOpMutex nao criado");
        return -1;
    }

    pthread_cond_init(&queueNotEmpty, NULL);

    processingOPs = malloc(sizeof(struct op_proc));
    if (processingOPs == NULL) {
        printf("processingOPs eh nula");
        return -1;
    }

    processingOPs->in_progress = malloc(N*sizeof(int));
    processingOPs->max_proc = -1;
    
    if (processingOPs->in_progress == NULL) {
        printf("in_progress eh nula");
        return -1;
    }

    for (int i = 0; i < threadsNumber; i++) {
        processingOPs->in_progress[i] = 0;
    }

    threads = malloc(threadsNumber*sizeof(pthread_t));
	int thread_param[threadsNumber];
    int i;
    for (i = 0; i < threadsNumber; i++) {
        thread_param[i] = i;
        if (pthread_create(&threads[i], NULL, &process_request, (void*) thread_param[i]) != 0) {
            printf("thread %d failed to create\n", i);
            return -1;
        } 
    }

    tree = tree_create();

    if(tree == NULL)
        return -1;
	

    return 0;
}

/* Função da thread secundária que vai processar pedidos de escrita.
*/
void * process_request (void *params) {
    int i = (int) params;

    while (1) {

        pthread_mutex_lock(&queueMutex);

        while (queue_head == NULL && toShutDown == FALSE)
            pthread_cond_wait(&queueNotEmpty, &queueMutex);

        if (toShutDown == TRUE) {
            pthread_mutex_unlock(&queueMutex); 
            pthread_exit(NULL);
        }
        
        struct request_t* currentRequest = queue_get_request();
        
        pthread_mutex_unlock(&queueMutex);

        processingOPs->in_progress[i] = currentRequest->op_n;
        

        if (currentRequest->op == 1) { //put

            pthread_mutex_lock(&treeMutex);

            tree_put(tree,currentRequest->key,currentRequest->data);
            pthread_mutex_unlock(&treeMutex);

            if (next_server != NULL) {
                struct data_t *dataC = data_create2(currentRequest->data->datasize + 1, strdup(currentRequest->data->data));
                struct entry_t *entryToPut = entry_create(strdup(currentRequest->key), dataC);
                int res = rtree_put(next_server, entryToPut);

                data_destroy(dataC);
                free(entryToPut->key);
                free(entryToPut);
            }

        } else if (currentRequest->op == 0){ //del

            pthread_mutex_lock(&treeMutex);
            tree_del(tree, currentRequest->key);
            pthread_mutex_unlock(&treeMutex); 

            if (next_server != NULL) {
                rtree_del(next_server, currentRequest->key);
            }

        }
        
        pthread_mutex_lock(&procOpMutex);
        if (currentRequest->op_n > processingOPs->max_proc)
            processingOPs->max_proc = currentRequest->op_n;
        pthread_mutex_unlock(&procOpMutex);

        processingOPs->in_progress[i] = 0;
        
        if (currentRequest->data != NULL)
            data_destroy(currentRequest->data);
        free(currentRequest->key);
        free(currentRequest);
    }
}

/*Produz pedido para a fila de pedidos
*/
void addQueue(struct request_t *request) {
     
    if (queue_head == NULL) {

        queue_head = request;
        request->next = NULL;

    } else {
        struct request_t *currentRequest = queue_head;
        
        while (currentRequest->next != NULL) 
            currentRequest = currentRequest->next;
        
        currentRequest->next = request;
        request->next = NULL;

    }
    pthread_cond_broadcast(&queueNotEmpty);  

}

/*Consome pedido da fila de pedidos
*/
struct request_t* queue_get_request() {
    
    struct request_t *currentRequest = queue_head;
    queue_head = currentRequest->next;
    
    return currentRequest;

}

/* 
Verifica se a operação identificada por op_n foi executada.
Retorna 1 se já tiver sido, 0 se não e -1 se op_n for um id que nem sequer foi retornado ao cliente
*/
int verify(int op_n) {

    if (op_n == processingOPs->max_proc) {
        return 1;
    }

    if (op_n < processingOPs->max_proc) {
        for (int i = 0; i < threadsNumber; i++) {
            if (op_n == processingOPs->in_progress[i])
                return 0;
        }
    }

    if (op_n < last_assigned)
        return 1;
    
    if (op_n > last_assigned - 1)
        return -1; //erro porque é impossivel ter sido executada

    return 0; //ainda nao foi executada
}

/* Liberta toda a memória e recursos alocados pela função tree_skel_init.
 */
void tree_skel_destroy(){

    pthread_mutex_destroy(&procOpMutex);
    pthread_mutex_destroy(&queueMutex);
    pthread_mutex_destroy(&treeMutex);

    pthread_cond_destroy(&queueNotEmpty);

    free(current_path);
    free(next_server);
	
    for (int i = 0; i < children_list->count; i++) {
        free(children_list->data[i]);
    }

    free(children_list->data);
    free(children_list);
    
    free(threads);
    free(processingOPs->in_progress);
    free(processingOPs);

    tree_destroy(tree);
}


/* Executa uma operação na árvore (indicada pelo opcode contido em msg)
 * e utiliza a mesma estrutura message_t para devolver o resultado.
 * Retorna 0 (OK) ou -1 (erro, por exemplo, árvore nao incializada)
*/
int invoke(struct _MessageT *msg){

    if(msg == NULL || tree == NULL)
        return -1;

    if(msg->opcode == MESSAGE_T__OPCODE__OP_PUT){
        
        if(msg->entry->data->data == NULL) {
                errorCase(msg);
                return -1;
        }

        struct data_t *data = data_create2(msg->entry->data->datasize, strdup(msg->entry->data->data));
        if(msg->c_type == MESSAGE_T__C_TYPE__CT_ENTRY){

            struct request_t *currentPut = malloc(sizeof(struct request_t));

            if (currentPut == NULL) {
                errorCase(msg);
                printf("currentPut == NULL");
                return -1;
            }

            currentPut->key = malloc(sizeof(msg->entry->key));

            if (currentPut->key == NULL) {
                errorCase(msg);
                printf("currentPut == NULL");
                return -1;
            }

            currentPut->op_n = last_assigned;
            currentPut->op = 1; //1 for put
            strcpy(currentPut->key, msg->entry->key);
            currentPut->data = data_dup(data);
            int current = currentPut->op_n;
            msg->verifiedresult = current;

            

            pthread_mutex_lock(&queueMutex);
            addQueue(currentPut);
            pthread_mutex_unlock(&queueMutex); 

            msg->opcode = MESSAGE_T__OPCODE__OP_PUT + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            last_assigned++;
            
            data_destroy(data); 

            return 0;
        }

    } else if (msg->opcode == MESSAGE_T__OPCODE__OP_GET){

        if(msg->c_type == MESSAGE_T__C_TYPE__CT_KEY){

            if(msg->entry->key == NULL){
                errorCase(msg);
                return -1;
            }

            pthread_mutex_lock(&treeMutex);
            struct data_t *data = tree_get(tree,msg->entry->key);
            pthread_mutex_unlock(&treeMutex);

            if(data == NULL){
                errorCase(msg);
                msg->entry->data->data = NULL;
                msg->entry->data->datasize = 0;
                data_destroy(data);
                return 0;
            }

            msg->opcode = MESSAGE_T__OPCODE__OP_GET + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;

            msg->entry->data->data = strdup(data->data);
            msg->entry->data->datasize = data->datasize;
            data_destroy(data);

            return 0;
        }

    } else if (msg->opcode == MESSAGE_T__OPCODE__OP_DEL){

        if(msg->c_type == MESSAGE_T__C_TYPE__CT_KEY){

            struct request_t *currentDel = malloc(sizeof(struct request_t));

            if (currentDel == NULL) {
                errorCase(msg);
                printf("currentDel == NULL");
                return -1;
            }

            currentDel->key = malloc(sizeof(msg->entry->key));

            if (currentDel->key == NULL) {
                errorCase(msg);
                printf("currentDel == NULL");
                return -1;
            }

            currentDel->op_n = last_assigned;
            currentDel->op = 0; //0 for del
            strcpy(currentDel->key, msg->entry->key);
            currentDel->data = NULL;
            int current = currentDel->op_n;
            msg->verifiedresult = current;

            pthread_mutex_lock(&queueMutex);
            addQueue(currentDel);
            pthread_mutex_unlock(&queueMutex);  
            
            msg->opcode = MESSAGE_T__OPCODE__OP_DEL + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            last_assigned++;

            return 0;
        }

    } else if (msg->opcode == MESSAGE_T__OPCODE__OP_SIZE){

        if(msg->c_type == MESSAGE_T__C_TYPE__CT_NONE){

            pthread_mutex_lock(&treeMutex);
            int size = tree_size(tree);
            pthread_mutex_unlock(&treeMutex);

            msg->opcode = MESSAGE_T__OPCODE__OP_SIZE + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg->size = size;

            return 0;
        }

    } else if (msg->opcode == MESSAGE_T__OPCODE__OP_HEIGHT){

        if(msg->c_type == MESSAGE_T__C_TYPE__CT_NONE){
            
            pthread_mutex_lock(&treeMutex);
            msg->height = tree_height(tree);
            pthread_mutex_unlock(&treeMutex);

            msg->opcode = MESSAGE_T__OPCODE__OP_HEIGHT + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;

            return 0;
        }

    } else if (msg->opcode == MESSAGE_T__OPCODE__OP_GETKEYS){

        if (msg->c_type == MESSAGE_T__C_TYPE__CT_NONE) {

            pthread_mutex_lock(&treeMutex);
            msg->n_repeatedkeys = tree_size(tree);
            msg->repeatedkeys = tree_get_keys(tree);
            pthread_mutex_unlock(&treeMutex);

            msg->opcode = MESSAGE_T__OPCODE__OP_GETKEYS + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;

            return 0;
        }

    } else if (msg->opcode == MESSAGE_T__OPCODE__OP_GETVALUES){

        if (msg->c_type == MESSAGE_T__C_TYPE__CT_NONE) {
            
            pthread_mutex_lock(&treeMutex);
            msg->n_repeatedkeys = tree_size(tree);
            void ** getValuesArray = tree_get_values(tree);
            pthread_mutex_unlock(&treeMutex);

            int len = msg->n_repeatedkeys + 1;
            char ** arrayToSend = malloc(len * sizeof(char) + 1);

            if(arrayToSend == NULL) {
                printf("Erro malloc: arrayToSend == NULL");
                free(arrayToSend);
                return -1;
            }
          
            for(int i = 0; i < len-1; i++ ){
                struct data_t* dataNode = (getValuesArray[i]);
                char * string = strdup(dataNode->data);
                arrayToSend[i] = string;
            }

            msg->opcode = MESSAGE_T__OPCODE__OP_GETVALUES +1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_VALUES;
            msg->repeatedkeys = arrayToSend;
            
            return 0;
        }

    } else if(msg->opcode == MESSAGE_T__OPCODE__OP_VERIFY) {

        if (msg->c_type == MESSAGE_T__C_TYPE__CT_RESULT) {

            pthread_mutex_lock(&procOpMutex);
            pthread_mutex_lock(&treeMutex);
            int result = verify(msg->op_n);
            pthread_mutex_unlock(&treeMutex);
            pthread_mutex_unlock(&procOpMutex);

            
            if (result == -1) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                msg->verifiedresult = result;
                return 0;
            }

            msg->opcode = MESSAGE_T__OPCODE__OP_VERIFY+1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg->verifiedresult = result;

            return 0;
        }

    } else if(msg->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        return -1;
    }    
    
    return -1;

}
