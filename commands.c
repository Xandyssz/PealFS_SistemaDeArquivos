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


void do_mkdir(const char* dir_name) {
    // 0. Validações básicas
    if (strlen(dir_name) > MAX_FILENAME - 1) {
        printf("Erro: Nome do diretorio e muito longo.\n");
        return;
    }

    // 1. Verificar se o nome já existe no diretório atual
    Inode parent_inode;
    read_inode(current_directory_inode, &parent_inode);
    char block_buffer[BLOCK_SIZE];
    read_block(parent_inode.direct_blocks[0], block_buffer);
    DirectoryEntry *entries = (DirectoryEntry *)block_buffer;
    int num_entries = parent_inode.size / sizeof(DirectoryEntry);

    for (int i = 0; i < num_entries; i++) {
        if (strcmp(entries[i].name, dir_name) == 0) {
            printf("Erro: O nome '%s' ja existe neste diretorio.\n", dir_name);
            return;
        }
    }

    // Checar se há espaço para nova entrada no bloco do diretório pai
    if (num_entries >= (BLOCK_SIZE / sizeof(DirectoryEntry))) {
        printf("Erro: O diretorio atual esta cheio.\n");
        return;
    }

    // 2. Encontrar um inode e um bloco de dados livres
    int new_inode_num = find_free_inode();
    if (new_inode_num == -1) {
        printf("Erro: Nao ha inodes livres.\n");
        return;
    }
    int new_block_num = find_free_block();
    if (new_block_num == -1) {
        printf("Erro: Nao ha blocos de dados livres.\n");
        return;
    }

    // 3. Alocar o bloco de dados
    alloc_block(new_block_num);

    // 4. Configurar o inode para o novo diretório
    Inode new_inode;
    new_inode.type = 'd';
    new_inode.size = 2 * sizeof(DirectoryEntry); // Para '.' e '..'
    new_inode.block_count = 1;
    new_inode.direct_blocks[0] = new_block_num;
    for (int i = 1; i < NUM_DIRECT_POINTERS; i++) {
        new_inode.direct_blocks[i] = -1; // Marcar outros ponteiros como não utilizados
    }

    // 5. Preparar o bloco de dados do novo diretório com as entradas '.' e '..'
    char new_block_buffer[BLOCK_SIZE] = {0};
    DirectoryEntry *new_entries = (DirectoryEntry *)new_block_buffer;
    strcpy(new_entries[0].name, ".");
    new_entries[0].inode_number = new_inode_num;
    strcpy(new_entries[1].name, "..");
    new_entries[1].inode_number = current_directory_inode;

    // 6. Adicionar a entrada do novo diretório no diretório pai
    strcpy(entries[num_entries].name, dir_name);
    entries[num_entries].inode_number = new_inode_num;

    // 7. Atualizar o inode do diretório pai
    parent_inode.size += sizeof(DirectoryEntry);

    // 8. Escrever tudo de volta para o "disco"
    write_inode(new_inode_num, &new_inode);
    write_block(new_block_num, new_block_buffer);
    write_inode(current_directory_inode, &parent_inode);
    write_block(parent_inode.direct_blocks[0], block_buffer);

    printf("Diretorio '%s' criado com sucesso.\n", dir_name);
}


void do_cd(const char* path) {
    // Caso especial: ir para o diretório raiz
    if (strcmp(path, "/") == 0) {
        current_directory_inode = 0;
        return;
    }

    // Procurar pelo nome do diretório no diretório atual
    Inode current_inode;
    read_inode(current_directory_inode, &current_inode);

    char block_buffer[BLOCK_SIZE];
    read_block(current_inode.direct_blocks[0], block_buffer);
    DirectoryEntry *entries = (DirectoryEntry *)block_buffer;
    int num_entries = current_inode.size / sizeof(DirectoryEntry);

    for (int i = 0; i < num_entries; i++) {
        if (strcmp(entries[i].name, path) == 0) {
            // Encontramos uma entrada com o nome correspondente.
            // Agora, precisamos verificar se é um diretório.
            Inode target_inode;
            read_inode(entries[i].inode_number, &target_inode);
            if (target_inode.type == 'd') {
                // É um diretório, então a mudança é válida.
                current_directory_inode = entries[i].inode_number;
                return;
            } else {
                printf("Erro: '%s' nao e um diretorio.\n", path);
                return;
            }
        }
    }

    // Se o loop terminar, não encontramos o diretório.
    printf("Erro: Diretorio '%s' nao encontrado.\n", path);
}


void do_pwd() {
    // Se estamos na raiz, a tarefa é simples.
    if (current_directory_inode == 0) {
        printf("/\n");
        return;
    }

    char path[1024] = ""; // Armazenará o caminho final
    char reversed_path[1024] = ""; // Armazenará o caminho construído de trás para frente
    int current_inode_num = current_directory_inode;

    while (current_inode_num != 0) {
        Inode current_inode_obj;
        read_inode(current_inode_num, &current_inode_obj);

        // Precisamos encontrar o pai para descobrir o nome do diretório atual
        Inode parent_inode_obj;
        int parent_inode_num = -1;

        // Ler o bloco de dados do diretório ATUAL para encontrar a entrada '..'
        char block_buffer[BLOCK_SIZE];
        read_block(current_inode_obj.direct_blocks[0], block_buffer);
        DirectoryEntry* entries = (DirectoryEntry*)block_buffer;
        int num_entries = current_inode_obj.size / sizeof(DirectoryEntry);

        // Encontrar o inode do pai
        for (int i = 0; i < num_entries; i++) {
            if (strcmp(entries[i].name, "..") == 0) {
                parent_inode_num = entries[i].inode_number;
                break;
            }
        }

        if (parent_inode_num == -1) {
            printf("Erro: Estrutura de diretorio corrompida. Nao foi possivel encontrar '..'\n");
            return;
        }

        read_inode(parent_inode_num, &parent_inode_obj);

        // Agora, ler o bloco de dados do PAI para encontrar como ele nos chama
        char parent_block_buffer[BLOCK_SIZE];
        read_block(parent_inode_obj.direct_blocks[0], parent_block_buffer);
        DirectoryEntry* parent_entries = (DirectoryEntry*)parent_block_buffer;
        int num_parent_entries = parent_inode_obj.size / sizeof(DirectoryEntry);

        char current_dir_name[MAX_FILENAME] = "";
        for (int i = 0; i < num_parent_entries; i++) {
            if (parent_entries[i].inode_number == current_inode_num) {
                strcpy(current_dir_name, parent_entries[i].name);
                break;
            }
        }

        // Prepend (adicionar no início) o nome encontrado ao caminho reverso
        char temp_path[1024];
        sprintf(temp_path, "/%s%s", current_dir_name, reversed_path);
        strcpy(reversed_path, temp_path);

        // Subir um nível para a próxima iteração
        current_inode_num = parent_inode_num;
    }

    printf("%s\n", reversed_path);
}
