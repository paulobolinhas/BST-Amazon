/*
* Grupo SD-050
* João Santos nº 57103
* Paulo Bolinhas nº 56300
* Rui Martins nº 56283
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "../include/data.h"
#include "../include/entry.h"

/* Função que cria uma entry, reservando a memória necessária para a
 * estrutura e inicializando os campos key e value, respetivamente, com a
 * string e o bloco de dados passados como parâmetros, sem reservar
 * memória para estes campos.
 */
struct entry_t *entry_create(char *key, struct data_t *data) {

    struct entry_t* current = malloc(sizeof(struct entry_t));

    if (current == NULL) {
        free(current);
        return NULL;
    }

    current->key = key;
    current->value = data;
    
    return current;

 }

/* Função que elimina uma entry, libertando a memória por ela ocupada
 */
void entry_destroy(struct entry_t *entry) {

    if (entry != NULL) {
        if (entry->value != NULL)
            data_destroy(entry->value);
        free(entry->key);
        free(entry);
    }
    return;
}

/* Função que duplica uma entry, reservando a memória necessária para a
 * nova estrutura.
 */
struct entry_t *entry_dup(struct entry_t *entry) {

    if(entry == NULL)
        return NULL;

    char *key = strdup(entry->key);
    struct data_t *data = data_dup(entry->value);
    struct entry_t *dup_entry = entry_create(key, data);
    return dup_entry;
  }

/* Função que substitui o conteúdo de uma entrada entry_t.
*  Deve assegurar que destroi o conteúdo antigo da mesma.
*/
void entry_replace(struct entry_t *entry, char *new_key, struct data_t *new_value){
    
    // data_destroy(entry->value);
    // free(entry->key);
    entry->key = new_key;
    entry->value = new_value;
}

/* Função que compara duas entradas e retorna a ordem das mesmas.
*  Ordem das entradas é definida pela ordem das suas chaves.
*  A função devolve 0 se forem iguais, -1 se entry1<entry2, e 1 caso contrário.
*/
int entry_compare(struct entry_t *entry1, struct entry_t *entry2){
    
    int comparator = strcmp(entry1->key,entry2->key);

    if (comparator == 0) 
        return 0;
    else if (comparator > 0)
        return 1;
        
    return -1;

}
