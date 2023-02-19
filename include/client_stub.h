/*
* Grupo SD-050
* João Santos nº 57103
* Paulo Bolinhas nº 56300
* Rui Martins nº 56283
*/

#ifndef _CLIENT_STUB_H
#define _CLIENT_STUB_H

#include "data.h"
#include "entry.h"
#include "zookeeper/zookeeper.h"



/* Remote tree. A definir pelo grupo em client_stub-private.h
 */
struct rtree_t;

void connection_watcher2(zhandle_t *zzh, int type, int state, const char *path, void *watcherCtx);

static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);

char* getHeadID();

char* getTailID();

void rtree_connectZooKeeper(const char *address_port);

void rtree_connect_head(struct rtree_t* headTree);

void rtree_connect_tail(struct rtree_t* tailTree) ;

/* Função para estabelecer uma associação entre o cliente e o servidor, 
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna NULL em caso de erro.
 */
struct rtree_t *rtree_connect(const char *address_port);

/* Termina a associação entre o cliente e o servidor, fechando a 
 * ligação com o servidor e libertando toda a memória local.
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtree_disconnect(struct rtree_t *headTree, struct rtree_t *tailTree);

/* Verifica se a operação identificada por op_n foi executada.
*/
int rtree_verify(struct rtree_t *rtree, int op_n);

/* Função para adicionar um elemento na árvore.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok, em adição/substituição) ou -1 (problemas).
 */
int rtree_put(struct rtree_t *rtree, struct entry_t *entry);

/* Função para obter um elemento da árvore.
 * Em caso de erro, devolve NULL.
 */
struct data_t *rtree_get(struct rtree_t *rtree, char *key);

/* Função para remover um elemento da árvore. Vai libertar 
 * toda a memoria alocada na respetiva operação rtree_put().
 * Devolve: 0 (ok), -1 (key not found ou problemas).
 */
int rtree_del(struct rtree_t *rtree, char *key);

/* Devolve o número de elementos contidos na árvore.
 */
int rtree_size(struct rtree_t *rtree);

/* Função que devolve a altura da árvore.
 */
int rtree_height(struct rtree_t *rtree);

/* Devolve um array de char* com a cópia de todas as keys da árvore,
 * colocando um último elemento a NULL.
 */
char **rtree_get_keys(struct rtree_t *rtree);

/* Devolve um array de void* com a cópia de todas os values da árvore,
 * colocando um último elemento a NULL.
 */
void **rtree_get_values(struct rtree_t *rtree);


#endif
