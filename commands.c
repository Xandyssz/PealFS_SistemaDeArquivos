// Arquivo: commands.c

#include "commands.h"
#include "disk_ops.h"
#include "hash_search.h" // Inclui a busca hash
#include <stdio.h>
#include <string.h>

// Variável global externa (do shell.c)
extern int current_directory_inode;

// --- COMANDO LS ---
void do_ls() {
    Inode dir_inode;
    read_inode(current_directory_inode, &dir_inode);

    if (dir_inode.type != 'd') {
        printf("Erro: 'ls' só pode ser executado em um diretorio.\n");
        return;
    }

    char block_buffer[BLOCK_SIZE];
    read_block(dir_inode.direct_blocks[0], block_buffer);

    DirectoryEntry *dir_entries = (DirectoryEntry *)block_buffer;
    int num_entries = dir_inode.size / sizeof(DirectoryEntry);

    printf("TIPO - INODE - NOME                - TAMANHO\n");
    printf("-------------------------------------------\n");

    for (int i = 0; i < num_entries; i++) {
        if (dir_entries[i].inode_number != 0 || 
            strcmp(dir_entries[i].name, ".") == 0 || 
            strcmp(dir_entries[i].name, "..") == 0) {
            
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

// --- COMANDO MKDIR ---
void do_mkdir(const char* dir_name) {
    if (strlen(dir_name) > MAX_FILENAME - 1) {
        printf("Erro: Nome do diretorio e muito longo.\n");
        return;
    }

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

    if (num_entries >= (BLOCK_SIZE / sizeof(DirectoryEntry))) {
        printf("Erro: O diretorio atual esta cheio.\n");
        return;
    }

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

    alloc_block(new_block_num);

    Inode new_inode;
    new_inode.type = 'd';
    new_inode.size = 2 * sizeof(DirectoryEntry);
    new_inode.block_count = 1;
    new_inode.direct_blocks[0] = new_block_num;
    for (int i = 1; i < NUM_DIRECT_POINTERS; i++) {
        new_inode.direct_blocks[i] = -1;
    }

    char new_block_buffer[BLOCK_SIZE] = {0};
    DirectoryEntry *new_entries = (DirectoryEntry *)new_block_buffer;
    strcpy(new_entries[0].name, ".");
    new_entries[0].inode_number = new_inode_num;
    strcpy(new_entries[1].name, "..");
    new_entries[1].inode_number = current_directory_inode;

    strcpy(entries[num_entries].name, dir_name);
    entries[num_entries].inode_number = new_inode_num;

    parent_inode.size += sizeof(DirectoryEntry);

    write_inode(new_inode_num, &new_inode);
    write_block(new_block_num, new_block_buffer);
    write_inode(current_directory_inode, &parent_inode);
    write_block(parent_inode.direct_blocks[0], block_buffer);

    printf("Diretorio '%s' criado com sucesso.\n", dir_name);
}

// --- COMANDO CD ---
void do_cd(const char* path) {
    if (strcmp(path, "/") == 0) {
        current_directory_inode = 0;
        return;
    }

    Inode current_inode;
    read_inode(current_directory_inode, &current_inode);

    char block_buffer[BLOCK_SIZE];
    read_block(current_inode.direct_blocks[0], block_buffer);
    DirectoryEntry *entries = (DirectoryEntry *)block_buffer;
    int num_entries = current_inode.size / sizeof(DirectoryEntry);

    for (int i = 0; i < num_entries; i++) {
        if (strcmp(entries[i].name, path) == 0) {
            Inode target_inode;
            read_inode(entries[i].inode_number, &target_inode);
            if (target_inode.type == 'd') {
                current_directory_inode = entries[i].inode_number;
                return;
            } else {
                printf("Erro: '%s' nao e um diretorio.\n", path);
                return;
            }
        }
    }
    printf("Erro: Diretorio '%s' nao encontrado.\n", path);
}

// --- COMANDO PWD ---
void do_pwd() {
    if (current_directory_inode == 0) {
        printf("/\n");
        return;
    }

    char path[1024] = "";
    char reversed_path[1024] = "";
    int current_inode_num = current_directory_inode;

    while (current_inode_num != 0) {
        Inode current_inode_obj;
        read_inode(current_inode_num, &current_inode_obj);

        Inode parent_inode_obj;
        int parent_inode_num = -1;

        char block_buffer[BLOCK_SIZE];
        read_block(current_inode_obj.direct_blocks[0], block_buffer);
        DirectoryEntry* entries = (DirectoryEntry*)block_buffer;
        int num_entries = current_inode_obj.size / sizeof(DirectoryEntry);

        for (int i = 0; i < num_entries; i++) {
            if (strcmp(entries[i].name, "..") == 0) {
                parent_inode_num = entries[i].inode_number;
                break;
            }
        }

        if (parent_inode_num == -1) {
            printf("Erro: Estrutura corrompida.\n");
            return;
        }

        read_inode(parent_inode_num, &parent_inode_obj);

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

        char temp_path[1024];
        sprintf(temp_path, "/%s%s", current_dir_name, reversed_path);
        strcpy(reversed_path, temp_path);

        current_inode_num = parent_inode_num;
    }

    printf("%s\n", reversed_path);
}

// --- COMANDO TOUCH ---
void do_touch(const char* filename) {
    if (strlen(filename) > MAX_FILENAME - 1) {
        printf("Erro: Nome muito longo.\n");
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
            printf("Erro: O nome '%s' ja existe.\n", filename);
            return;
        }
    }

    int new_inode_num = find_free_inode();
    if (new_inode_num == -1) {
        printf("Erro: Sem inodes livres.\n");
        return;
    }

    printf("Digite o conteudo (CTRL+D ou CTRL+Z para salvar):\n");
    char full_content[PARTITION_SIZE] = {0};
    char line[512];
    size_t total_size = 0;

    while (fgets(line, sizeof(line), stdin) != NULL) {
        strcat(full_content, line);
        total_size += strlen(line);
    }
    clearerr(stdin);

    int blocks_needed = (total_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (blocks_needed == 0) blocks_needed = 1;
    if (blocks_needed > NUM_DIRECT_POINTERS) {
        printf("Erro: Conteudo muito grande.\n");
        return;
    }

    int allocated_blocks[NUM_DIRECT_POINTERS];
    for (int i = 0; i < blocks_needed; i++) {
        int new_block = find_free_block();
        if (new_block == -1) {
            printf("Erro: Disco cheio.\n");
            for(int j = 0; j < i; j++) free_block(allocated_blocks[j]);
            return;
        }
        alloc_block(new_block);
        allocated_blocks[i] = new_block;
    }

    Inode new_inode;
    new_inode.type = 'f';
    new_inode.size = total_size;
    new_inode.block_count = blocks_needed;
    for (int i = 0; i < blocks_needed; i++) {
        new_inode.direct_blocks[i] = allocated_blocks[i];
    }
    for (int i = blocks_needed; i < NUM_DIRECT_POINTERS; i++) {
        new_inode.direct_blocks[i] = -1;
    }

    for (int i = 0; i < blocks_needed; i++) {
        char block_to_write[BLOCK_SIZE] = {0};
        memcpy(block_to_write, full_content + (i * BLOCK_SIZE), BLOCK_SIZE);
        write_block(allocated_blocks[i], block_to_write);
    }

    strcpy(entries[num_entries].name, filename);
    entries[num_entries].inode_number = new_inode_num;
    parent_inode.size += sizeof(DirectoryEntry);

    write_inode(new_inode_num, &new_inode);
    write_inode(current_directory_inode, &parent_inode);
    write_block(parent_inode.direct_blocks[0], parent_block_buffer);

    printf("Arquivo '%s' criado com sucesso.\n", filename);
}

void do_cat(const char* filename) {
    // 1. Carrega o índice do diretório atual para a Hash Table (RAM)
    HashTable *dir_cache = load_directory_to_hash(current_directory_inode);
    
    if (dir_cache == NULL) {
        printf("Erro ao carregar cache do diretorio.\n");
        return;
    }

    // 2. Busca O(1) usando a tabela Hash
    int file_inode_num = search_in_hash(dir_cache, filename);

    // 3. Limpa a Hash da memória imediatamente após o uso
    free_hash_table(dir_cache);

    if (file_inode_num == -1) {
        printf("Erro: Arquivo '%s' nao encontrado.\n", filename);
        return;
    }

    // 4. Prossegue com a leitura normal do arquivo usando o inode encontrado
    Inode file_inode;
    read_inode(file_inode_num, &file_inode);
    
    if (file_inode.type != 'f') {
        printf("Erro: '%s' nao e um arquivo de texto.\n", filename);
        return;
    }

    char content_buffer[BLOCK_SIZE];
    int bytes_left = file_inode.size;

    for (int i = 0; i < file_inode.block_count; i++) {
        read_block(file_inode.direct_blocks[i], content_buffer);
        int to_print = (bytes_left > BLOCK_SIZE) ? BLOCK_SIZE : bytes_left;
        fwrite(content_buffer, 1, to_print, stdout);
        bytes_left -= to_print;
    }
    printf("\n");
}

// --- COMANDO RM ---
void do_rm(const char* name) {
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
        printf("Erro: '%s' nao encontrado.\n", name);
        return;
    }

    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        printf("Erro: Nao pode remover '.' ou '..'.\n");
        return;
    }

    Inode target_inode;
    read_inode(target_inode_num, &target_inode);

    if (target_inode.type == 'd') {
        if (target_inode.size > 2 * sizeof(DirectoryEntry)) {
            printf("Erro: Diretorio nao esta vazio.\n");
            return;
        }
    }

    for (int i = 0; i < target_inode.block_count; i++) {
        if (target_inode.direct_blocks[i] != -1) {
            free_block(target_inode.direct_blocks[i]);
        }
    }

    target_inode.type = 'u';
    write_inode(target_inode_num, &target_inode);

    if (entry_index != num_entries - 1) {
        entries[entry_index] = entries[num_entries - 1];
    }

    parent_inode.size -= sizeof(DirectoryEntry);
    write_inode(current_directory_inode, &parent_inode);
    write_block(parent_inode.direct_blocks[0], parent_block_buffer);

    printf("'%s' removido.\n", name);
}

// --- COMANDO STAT ---
void do_stat() {
    int free_blocks_count = 0;
    FILE *fp = fopen("fs/freespace.dat", "rb");
    if (!fp) return;

    char bitmap[NUM_BLOCKS / 8];
    fread(bitmap, sizeof(bitmap), 1, fp);
    fclose(fp);

    for (int byte_index = 0; byte_index < (NUM_BLOCKS / 8); byte_index++) {
        for (int bit_index = 0; bit_index < 8; bit_index++) {
            if (!((bitmap[byte_index] >> (7 - bit_index)) & 1)) {
                free_blocks_count++;
            }
        }
    }

    printf("Espaco livre: %d Bytes\n", free_blocks_count * BLOCK_SIZE);
    printf("Blocos livres: %d Blocos\n", free_blocks_count);
    printf("Tamanho do bloco: %d Bytes\n", BLOCK_SIZE);
}