# Script de debug do PealFS - Decodifica estrutura interna
# Execute do diretório raiz do projeto

# Definir caminhos
$fsPath = "fs"
$logPath = "fs_logs"

# Verificar se diretório fs existe
if (!(Test-Path $fsPath)) {
    Write-Host "ERRO: Diretório 'fs' não encontrado!" -ForegroundColor Red
    Write-Host "Execute o mkfs.exe primeiro para criar o filesystem." -ForegroundColor Yellow
    exit 1
}

# Criar pasta de logs
if (!(Test-Path $logPath)) {
    New-Item -ItemType Directory -Path $logPath | Out-Null
}

Write-Host "=== DEBUG DO PEALFS ===" -ForegroundColor Cyan
Write-Host ""

# 1. Estrutura física dos arquivos
Write-Host "1. Estrutura física de arquivos:" -ForegroundColor Yellow
Get-ChildItem -Path $fsPath | Format-Table Name, Length, LastWriteTime -AutoSize
Get-ChildItem -Path $fsPath | Format-Table Name, Length, LastWriteTime -AutoSize | Out-File "$logPath/fs_structure.txt" -Encoding UTF8

# 2. Conteúdo do superblock
Write-Host "`n2. Conteúdo do Superblock:" -ForegroundColor Yellow
$superblock = Get-Content "$fsPath/superblock.dat"
$superblock | ForEach-Object { Write-Host "   $_" }
$superblock | Out-File "$logPath/superblock.txt" -Encoding UTF8

# 3. Análise do Bitmap (freespace.dat)
Write-Host "`n3. Análise do Bitmap (blocos livres/ocupados):" -ForegroundColor Yellow
$bitmap = [System.IO.File]::ReadAllBytes("$fsPath/freespace.dat")

$output = @()
$output += "Byte | Binário    | Hex | Blocos (0=Livre, 1=Ocupado)"
$output += "-----+------------+-----+-----------------------------"

for ($i = 0; $i -lt $bitmap.Length; $i++) {
    $byte = $bitmap[$i]
    $binary = [Convert]::ToString($byte, 2).PadLeft(8, '0')
    $hex = $byte.ToString("X2")
    $blocks = ($i * 8)..($i * 8 + 7) -join ","
    $line = "{0,4} | {1} | {2}  | Blocos {3}: {4}" -f $i, $binary, $hex, $blocks, $binary
    $output += $line
    Write-Host "   $line"
}
$output | Out-File "$logPath/bitmap_analysis.txt" -Encoding UTF8

# Contar blocos livres
$freeBits = 0
foreach ($byte in $bitmap) {
    for ($bit = 0; $bit -lt 8; $bit++) {
        if (($byte -shr (7 - $bit)) -band 1 -eq 0) {
            $freeBits++
        }
    }
}
Write-Host "`n   Total de blocos livres: $freeBits / 80" -ForegroundColor Green

# 4. Análise dos Inodes (CORRIGIDO)
Write-Host "`n4. Análise dos I-nodes:" -ForegroundColor Yellow

# Ler arquivo de inodes
$inodesPath = "$fsPath/inodes.dat"
if (!(Test-Path $inodesPath)) {
    Write-Host "   ERRO: Arquivo inodes.dat não encontrado!" -ForegroundColor Red
} else {
    $inodes = [System.IO.File]::ReadAllBytes($inodesPath)
    
    $output = @()
    $output += "Inode | Tipo | Tamanho | Blocos | Ponteiros Diretos"
    $output += "------+------+---------+--------+------------------"
    
    # Detectar tamanho do inode
    $totalInodes = 256
    $inodeSize = [Math]::Floor($inodes.Length / $totalInodes)
    Write-Host "   Tamanho detectado do inode: $inodeSize bytes" -ForegroundColor Cyan
    
    for ($i = 0; $i -lt 10; $i++) {
        $offset = $i * $inodeSize
        
        if ($offset + $inodeSize -gt $inodes.Length) { break }
        
        # Ler campos
        $type = [char]$inodes[$offset]
        
        # Ajustar offsets baseado no tamanho real do inode
        if ($inodeSize -eq 20) {
            # Estrutura com padding/alinhamento
            $size = [BitConverter]::ToUInt32($inodes, $offset + 4)
            $blockCount = [BitConverter]::ToUInt16($inodes, $offset + 8)
            
            $pointers = @()
            for ($p = 0; $p -lt 4; $p++) {
                $ptrOffset = $offset + 10 + ($p * 2)
                if ($ptrOffset + 2 -le $inodes.Length) {
                    $ptr = [BitConverter]::ToUInt16($inodes, $ptrOffset)
                    $pointers += $ptr
                } else {
                    $pointers += 0
                }
            }
        } else {
            # Estrutura sem padding (12 bytes)
            $size = [BitConverter]::ToUInt32($inodes, $offset + 1)
            $blockCount = [BitConverter]::ToUInt16($inodes, $offset + 5)
            
            $pointers = @()
            for ($p = 0; $p -lt 4; $p++) {
                $ptrOffset = $offset + 7 + ($p * 2)
                if ($ptrOffset + 2 -le $inodes.Length) {
                    $ptr = [BitConverter]::ToUInt16($inodes, $ptrOffset)
                    $pointers += $ptr
                } else {
                    $pointers += 0
                }
            }
        }
        
        $ptrStr = $pointers -join ", "
        
        $typeStr = switch ($type) {
            'f' { "Arquivo" }
            'd' { "Dir" }
            'u' { "Livre" }
            default { "Desconhecido" }
        }
        
        # Mostrar apenas inodes em uso
        if ($type -eq 'f' -or $type -eq 'd') {
            $line = "{0,5} | {1,8} | {2,7} | {3,6} | [{4}]" -f $i, $typeStr, $size, $blockCount, $ptrStr
            $color = if ($type -eq 'd') { 'Cyan' } elseif ($type -eq 'f') { 'Green' } else { 'Gray' }
            Write-Host "   $line" -ForegroundColor $color
            $output += "{0,5} | {1} | {2,7} | {3,6} | [{4}]" -f $i, $type, $size, $blockCount, $ptrStr
        }
    }
    $output | Out-File "$logPath/inodes_analysis.txt" -Encoding UTF8
}

# 5. Conteúdo do bloco 0 (diretório raiz)
Write-Host "`n5. Conteúdo do Diretório Raiz (Bloco 0):" -ForegroundColor Yellow
$blocks = [System.IO.File]::ReadAllBytes("$fsPath/blocks.dat")

$output = @()
$output += "Nome              | Inode"
$output += "------------------+------"

$blockSize = 128
$entrySize = 15  # 14 bytes (nome) + 1 byte (inode)

for ($e = 0; $e -lt 8; $e++) {
    $offset = $e * $entrySize
    
    if ($offset + $entrySize -gt $blockSize) { break }
    
    # Ler nome (14 bytes)
    $nameBytes = $blocks[$offset..($offset + 13)]
    $name = [System.Text.Encoding]::ASCII.GetString($nameBytes).TrimEnd([char]0)
    
    # Ler inode (1 byte)
    $inodeNum = $blocks[$offset + 14]
    
    if ($name -ne "" -or ($name -eq "." -or $name -eq "..")) {
        $line = "{0,-18} | {1,5}" -f $name, $inodeNum
        $color = if ($name -eq "." -or $name -eq "..") { 'Cyan' } else { 'Green' }
        Write-Host "   $line" -ForegroundColor $color
        $output += $line
    }
}
$output | Out-File "$logPath/root_directory.txt" -Encoding UTF8

# 6. Dumps hexadecimais
Write-Host "`n6. Gerando dumps hexadecimais..." -ForegroundColor Yellow
Format-Hex -Path "$fsPath/inodes.dat" | Out-File "$logPath/inodes_hex.txt" -Encoding UTF8
Format-Hex -Path "$fsPath/blocks.dat" | Out-File "$logPath/blocks_hex.txt" -Encoding UTF8
Format-Hex -Path "$fsPath/freespace.dat" | Out-File "$logPath/freespace_hex.txt" -Encoding UTF8
Write-Host "   Dumps salvos em: $logPath/" -ForegroundColor Green

# Resumo
Write-Host "`n=== RESUMO ===" -ForegroundColor Cyan
Write-Host "Logs salvos em: $logPath/" -ForegroundColor Green
Write-Host "- fs_structure.txt      : Lista de arquivos físicos" -ForegroundColor Gray
Write-Host "- superblock.txt        : Metadados do filesystem" -ForegroundColor Gray
Write-Host "- bitmap_analysis.txt   : Análise do bitmap de blocos" -ForegroundColor Gray
Write-Host "- inodes_analysis.txt   : Lista de inodes em uso" -ForegroundColor Gray
Write-Host "- root_directory.txt    : Conteúdo do diretório raiz" -ForegroundColor Gray
Write-Host "- *_hex.txt             : Dumps hexadecimais" -ForegroundColor Gray
Write-Host ""
