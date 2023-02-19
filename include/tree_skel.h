/*
* Grupo SD-050
* João Santos nº 57103
* Paulo Bolinhas nº 56300
* Rui Martins nº 56283
*/

#ifndef _TREE_SKEL_H
#define _TREE_SKEL_H

#include "../sdmessage.pb-c.h"
#include "tree.h"

extern int toShutDown;

struct op_proc {
    int max_proc;
    int* in_progress;
    //regista o identificador das operações de escrita que estão a ser atendidas por um conjunto de threads dedicadas às escritas. 
    //Ou seja, enquanto uma thread está a executar uma operação de escrita, o identificador da operação permanece armazenado em in_progress
};

struct request_t {
    int op_n; //o número da operação
    int op; //a operação a executar. op=0 se for um delete, op=1 se for um put
    char* key; //a chave a remover ou adicionar
    struct data_t* data; // os dados a adicionar em caso de put, ou NULL em caso de delete
    struct request_t *next; //pointer to the next element of the queue
    //adicionar campo(s) necessário(s) para implementar fila do tipo produtor/consumidor
};


/*
* Watcher function for connection state change events
*/
// void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context);
/*Função auxiliar que termina as threads e o programa
*/
void auxShutDown();


int updateNextServer();

int connectZooKeeper(char* portServer, char* zooKeeperIP);

/* Inicia o skeleton da árvore.
* O main() do servidor deve chamar esta função antes de poder usar a
* função invoke(). 
* A função deve lançar N threads secundárias responsáveis por atender 
* pedidos de escrita na árvore.
* Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY)
*/
int tree_skel_init(int N);

/* Função da thread secundária que vai processar pedidos de escrita.
*/
void * process_request (void *params);

/* 
Verifica se a operação identificada por op_n foi executada.
Retorna 1 se já tiver sido, 0 se não e -1 se op_n for um id que nem sequer foi retornado ao cliente
*/
int verify(int op_n);

/*Produz pedido para a fila de pedidos
*/
void addQueue(struct request_t *request);

/*Consome pedido da fila de pedidos
*/
struct request_t* queue_get_request();

/* Liberta toda a memória e recursos alocados pela função tree_skel_init.
 */
void tree_skel_destroy();

/* Executa uma operação na árvore (indicada pelo opcode contido em msg)
 * e utiliza a mesma estrutura message_t para devolver o resultado.
 * Retorna 0 (OK) ou -1 (erro, por exemplo, árvore nao incializada)
*/
int invoke(struct _MessageT *msg);

#endif
