# Diretório raiz do sistema de arquivos
$fsPath = "fs"

# Criar pasta de logs, se não existir
$logPath = "fs_logs"
if (!(Test-Path $logPath)) {
    New-Item -ItemType Directory -Path $logPath
}

# 1. Listar todos os arquivos e pastas recursivamente
Write-Output "=== Estrutura de arquivos e pastas ==="
Get-ChildItem -Path $fsPath -Recurse | Format-Table -AutoSize
Get-ChildItem -Path $fsPath -Recurse | Out-File -FilePath "$logPath/fs_structure.txt" -Encoding UTF8

# 2. Mostrar tamanho dos arquivos principais
Write-Output "=== Tamanho dos arquivos principais ==="
Get-Item "$fsPath/inodes.dat" | Format-List Name, Length
Get-Item "$fsPath/freespace.dat" | Format-List Name, Length
Get-Item "$fsPath/superblock.dat" | Format-List Name, Length | Out-File -FilePath "$logPath/fs_filesizes.txt" -Encoding UTF8

# 3. Exibir conteúdo hexadecimal do arquivo de inodes.dat
Write-Output "=== Conteúdo hexadecimal de inodes.dat ==="
Format-Hex -Path "$fsPath/inodes.dat"
Format-Hex -Path "$fsPath/inodes.dat" | Out-File -FilePath "$logPath/inodes_hex.txt" -Encoding UTF8

# 4. Mostrar conteúdo do superblock.dat (arquivo texto simples)
Write-Output "=== Conteúdo do superblock.dat ==="
Get-Content "$fsPath/superblock.dat"
Get-Content "$fsPath/superblock.dat" | Out-File -FilePath "$logPath/superblock_content.txt" -Encoding UTF8

# 5. Opcional: mostrar conteúdo de algum arquivo texto que exista para verificação
# Aqui não há .mickey.txt, substituímos por algum arquivo texto exemplo, caso exista. Se não existir, comentar.
# Write-Output "=== Conteúdo do arquivo exemplo ==="
# Get-Content "$fsPath/exemplo.txt"
# Get-Content "$fsPath/exemplo.txt" | Out-File -FilePath "$logPath/exemplo_content.txt" -Encoding UTF8

Write-Output "`nLogs salvos em $logPath"