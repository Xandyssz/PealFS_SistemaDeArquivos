#ifndef HASH_SEARCH_H
#define HASH_SEARCH_H

#include "fs.h"

#define HASH_TABLE_SIZE 32

// Nó da lista encadeada para tratar colisões
typedef struct HashNode {
    char name[MAX_FILENAME];
    uint8_t inode_number;
    struct HashNode *next;
} HashNode;

// A estrutura da Tabela Hash
typedef struct {
    HashNode *buckets[HASH_TABLE_SIZE];
} HashTable;

// Protótipos
unsigned long hash_function(const char *str);
HashTable* create_hash_table();
void insert_into_hash(HashTable *ht, const char *name, uint8_t inode_num);
int search_in_hash(HashTable *ht, const char *name);
void free_hash_table(HashTable *ht);

// Função auxiliar que carrega um diretório do disco para a Hash
HashTable* load_directory_to_hash(int dir_inode_num);

#endif