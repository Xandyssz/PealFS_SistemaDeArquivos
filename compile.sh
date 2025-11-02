#!/bin/bash

# Define o compilador
CC="gcc"

# Define as flags (para incluir os headers do diretório atual)
CFLAGS="-I."

# Limpa os executáveis antigos (sem .exe no Linux)
rm -f mkfs shell

echo "Compilando mkfs..."
$CC -o mkfs mkfs.c $CFLAGS

echo "Compilando shell..."
$CC -o shell shell.c disk_ops.c commands.c $CFLAGS

# Verifica se a compilação foi bem-sucedida
if [ -f "mkfs" ] && [ -f "shell" ]; then
    echo ""
    echo "Compilacao concluida!"

    echo ""
    echo "Executando mkfs para formatar o filesystem..."
    # Executa o mkfs
    ./mkfs

    echo ""
    echo "Iniciando o shell..."
    echo "--------------------------------"

    # Executa o shell
    ./shell

    echo "--------------------------------"
    echo "Shell finalizado."

else
    echo ""
    echo "Erro durante a compilacao."
fi