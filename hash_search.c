#include "hash_search.h"
#include "disk_ops.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Algoritmo de Hash 
unsigned long hash_function(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash % HASH_TABLE_SIZE;
}

// Inicializa a tabela hash zerada
HashTable* create_hash_table() {
    HashTable *ht = (HashTable*) malloc(sizeof(HashTable));
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        ht->buckets[i] = NULL;
    }
    return ht;
}

// Insere um par (nome, inode) na tabela
void insert_into_hash(HashTable *ht, const char *name, uint8_t inode_num) {
    unsigned long index = hash_function(name);
    
    // Cria novo nó
    HashNode *new_node = (HashNode*) malloc(sizeof(HashNode));
    strcpy(new_node->name, name);
    new_node->inode_number = inode_num;
    new_node->next = NULL;

    // Inserção no início da lista (tratamento de colisão)
    if (ht->buckets[index] == NULL) {
        ht->buckets[index] = new_node;
    } else {
        new_node->next = ht->buckets[index];
        ht->buckets[index] = new_node;
    }
}

// Busca um arquivo pelo nome. Retorna o inode ou -1 se não achar.
int search_in_hash(HashTable *ht, const char *name) {
    unsigned long index = hash_function(name);
    HashNode *current = ht->buckets[index];

    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current->inode_number; // Encontrou
        }
        current = current->next;
    }
    return -1; // Não encontrado
}

// Libera memória da tabela
void free_hash_table(HashTable *ht) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashNode *current = ht->buckets[i];
        while (current != NULL) {
            HashNode *temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(ht);
}


// Lê o diretório do disco (blocks.dat) e popula a Hash Table
HashTable* load_directory_to_hash(int dir_inode_num) {
    // 1. Ler o inode do diretório
    Inode dir_inode;
    read_inode(dir_inode_num, &dir_inode);

    if (dir_inode.type != 'd') return NULL;

    HashTable *ht = create_hash_table();

    // 2. Ler o bloco de dados onde estão as DirectoryEntries
    char block_buffer[BLOCK_SIZE];
    read_block(dir_inode.direct_blocks[0], block_buffer);

    DirectoryEntry *entries = (DirectoryEntry *)block_buffer;
    int num_entries = dir_inode.size / sizeof(DirectoryEntry);

    // 3. Iterar sobre as entradas lineares e inserir na Hash
    for (int i = 0; i < num_entries; i++) {
        // Verifica se a entrada é válida (nome não vazio)
        if (strlen(entries[i].name) > 0) {
            insert_into_hash(ht, entries[i].name, entries[i].inode_number);
        }
    }

    return ht;
}