// Arquivo: disk_ops.h

#ifndef DISK_OPS_H
#define DISK_OPS_H

#include "fs.h"

// Funções para ler e escrever blocos de dados individuais
void read_block(int block_num, char *buffer);
void write_block(int block_num, const char *data);

// Funções para ler e escrever inodes individuais
void read_inode(int inode_num, Inode *inode);
void write_inode(int inode_num, const Inode *inode);

// Funções para encontrar e alocar recursos livres
int find_free_inode();
int find_free_block();

// Funções para manipular o bitmap de blocos livres
void alloc_block(int block_num);
void free_block(int block_num);


#endif // DISK_OPS_H