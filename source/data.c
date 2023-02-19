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

/* Função que cria um novo elemento de dados data_t, reservando a memória
 * necessária para armazenar os dados, especificada pelo parâmetro size
 */
struct data_t *data_create(int size) {

    struct data_t *current = (struct data_t*) malloc(sizeof(struct data_t));

    if (current == NULL) {
        free(current);
        return NULL;
    }
    if (size <= 0) {
        free(current);
        return NULL;
    }
   
    current->datasize = size;
    current->data = (void*) malloc(size);

    if (current->data == NULL) {
        free(current->data);
        return NULL;
    }

    return current;
};

/* Função que cria um novo elemento de dados data_t, inicializando o campo
 * data com o valor passado no parâmetro data, sem necessidade de reservar
 * memória para os dados.
 */
struct data_t *data_create2(int size, void *data) {

    if (data == NULL)
        return NULL;

    struct data_t *current = malloc(sizeof(struct data_t));

    if (current == NULL || size <= 0) {
        free(current);
        return NULL;
    }

    current->datasize = size;
    current->data = data;

    return current;
};

/* Função que elimina um bloco de dados, apontado pelo parâmetro data,
 * libertando toda a memória por ele ocupada.
 */
void data_destroy(struct data_t *data) {
    if(data != NULL){
        if(data->data != NULL)
            free(data->data);
        free(data);
    }
}

/* Função que duplica uma estrutura data_t, reservando toda a memória
 * necessária para a nova estrutura, inclusivamente dados.
 */
struct data_t *data_dup(struct data_t *data) {

    if(data == NULL || data->data == NULL || data->datasize <= 0){
        return NULL;
    }

    struct data_t *cloned = data_create(data->datasize);

    if(cloned == NULL || data->datasize <= 0) {
        free(cloned);
        return NULL;
    }

    memcpy(cloned->data, data->data, data->datasize);

    return cloned;
}

/* Função que substitui o conteúdo de um elemento de dados data_t.
*  Deve assegurar que destroi o conteúdo antigo do mesmo.
*/
void data_replace(struct data_t *data, int new_size, void *new_data) {
    free(data->data);
    data->datasize = new_size;
    data->data = new_data;
}


