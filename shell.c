// Arquivo: shell.c

#include "disk_ops.h"
#include "commands.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// O inode do diretório em que estamos. Começamos na raiz (inode 0).
int current_directory_inode = 0;

int main()
{
    char input[1024];
    char *command;
    char *arg1;

    printf("Bem-vindo ao pealfs!\n");
    printf("Para sair, digite 'exit'.\n");

    while (1)
    {
        // 1. Mostrar o prompt
        printf("peal:/ > ");

        // 2. Ler a entrada do usuário
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            printf("\nSaindo...\n");
            break; // Sai se CTRL+D for pressionado
        }

        // Remover a quebra de linha do final do input
        input[strcspn(input, "\n")] = 0;

        // 3. Quebrar a entrada em tokens (comando e argumentos)
        command = strtok(input, " ");
        if (command == NULL)
        {
            continue; // Linha vazia, volta para o início do loop
        }

        arg1 = strtok(NULL, " "); // Pega o primeiro argumento

        // 4. Executar o comando correspondente
        if (strcmp(command, "ls") == 0)
        {
            do_ls();
        }
        else if (strcmp(command, "mkdir") == 0)
        {
            if (arg1 == NULL)
            {
                printf("Uso: mkdir <nome_do_diretorio>\n");
            }
            else
            {
                do_mkdir(arg1);
            }
        }
        else if (strcmp(command, "cd") == 0)
        {
            if (arg1 == NULL)
            {
                printf("Uso: cd <caminho>\n");
            }
            else
            {
                do_cd(arg1);
            }
        }
        else if (strcmp(command, "pwd") == 0)
        {
            do_pwd();
        }
        else if (strcmp(command, "touch") == 0)
        {
            if (arg1 == NULL)
            {
                printf("Uso: touch <nome_do_arquivo>\n");
            }
            else
            {
                do_touch(arg1);
            }
        }
        else if (strcmp(command, "cat") == 0)
        {
            if (arg1 == NULL)
            {
                printf("Uso: cat <nome_do_arquivo>\n");
            }
            else
            {
                do_cat(arg1);
            }
        }
        else if (strcmp(command, "rm") == 0)
        {
            if (arg1 == NULL)
            {
                printf("Uso: rm <nome_do_arquivo_ou_diretorio>\n");
            }
            else
            {
                do_rm(arg1);
            }
        }
        else if (strcmp(command, "stat") == 0)
        {
            do_stat();
        }
        else if (strcmp(command, "exit") == 0)
        {
            printf("Saindo...\n");
            break;
        }
        else
        {
            printf("Comando desconhecido: %s\n", command);
        }
    }

    return 0;
}