// Arquivo: commands.c (Completo até o comando 'cd')

#include "commands.h"
#include "disk_ops.h"
#include <stdio.h>
#include <string.h>

// Variável global definida em shell.c que rastreia o inode do diretório atual
extern int current_directory_inode;

void do_ls() {
    // 1. Ler o inode do diretório atual
    Inode dir_inode;
    read_inode(current_directory_inode, &dir_inode);

    // Verificar se é realmente um diretório
    if (dir_inode.type != 'd') {
        printf("Erro: 'ls' só pode ser executado em um diretorio.\n");
        return;
    }

    // 2. Ler o bloco de dados do diretório
    char block_buffer[BLOCK_SIZE];
    read_block(dir_inode.direct_blocks[0], block_buffer);

    // 3. Iterar pelas entradas do diretório
    DirectoryEntry *dir_entries = (DirectoryEntry *)block_buffer;
    int num_entries = dir_inode.size / sizeof(DirectoryEntry);

    printf("TIPO - INODE - NOME                - TAMANHO\n");
    printf("-------------------------------------------\n");

    for (int i = 0; i < num_entries; i++) {
        // Pular entradas "vazias" (inode 0 é a raiz, não deve ser 0 em outros lugares)
        if (dir_entries[i].inode_number != 0 || strcmp(dir_entries[i].name, ".") == 0) {
            Inode entry_inode;
            read_inode(dir_entries[i].inode_number, &entry_inode);

            printf("%c    - %-5d - %-20s - %u Bytes\n",
                   entry_inode.type,
                   dir_entries[i].inode_number,
                   dir_entries[i].name,
                   entry_inode.size);
        }
    }
}