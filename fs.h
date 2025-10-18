// Arquivo: fs.h

#ifndef FS_H
#define FS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // Para usar tipos como uint8_t

// --- CONSTANTES BASEADAS NOS REQUISITOS ---

#define BLOCK_SIZE 128          // Tamanho do bloco em bytes
#define PARTITION_SIZE 10240    // Tamanho total da partição em bytes
#define NUM_BLOCKS (PARTITION_SIZE / BLOCK_SIZE) // Total de blocos (80)
#define MAX_INODES 256          // Máximo de inodes (limitado pelo uint8_t)
#define MAX_FILENAME 14         // Tamanho máximo do nome de arquivo

// O número de ponteiros diretos para blocos de dados em um inode.
// A imagem sugere 4, vamos usar 4 para começar.
#define NUM_DIRECT_POINTERS 4

// --- ESTRUTURAS DE DADOS ---

/*
 * Superbloco: Armazena os metadados do sistema de arquivos.
 * Vamos simplificar e usar as constantes definidas acima, mas em um sistema
 * real, leríamos isso do arquivo superblock.dat.
 */
typedef struct {
    char filesystem[8];      // Nome do sistema de arquivos
    int block_size;
    int partition_size;
    int num_blocks;
    int num_inodes;
} Superblock;

/*
 * Inode: Nó de índice que representa um arquivo ou diretório.
 */
typedef struct {
    char type;                          // 'f' para arquivo, 'd' para diretório, 'u' para não utilizado
    uint32_t size;                      // Tamanho do arquivo em bytes
    uint16_t block_count;               // Quantidade de blocos utilizados
    uint16_t direct_blocks[NUM_DIRECT_POINTERS]; // Ponteiros diretos para os blocos de dados
    // Podemos adicionar ponteiros indiretos aqui no futuro, se necessário.
} Inode;

/*
 * Entrada de Diretório: Mapeia um nome de arquivo para um número de inode.
 * Um diretório é um arquivo cujo conteúdo é uma lista dessas entradas.
 */
typedef struct {
    char name[MAX_FILENAME];            // Nome do arquivo/diretório
    uint8_t inode_number;               // Número do inode correspondente (1 byte = 0-255)
} DirectoryEntry;


#endif // FS_H