// Arquivo: disk_ops.c

#include "disk_ops.h"

// Lê um bloco de dados do arquivo blocks.dat
void read_block(int block_num, char *buffer) {
    FILE *fp = fopen("fs/blocks.dat", "rb");
    if (fp) {
        fseek(fp, block_num * BLOCK_SIZE, SEEK_SET);
        fread(buffer, BLOCK_SIZE, 1, fp);
        fclose(fp);
    } else {
        perror("Erro ao abrir blocks.dat para leitura");
    }
}

// Escreve dados em um bloco no arquivo blocks.dat
void write_block(int block_num, const char *data) {
    FILE *fp = fopen("fs/blocks.dat", "r+b"); // r+b para ler e escrever
    if (fp) {
        fseek(fp, block_num * BLOCK_SIZE, SEEK_SET);
        fwrite(data, BLOCK_SIZE, 1, fp);
        fclose(fp);
    } else {
        perror("Erro ao abrir blocks.dat para escrita");
    }
}

// Lê um inode específico da tabela de inodes
void read_inode(int inode_num, Inode *inode) {
    FILE *fp = fopen("fs/inodes.dat", "rb");
    if (fp) {
        fseek(fp, inode_num * sizeof(Inode), SEEK_SET);
        fread(inode, sizeof(Inode), 1, fp);
        fclose(fp);
    } else {
        perror("Erro ao abrir inodes.dat para leitura");
    }
}

// Escreve um inode específico na tabela de inodes
void write_inode(int inode_num, const Inode *inode) {
    FILE *fp = fopen("fs/inodes.dat", "r+b");
    if (fp) {
        fseek(fp, inode_num * sizeof(Inode), SEEK_SET);
        fwrite(inode, sizeof(Inode), 1, fp);
        fclose(fp);
    } else {
        perror("Erro ao abrir inodes.dat para escrita");
    }
}

// Procura pelo primeiro inode livre na tabela de inodes
int find_free_inode() {
    FILE *fp = fopen("fs/inodes.dat", "rb");
    if (fp) {
        Inode temp_inode;
        for (int i = 0; i < MAX_INODES; i++) {
            fread(&temp_inode, sizeof(Inode), 1, fp);
            if (temp_inode.type == 'u') {
                fclose(fp);
                return i; // Retorna o número do primeiro inode livre
            }
        }
        fclose(fp);
    }
    return -1; // Nenhum inode livre encontrado
}

// Procura pelo primeiro bloco de dados livre no bitmap
int find_free_block() {
    FILE *fp = fopen("fs/freespace.dat", "rb");
    if (fp) {
        char bitmap[NUM_BLOCKS / 8];
        fread(bitmap, sizeof(bitmap), 1, fp);
        fclose(fp);

        for (int byte_index = 0; byte_index < (NUM_BLOCKS / 8); byte_index++) {
            for (int bit_index = 0; bit_index < 8; bit_index++) {
                // Verifica se o bit na posição 'bit_index' está zerado (livre)
                if (!((bitmap[byte_index] >> (7 - bit_index)) & 1)) {
                    
                    int block_num = (byte_index * 8) + bit_index;
                    return block_num;
                }
            }
        }
    }
    return -1; // Nenhum bloco livre encontrado
}

// Marca um bloco como ocupado (1) no bitmap
void alloc_block(int block_num) {
    FILE *fp = fopen("fs/freespace.dat", "r+b");
    if (fp) {
        char bitmap[NUM_BLOCKS / 8];
        fread(bitmap, sizeof(bitmap), 1, fp);

        int byte_index = block_num / 8;
        int bit_offset = 7 - (block_num % 8); // MSB-first
        bitmap[byte_index] |= (1 << bit_offset);
        // X | 0 = X

        fseek(fp, 0, SEEK_SET);
        fwrite(bitmap, sizeof(bitmap), 1, fp);
        fclose(fp);
    }
}



// Marca um bloco como livre (0) no bitmap (usaremos no comando 'rm')
void free_block(int block_num) {
    FILE *fp = fopen("fs/freespace.dat", "r+b");
    if (fp) {
        char bitmap[NUM_BLOCKS / 8];
        fread(bitmap, sizeof(bitmap), 1, fp);

        int byte_index = block_num / 8;
        int bit_offset = 7 - (block_num % 8); // MSB-first
        bitmap[byte_index] &= ~(1 << bit_offset);

        fseek(fp, 0, SEEK_SET);
        fwrite(bitmap, sizeof(bitmap), 1, fp);
        fclose(fp);
    }
}