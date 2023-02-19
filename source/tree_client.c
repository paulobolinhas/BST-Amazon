/*
* Grupo SD-050
* João Santos nº 57103
* Paulo Bolinhas nº 56300
* Rui Martins nº 56283
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../include/client_stub.h"
#include "../include/client_stub-private.h"
#include "../include/entry.h"
#include "../include/data.h"

struct rtree_t* headTree;
struct rtree_t* tailTree;

int main(int argc, char *argv[]) {

    //este IP e porto passam a ser do zookeeper
    if(argc != 2) {
        printf("Sintaxe do comando de execução errada. Forma correta: ./tree_client <localhost>:<port>\n");
        return -1;
    }

    rtree_connectZooKeeper(argv[1]);

    headTree = (struct rtree_t *) malloc (sizeof(struct rtree_t));
    tailTree = (struct rtree_t *) malloc (sizeof(struct rtree_t));

    rtree_connect_head(headTree);

    if (headTree == NULL) {
        printf("erro ao connectar ao servidor");
        return -1;
    }

    rtree_connect_tail(tailTree);


    if (tailTree == NULL) {
        printf("erro ao connectar ao servidor");
        rtree_disconnect(headTree, tailTree);
        return -1;
    }

    printf("\n------ INTERFACE DO CLIENTE ------\n");
    
    printf("Acoes disponiveis:\n");
    printf("size - retorna tamanho da arvore\n");
    printf("height - retorna altura da  arvore\n");
    printf("del <key> - apaga o no com a chave = key\n");
    printf("get <key> - retorna o no com a chave = key\n");
    printf("put <key> <data> - coloca o no com a chave (key) e valor (data) na arvore\n");
    printf("getkeys - retorna todas as chaves da arvore\n");
    printf("getvalues - retorna todos os valores da arvore\n");
    printf("verify <identificador> - retorna se uma operacao com o identificador indicado foi executada\n");
    printf("quit - para a execução do programa\n\n");


    while (1) {

        char line[150];
        int res;
        char *key; 
        char *data;

        printf("INSIRA UMA ACCAO:\n");
        fgets(line, 150, stdin);

        char *action = strtok(line, " ");

        if (strcmp(action, "size\n") == 0) {
            
            res = rtree_size(tailTree);
            if (res >= 0)
                printf("Tamanho da árvore %d\n\n", res);
            else
                printf("Tamanho da árvore negativo\n");

        } else if (strcmp(action, "height\n") == 0) {

            res = rtree_height(tailTree);
            if (res >= 0)
                printf("Altura da árvore %d\n\n", res);
            else
                printf("Altura da árvore negativo\n");

        } else if (strcmp(action, "del") == 0) { 
            
            key = strtok(NULL, " ");
            key[strcspn(key, "\n")] = '\0';

            res = rtree_del(headTree, key);
            if (res > 0)
                printf("A operação %d de delete da key %s vai ser realizada assim que possível.\n\n", res,key);
            else
                printf("Key não encontrada ou ocorrencia de problemas relacionados com esta.\n\n");

        } else if (strcmp(action, "get") == 0) {
            
            key = strtok(NULL, " ");
            key[strcspn(key, "\n")] = '\0'; 
            
            sleep(1);
            
            struct data_t *dataRes = rtree_get(tailTree, key);
            if (dataRes != NULL)
                printf("\nKey obtida com sucesso.\nKey: %s\nValue: %s\n\n", key, dataRes->data);
            else
                printf("Key não existente.\n\n");

        } else if (strcmp(action, "put") == 0) {

            key = strtok(NULL, " ");

            char *keyN;
            keyN = malloc (sizeof (key) + 1);
            strcpy (keyN, key);

            data = strtok(NULL, "\n");

            if(key == NULL || data == NULL) {
                free(keyN);
                printf("Input inválido.\n\n");
                continue;
            }

            struct data_t *dataC = data_create2(strlen(data) + 1, strdup(data));
            struct entry_t *entryToPut = entry_create(strdup(keyN), dataC);
            res = rtree_put(headTree, entryToPut);

            if (res > 0)
                printf("A operação %d de colocar/substituir a Entry vai ser realizada assim que possivel.\n\n", res);
            else
                printf("Entry nao colocada com sucesso.\n\n");
            
            sleep(2);
            int result = rtree_verify(tailTree, res);
            if (result > 0) {
                printf("A operacao FOI propagada ate a tail com sucesso.\n\n");
            } else if (result == -1){
                printf("Ocorreu um erro, tente novamente.\n\n");
            } else {
                printf("A operacao NAO FOI propagada ate a tail com sucesso.\n\n");
            }

            free(keyN);
            data_destroy(dataC);
            free(entryToPut->key);
            free(entryToPut);

        } else if (strcmp(action, "getkeys\n") == 0) {

            char** keysRes = rtree_get_keys(tailTree);

            if (keysRes != NULL) {

                if(keysRes[0] == NULL) {
                    printf("\nNão existem keys para apresentar.\n\n");
                } else {
                    printf("\nKeys obtidas com sucesso!\n");
                    
                    printf("Keys: ");

                    for(int i = 0; keysRes[i] != NULL; i++) {

                        if(keysRes[i+1] != NULL) {
                            printf("%s, ", keysRes[i]);
                        } else {
                            printf("%s", keysRes[i]);
                        }
                    }
                    printf("\n\n");
                }

                int i = 0;
                while (keysRes[i] != NULL) {
                    free(keysRes[i]);
                    i++;
                }
                free(keysRes);

            } else {
                printf("Keys nao obtidas com sucesso.\n");
            }


            
        } else if (strcmp(action, "getvalues\n") == 0) {

            void** valuesRes = rtree_get_values(tailTree);

            if (valuesRes != NULL) {

                if(valuesRes[0] == NULL) {
                    printf("\nNão existem values para apresentar.\n\n");
                } else {
                    printf("\nValues obtidos com sucesso!\n");
                    
                    printf("Values: ");

                    for(int i = 0; valuesRes[i] != NULL; i++) {
                        char * valueString = valuesRes[i];
                        if(valuesRes[i+1] != NULL) {
                            printf("%s, ", valueString);
                        } else {
                            printf("%s", valueString);
                        }
                    }
                    printf("\n\n");
                }

                int i = 0;
                while (valuesRes[i] != NULL) {
                    free(valuesRes[i]);
                    i++;
                }
                free(valuesRes);

            } else {
                printf("Values nao obtidos com sucesso.\n");
            }
        } else if (strcmp(action, "verify") == 0) {
    
            int op_n = atoi(strtok(NULL, " "));
            if (op_n <= 0) {
                printf("ID invalido, insira um ID superior a 0.\n\n");       
                continue;
            }

            printf("head desc: %d\n", headTree->descriptor);
            printf("tail desc: %d\n", tailTree->descriptor);

            int result = rtree_verify(tailTree, op_n);
            if (result == -1) {
                printf("ID %d invalido, insira um ID que ja lhe tenha sido retornado.\n\n", op_n);
            } else if (result == 0) {
                printf("A operacao ainda nao foi executada.\n\n");
            } else {
                printf("A operacao foi executada com sucesso.\n\n");
            }

        } else if (strcmp(action, "quit\n") == 0) {
            rtree_disconnect(headTree, tailTree);
            return 0;
        } else {
            printf("Comando desconhecido, tente novamente.\n\n");
        }
    }
}

