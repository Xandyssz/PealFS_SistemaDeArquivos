// Arquivo: commands.h

#ifndef COMMANDS_H
#define COMMANDS_H

// Declaração da nossa função para o comando 'ls'
void do_ls();

// Declaração para o comando 'mkdir'
void do_mkdir(const char* dir_name);

// Declaração para o comando 'cd'
void do_cd(const char* path);

// Declaração para o comando 'pwd'
void do_pwd();

// Declarações para 'touch' e 'cat'
void do_touch(const char* filename);
void do_cat(const char* filename);

// Declaração para 'rm'
void do_rm(const char* name);

// Declaração para 'stat'
void do_stat();

#endif // COMMANDS_H