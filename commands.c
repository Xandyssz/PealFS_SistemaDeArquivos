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
                // É um diretório
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
    // Se estamos na raiz
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

        // ler o bloco de dados do PAI para encontrar como ele nos chama
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


void do_touch(const char* filename) {
    // 1. Validações e verificação de nome duplicado (similar ao mkdir)
    if (strlen(filename) > MAX_FILENAME - 1) {
        printf("Erro: Nome do arquivo e muito longo.\n");
        return;
    }

    Inode parent_inode;
    read_inode(current_directory_inode, &parent_inode);
    char parent_block_buffer[BLOCK_SIZE];
    read_block(parent_inode.direct_blocks[0], parent_block_buffer);
    DirectoryEntry *entries = (DirectoryEntry *)parent_block_buffer;
    int num_entries = parent_inode.size / sizeof(DirectoryEntry);

    for (int i = 0; i < num_entries; i++) {
        if (strcmp(entries[i].name, filename) == 0) {
            printf("Erro: O nome '%s' ja existe neste diretorio.\n", filename);
            return;
        }
    }

    // 2. Encontrar um inode livre
    int new_inode_num = find_free_inode();
    if (new_inode_num == -1) {
        printf("Erro: Nao ha inodes livres.\n");
        return;
    }

    // 3. Ler o conteúdo do usuário
    printf("Digite o conteudo do arquivo. Pressione Ctrl+Z (Windows) ou Ctrl+D (Linux) para salvar.\n");
    char full_content[PARTITION_SIZE] = {0}; // Buffer grande para o conteúdo
    char line[512];
    size_t total_size = 0;

    while (fgets(line, sizeof(line), stdin) != NULL) {
        strcat(full_content, line);
        total_size += strlen(line);
    }
    clearerr(stdin); // Limpa o estado de EOF para o próximo comando

    // 4. Calcular e alocar blocos de dados necessários
    int blocks_needed = (total_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (blocks_needed == 0) blocks_needed = 1; // Alocar ao menos 1 bloco para arquivo vazio
    if (blocks_needed > NUM_DIRECT_POINTERS) {
        printf("Erro: O conteudo do arquivo e muito grande para esta versao do sistema.\n");
        return;
    }

    int allocated_blocks[NUM_DIRECT_POINTERS];
    for (int i = 0; i < blocks_needed; i++) {
        int new_block = find_free_block();
        if (new_block == -1) {
            printf("Erro: Espaco em disco insuficiente.\n");
            // Liberar blocos já alocados (importante!)
            for(int j = 0; j < i; j++) free_block(allocated_blocks[j]);
            return;
        }
        alloc_block(new_block);
        allocated_blocks[i] = new_block;
    }

    // 5. Configurar o inode do novo arquivo
    Inode new_inode;
    new_inode.type = 'f';
    new_inode.size = total_size;
    new_inode.block_count = blocks_needed;
    for (int i = 0; i < blocks_needed; i++) {
        new_inode.direct_blocks[i] = allocated_blocks[i];
    }
    for (int i = blocks_needed; i < NUM_DIRECT_POINTERS; i++) {
        new_inode.direct_blocks[i] = -1; // Marcar como não usado
    }

    // 6. Escrever o conteúdo nos blocos de dados
    for (int i = 0; i < blocks_needed; i++) {
        char block_to_write[BLOCK_SIZE] = {0};
        memcpy(block_to_write, full_content + (i * BLOCK_SIZE), BLOCK_SIZE);
        write_block(allocated_blocks[i], block_to_write);
    }

    // 7. Atualizar o diretório pai
    strcpy(entries[num_entries].name, filename);
    entries[num_entries].inode_number = new_inode_num;
    parent_inode.size += sizeof(DirectoryEntry);

    // 8. Salvar tudo no disco
    write_inode(new_inode_num, &new_inode);
    write_inode(current_directory_inode, &parent_inode);
    write_block(parent_inode.direct_blocks[0], parent_block_buffer);

    printf("Arquivo '%s' criado com sucesso.\n", filename);
}


void do_cat(const char* filename) {
    // 1. Encontrar o arquivo no diretório atual
    Inode parent_inode;
    read_inode(current_directory_inode, &parent_inode);
    char block_buffer[BLOCK_SIZE];
    read_block(parent_inode.direct_blocks[0], block_buffer);
    DirectoryEntry *entries = (DirectoryEntry *)block_buffer;
    int num_entries = parent_inode.size / sizeof(DirectoryEntry);

    int file_inode_num = -1;
    for (int i = 0; i < num_entries; i++) {
        if (strcmp(entries[i].name, filename) == 0) {
            file_inode_num = entries[i].inode_number;
            break;
        }
    }

    if (file_inode_num == -1) {
        printf("Erro: Arquivo '%s' nao encontrado.\n", filename);
        return;
    }

    // 2. Verificar se é um arquivo
    Inode file_inode;
    read_inode(file_inode_num, &file_inode);
    if (file_inode.type != 'f') {
        printf("Erro: '%s' nao e um arquivo de texto.\n", filename);
        return;
    }

    // 3. Ler e imprimir o conteúdo bloco por bloco
    char content_buffer[BLOCK_SIZE];
    int bytes_left_to_read = file_inode.size;

    for (int i = 0; i < file_inode.block_count; i++) {
        read_block(file_inode.direct_blocks[i], content_buffer);

        int bytes_to_print = (bytes_left_to_read > BLOCK_SIZE) ? BLOCK_SIZE : bytes_left_to_read;

        // fwrite é melhor para imprimir dados brutos do que printf
        fwrite(content_buffer, 1, bytes_to_print, stdout);

        bytes_left_to_read -= bytes_to_print;
        if (bytes_left_to_read <= 0) break;
    }
    printf("\n"); // Adiciona uma nova linha no final da saída
}


void do_rm(const char* name) {
    // 1. Encontrar a entrada no diretório atual
    Inode parent_inode;
    read_inode(current_directory_inode, &parent_inode);
    char parent_block_buffer[BLOCK_SIZE];
    read_block(parent_inode.direct_blocks[0], parent_block_buffer);
    DirectoryEntry *entries = (DirectoryEntry *)parent_block_buffer;
    int num_entries = parent_inode.size / sizeof(DirectoryEntry);

    int entry_index = -1;
    int target_inode_num = -1;
    for (int i = 0; i < num_entries; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            entry_index = i;
            target_inode_num = entries[i].inode_number;
            break;
        }
    }

    if (entry_index == -1) {
        printf("Erro: Arquivo ou diretorio '%s' nao encontrado.\n", name);
        return;
    }

    // Não permitir remover '.' ou '..'
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        printf("Erro: Nao e possivel remover os diretorios '.' ou '..'.\n");
        return;
    }

    // 2. Ler o inode do alvo
    Inode target_inode;
    read_inode(target_inode_num, &target_inode);

    // 3. Se for um diretório, verificar se está vazio
    if (target_inode.type == 'd') {
        if (target_inode.size > 2 * sizeof(DirectoryEntry)) {
            printf("Erro: O diretorio '%s' nao esta vazio.\n", name);
            return;
        }
    }

    // 4. Liberar os blocos de dados
    for (int i = 0; i < target_inode.block_count; i++) {
        if (target_inode.direct_blocks[i] != -1) {
            free_block(target_inode.direct_blocks[i]);
        }
    }

    // 4.1. Liberar o inode
    target_inode.type = 'u';
    write_inode(target_inode_num, &target_inode);

    // 5. Remover a entrada do diretório pai
    // Copiamos a última entrada para a posição da entrada removida
    if (entry_index != num_entries - 1) { // Se não for o último elemento
        entries[entry_index] = entries[num_entries - 1];
    }
    // O último slot pode ser zerado, mas não é estritamente necessário
    // já que vamos diminuir o tamanho do diretório.

    // 6. Atualizar e salvar o inode e bloco do pai
    parent_inode.size -= sizeof(DirectoryEntry);
    write_inode(current_directory_inode, &parent_inode);
    write_block(parent_inode.direct_blocks[0], parent_block_buffer);

    printf("'%s' removido com sucesso.\n", name);
}


void do_stat() {
    int free_blocks_count = 0;

    // 1. Ler o bitmap de freespace.dat
    FILE *fp = fopen("fs/freespace.dat", "rb");
    if (!fp) {
        perror("Erro ao abrir freespace.dat");
        return;
    }

    char bitmap[NUM_BLOCKS / 8];
    fread(bitmap, sizeof(bitmap), 1, fp);
    fclose(fp);

    // 2. Iterar sobre o bitmap para contar os bits '0' (livres)
    for (int byte_index = 0; byte_index < (NUM_BLOCKS / 8); byte_index++) {
        for (int bit_index = 0; bit_index < 8; bit_index++) {
            // Verifica se o bit na posição 'bit_index' está zerado (livre)
            if (!((bitmap[byte_index] >> (7 - bit_index)) & 1)) {
                free_blocks_count++;
            }
        }
    }

    // 3. Calcular o espaço livre
    int free_space_bytes = free_blocks_count * BLOCK_SIZE;

    // 4. Imprimir as informações no formato solicitado
    printf("Espaco livre: %d Bytes\n", free_space_bytes);
    printf("Blocos livres: %d Blocos\n", free_blocks_count);
    printf("Tamanho do bloco: %d Bytes\n", BLOCK_SIZE);
}