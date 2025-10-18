# PealFS - Sistema de Arquivos Customizado em C


![C](https://img.shields.io/badge/language-C-blue) ![Status](https://img.shields.io/badge/status-em%20desenvolvimento-yellow)
<br><br>
 📋 Índice
- [Sobre o Projeto](#sobre-o-projeto)
- [Características](#características)
- [Arquitetura do Sistema](#arquitetura-do-sistema)
- [Comandos Disponíveis](#comandos-disponíveis)
- [Instalação e Compilação](#instalação-e-compilação)
- [Uso](#uso)
- [Detalhes de Implementação](#detalhes-de-implementação)
- [Estrutura do Código](#estrutura-do-código)
- [Licença](#licença)

---

## 🎯 Sobre o Projeto

**PealFS** é um sistema de arquivos didático implementado em C que simula as operações fundamentais de um filesystem real. Desenvolvido como trabalho acadêmico para a disciplina de Sistemas Operacionais 2, o projeto demonstra conceitos como i-nodes, alocação de blocos, gerenciamento de espaço livre via bitmap e persistência de dados.

O sistema opera sobre o filesystem nativo do SO, armazenando todos os dados dentro do diretório `fs/`, garantindo isolamento e persistência entre execuções.

### Objetivos do Projeto

- Implementar um interpretador de comandos (shell) funcional
- Demonstrar gerenciamento de i-nodes e blocos de dados
- Implementar persistência completa de dados
- Aplicar conceitos de estruturas de dados para organização de diretórios
- Gerenciar espaço livre através de bitmap

---

## ✨ Características

- ✅ **Shell interativo** com 8 comandos funcionais
- ✅ **Persistência completa** de dados entre execuções
- ✅ **Gerenciamento via bitmap** para alocação de blocos
- ✅ **I-nodes** para representação de arquivos e diretórios
- ✅ **Navegação hierárquica** com suporte a caminhos relativos e absolutos
- ✅ **Estrutura de diretórios** com entradas `.` e `..`
- ✅ **Tamanho de bloco fixo** (128 bytes)
- ✅ **Limite de partição** configurável (10240 bytes / 80 blocos)

***

## 🏗️ Arquitetura do Sistema

### Estrutura do Filesystem

```
fs/
 ├─ superblock.dat     # Metadados: filesystem=pealfs, blocksize, partitionsize
 ├─ freespace.dat      # Bitmap de 10 bytes (80 bits) para gerenciamento de blocos
 ├─ inodes.dat         # Tabela de 256 i-nodes (cada um com 12 bytes)
 └─ blocks.dat         # 80 blocos de 128 bytes cada (10240 bytes total)
```

### Componentes Principais

#### 1. Superbloco (`superblock.dat`)
Armazena os metadados do sistema:
```
filesystem=pealfs
blocksize=128
partitionsize=10240
```

#### 2. I-nodes (`inodes.dat`)
Cada i-node contém:
- **type**: `'f'` (arquivo), `'d'` (diretório), `'u'` (não utilizado)
- **size**: Tamanho em bytes
- **block_count**: Número de blocos alocados
- **direct_blocks**: Ponteiros para até 4 blocos de dados[1]

#### 3. Bitmap (`freespace.dat`)
- 10 bytes (80 bits) representam os 80 blocos
- Bit `1` = bloco ocupado
- Bit `0` = bloco livre
- Usa MSB-first (bit mais significativo primeiro)

#### 4. Blocos de Dados (`blocks.dat`)
- Arquivo contíguo com 80 blocos de 128 bytes
- Bloco 0 reservado para o diretório raiz

### Estrutura de Diretórios

Cada diretório contém entradas no formato:

```c
typedef struct {
    char name[14];        // Nome do arquivo/diretório (máx 14 bytes)
    uint8_t inode_number; // Número do i-node (0-255)
} DirectoryEntry;
```

Diretórios sempre possuem:
- `.` → referência ao próprio diretório
- `..` → referência ao diretório pai

***

## 🖥️ Comandos Disponíveis

| Comando | Sintaxe | Descrição | Exemplo |Status |
|---------|---------|-----------|---------|--------|
| **mkdir** | `mkdir <nome>` | Cria um novo diretório no diretório atual | `$ mkdir usuarios` | ✅ Feito |
| **cd** | `cd <caminho>` | Altera o diretório atual (suporta `/`, `..`, caminhos relativos) | `$ cd /usuarios` | ✅ Feito |
| **pwd** | `pwd` | Exibe o caminho do diretório atual | `$ pwd` → `/usuarios` | ✅ Feito |
| **touch** | `touch <arquivo>` | Cria arquivo e lê conteúdo até `<CTRL+D>` | `$ touch teste.txt` + conteúdo + `<CTRL+D>` | ✅ Feito |
| **cat** | `cat <arquivo>` | Exibe o conteúdo de um arquivo | `$ cat teste.txt` | ✅ Feito |
| **ls** | `ls` | Lista arquivos e diretórios com tipo, i-node e tamanho | `$ ls` | ✅ Feito |
| **rm** | `rm <nome>` | Remove arquivo ou diretório vazio | `$ rm arquivo.txt` | ✅ Feito |
| **stat** | `stat` | Mostra espaço livre, blocos livres e tamanho do bloco | `$ stat` | ✅ Feito |
| **exit** | `exit` | Encerra o shell | `$ exit` | ✅ Feito |

***

## 🚀 Instalação e Compilação

### Pré-requisitos

- **Compilador GCC** ou compatível
- Sistema operacional: Windows (com MinGW) ou Linux

### Passo a Passo

1. **Clone o repositório:**
```bash
git clone https://github.com/seu-usuario/pealfs.git
cd pealfs
```

2. **Compile o formatador do filesystem:**
```bash
gcc -o mkfs mkfs.c -I.
```

3. **Formate o sistema de arquivos:**
```bash
./mkfs
```

Saída esperada:
```
Criando superblock.dat...
Criando freespace.dat...
Criando inodes.dat...
Criando blocks.dat...
Configurando o diretorio raiz (/)...

Sistema de arquivos 'pealfs' criado com sucesso no diretorio 'fs/'!
```

4. **Compile o shell:**
```bash
gcc -o shell shell.c disk_ops.c commands.c -I.
```

5. **Execute o shell:**
```bash
./shell
```

***

## 📖 Uso

### Exemplo de Sessão Interativa

```bash
$ ./shell
Bem-vindo ao pealfs!
Para sair, digite 'exit'.

peal:/ > mkdir documentos
Diretorio 'documentos' criado com sucesso!

peal:/ > cd documentos

peal:/ > touch arquivo.txt
Digite o conteúdo do arquivo (CTRL+D para finalizar):
Este é um teste do sistema de arquivos PealFS.
Arquivo 'arquivo.txt' criado com sucesso!

peal:/ > ls
TIPO - INODE - NOME          - TAMANHO
-------------------------------------------
d    - 0     - .             - 30
d    - 0     - ..            - 30
f    - 2     - arquivo.txt   - 48

peal:/ > cat arquivo.txt
Este é um teste do sistema de arquivos PealFS.

peal:/ > stat
Espaco livre: 9856 Bytes
Blocos livres: 77 Blocos
Tamanho do bloco: 128 Bytes

peal:/ > cd ..

peal:/ > pwd
/

peal:/ > exit
Saindo...
```

***

## 🔧 Detalhes de Implementação

### Alocação de Blocos

O sistema utiliza **busca linear** no bitmap para encontrar blocos livres:
```c
int find_free_block() {
    // Percorre os 10 bytes do bitmap
    // Retorna o primeiro bit com valor 0
}
```

### Busca em Diretórios

A implementação atual usa **busca sequencial** nas entradas de diretório. Cada entrada é verificada linearmente até encontrar o nome correspondente.

### Gerenciamento de I-nodes

- Total de 256 i-nodes disponíveis
- I-node 0 reservado para o diretório raiz
- Busca linear para encontrar i-nodes livres (type == 'u')

### Persistência

Todas as operações são imediatamente sincronizadas com os arquivos em `fs/`:
- Criação de arquivo → atualiza `inodes.dat`, `blocks.dat` e `freespace.dat`
- Remoção → libera i-node e blocos, atualiza bitmap
- Navegação → altera variável global `current_directory_inode`

***

## 📂 Estrutura do Código

```
pealfs/
├─ fs/                      # Diretório do filesystem (gerado pelo mkfs)
├─ fs.h                     # Definições de estruturas e constantes
├─ disk_ops.h               # Protótipos de operações de disco
├─ disk_ops.c               # Funções de leitura/escrita de blocos e i-nodes
├─ commands.h               # Protótipos dos comandos do shell
├─ commands.c               # Implementação dos comandos (mkdir, cd, ls, etc)
├─ shell.c                  # Loop principal do shell e parser de comandos
├─ mkfs.c                   # Formatador do filesystem
├─ gerarLogs.ps1               # Script PowerShell para compilação automatizada
└─ README.md
```

### Módulos Principais

#### `fs.h`
Define as estruturas de dados:
- `Inode`: Representa arquivos e diretórios
- `DirectoryEntry`: Mapeia nomes para i-nodes
- `Superblock`: Metadados do sistema
- Constantes: `BLOCK_SIZE`, `PARTITION_SIZE`, `MAX_INODES`, etc.

#### `disk_ops.c`
Funções de baixo nível:
- `read_block()` / `write_block()`: Acesso aos blocos de dados
- `read_inode()` / `write_inode()`: Acesso aos i-nodes
- `find_free_block()` / `find_free_inode()`: Localização de recursos livres
- `alloc_block()` / `free_block()`: Gerenciamento do bitmap

#### `commands.c`
Implementação dos comandos:
- `do_ls()`: Lista conteúdo do diretório
- `do_mkdir()`: Cria diretórios
- `do_cd()`: Navegação entre diretórios
- `do_touch()`: Criação de arquivos
- `do_cat()`: Leitura de arquivos
- `do_rm()`: Remoção de arquivos/diretórios
- `do_stat()`: Exibe estatísticas do sistema
- `do_pwd()`: Mostra diretório atual

#### `shell.c`
- Loop principal do shell
- Parser de comandos usando `strtok()`
- Dispatch para funções específicas
- Variável global `current_directory_inode`

***

## 🤝 Contribuindo

Contribuições são bem-vindas! Para contribuir:

1. Faça um fork do projeto
2. Crie uma branch para sua feature (`git checkout -b feature/nova-funcionalidade`)
3. Commit suas mudanças (`git commit -m 'Adiciona nova funcionalidade'`)
4. Push para a branch (`git push origin feature/nova-funcionalidade`)
5. Abra um Pull Request

***

## 📄 Licença

Distribuído sob a licença MIT. Veja `LICENSE` para mais informações.

***

## 👨‍💻 Autor

Desenvolvido como projeto acadêmico para a disciplina de Sistemas Operacionais 2.

**Prazo de entrega:** 20/10/2025 - 07h10  
**Peso:** 1,0

***

## 📚 Referências

- Tanenbaum, A. S. - *Modern Operating Systems*

***
