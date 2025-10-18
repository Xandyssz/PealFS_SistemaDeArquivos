// Arquivo: mkfs.c

#include "fs.h"
#include <direct.h>

// Função principal que formata o sistema de arquivos
int main() {
    FILE *fp;

    // 1. Criar o diretório 'fs' se ele não existir
    _mkdir("fs");

    // 2. Criar e inicializar o superblock.dat
    printf("Criando superblock.dat...\n");
    fp = fopen("fs/superblock.dat", "w");
    if (fp == NULL) {
        perror("Erro ao criar superblock.dat");
        return 1;
    }
    // Escreve as informações no formato exato solicitado
    fprintf(fp, "filesystem=pealfs\n");
    fprintf(fp, "blocksize=%d\n", BLOCK_SIZE);
    fprintf(fp, "partitionsize=%d\n", PARTITION_SIZE);
    fclose(fp);

    // 3. Criar e inicializar o freespace.dat (mapa de bits)
    printf("Criando freespace.dat...\n");
    fp = fopen("fs/freespace.dat", "wb");
    if (fp == NULL) {
        perror("Erro ao criar freespace.dat");
        return 1;
    }
    // NUM_BLOCKS (80) / 8 bits = 10 bytes para o bitmap
    char bitmap[NUM_BLOCKS / 8] = {0}; // Inicializa todos os bits com 0 (livre)
    // Marca o primeiro bloco (bloco 0) como usado (ocupado pelo diretório raiz)
    // O primeiro byte do bitmap, bit mais significativo (posição 7)
    // 10000000 em binário é 0x80 em hexadecimal
    bitmap[0] |= (1 << 7);
    fwrite(bitmap, sizeof(bitmap), 1, fp);
    fclose(fp);

    // 4. Criar e inicializar o inodes.dat
    printf("Criando inodes.dat...\n");
    fp = fopen("fs/inodes.dat", "wb");
    if (fp == NULL) {
        perror("Erro ao criar inodes.dat");
        return 1;
    }
    Inode all_inodes[MAX_INODES];
    for (int i = 0; i < MAX_INODES; i++) {
        all_inodes[i].type = 'u'; // 'u' para unused (não utilizado)
    }
    fwrite(all_inodes, sizeof(Inode), MAX_INODES, fp);
    fclose(fp);

    // 5. Criar o blocks.dat preenchido com zeros
    printf("Criando blocks.dat...\n");
    fp = fopen("fs/blocks.dat", "wb");
    if (fp == NULL) {
        perror("Erro ao criar blocks.dat");
        return 1;
    }
    char empty_block[BLOCK_SIZE] = {0};
    for (int i = 0; i < NUM_BLOCKS; i++) {
        fwrite(empty_block, BLOCK_SIZE, 1, fp);
    }
    fclose(fp);

    // 6. Configurar o diretório raiz (/)
    printf("Configurando o diretorio raiz (/)...\n");

    // 6.1. Configurar o inode 0
    Inode root_inode;
    root_inode.type = 'd'; // 'd' para diretório [cite: 3]
    root_inode.size = 2 * sizeof(DirectoryEntry); // Tamanho inicial para as entradas '.' e '..'
    root_inode.block_count = 1;
    root_inode.direct_blocks[0] = 0; // O diretório raiz usa o bloco de dados 0
    for (int i = 1; i < NUM_DIRECT_POINTERS; i++) {
        root_inode.direct_blocks[i] = -1; // Marcar outros ponteiros como não utilizados
    }

    // 6.2. Escrever o inode 0 atualizado de volta no inodes.dat
    fp = fopen("fs/inodes.dat", "r+b"); // Abre para leitura e escrita binária
    if (fp == NULL) {
        perror("Erro ao abrir inodes.dat para atualizar a raiz");
        return 1;
    }
    fseek(fp, 0, SEEK_SET); // Posiciona no início do arquivo (onde está o inode 0)
    fwrite(&root_inode, sizeof(Inode), 1, fp);
    fclose(fp);

    // 6.3. Criar as entradas '.' e '..' para o diretório raiz
    DirectoryEntry root_entries[2];
    // Entrada para o diretório atual (.)
    strcpy(root_entries[0].name, ".");
    root_entries[0].inode_number = 0;
    // Entrada para o diretório pai (..)
    strcpy(root_entries[1].name, "..");
    root_entries[1].inode_number = 0; // Raiz é seu próprio pai

    // 6.4. Escrever essas entradas no bloco de dados 0
    fp = fopen("fs/blocks.dat", "r+b");
    if (fp == NULL) {
        perror("Erro ao abrir blocks.dat para escrever a raiz");
        return 1;
    }
    fseek(fp, 0, SEEK_SET); // Posiciona no início do arquivo (bloco 0)
    fwrite(root_entries, sizeof(DirectoryEntry), 2, fp);
    fclose(fp);

    printf("\nSistema de arquivos 'pealfs' criado com sucesso no diretorio 'fs/'!\n");

    return 0;
}