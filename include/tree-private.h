/*
* Grupo SD-050
* João Santos nº 57103
* Paulo Bolinhas nº 56300
* Rui Martins nº 56283
*/

#ifndef _TREE_PRIVATE_H
#define _TREE_PRIVATE_H

#include "entry.h"
#include "tree.h"

/* Estrutura que define a árvore de dados
*/
struct tree_t {
    
    struct entry_t *entry;
    struct tree_t *right_child;
    struct tree_t *left_child;
    int size;

};


/* Funçao que insere uma entry e a respetiva key na tree. Retorna 0 ou 1 (ok), -1 em caso de erro.
*/
int put_aux(struct tree_t *tree, struct entry_t *current_entry, char* key, int size);


/* Função para remover o elemento da árvore associado ao conteúdo da entrada current_entry.
 * Retorna 0 (ok), -1 caso contrário.
 */
int delete_aux(struct tree_t* tree, struct entry_t* current_entry);


/* Função para obter a árvore associada ao conteúdo da entrada current_entry. 
 * Retorna NULL em caso de erro.
 */
struct tree_t* tree_search(struct tree_t* tree, struct entry_t* current_entry);


/* Função que forma um array composto pelas keys existentes na àrvore.
 */
void key_put(struct tree_t* node, char **keys, int value);


/* Função que forma um array composto pelos values existentes na àrvore.
*/
void values_put(struct tree_t *node, void **values, int value);


/* Função para libertar a memória associada a uma árvore.
 */
void nodes_destroy(struct tree_t *node);

/* Função para obter o menor valor da árvore.
*/
struct tree_t* get_min(struct tree_t* tree);

/* Função para obter o máximo dos dois argumentos.
*/
int max(int x, int y);

#endif