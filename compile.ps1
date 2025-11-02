# Define o compilador
$CC = "gcc"

# Define as flags (para incluir os headers do diretório atual)
$CFLAGS = "-I."

# Limpa os executáveis antigos, se existirem 
Remove-Item -Path ".\mkfs.exe", ".\shell.exe" -ErrorAction SilentlyContinue

Write-Host "Compilando mkfs..." -ForegroundColor Cyan
& $CC -o mkfs mkfs.c $CFLAGS

Write-Host "Compilando shell..." -ForegroundColor Cyan
& $CC -o shell shell.c disk_ops.c commands.c $CFLAGS

# Verifica se a compilação foi bem-sucedida
if ((Test-Path ".\mkfs.exe") -and (Test-Path ".\shell.exe")) {
    Write-Host ""
    Write-Host "Compilacao concluida!" -ForegroundColor Green
    Write-Host "Executaveis 'mkfs.exe' e 'shell.exe' criados."
    Write-Host ""
    
    # --- EXECUTAR OS ARQUIVOS MKFS E SHELL ---
    
    Write-Host "Executando mkfs.exe para formatar o filesystem..." -ForegroundColor Yellow
    
    # Executa o mkfs
    ./mkfs.exe
    
    Write-Host ""
    Write-Host "Iniciando o shell.exe..." -ForegroundColor Yellow
    Write-Host "--------------------------------"
    
    # Executa o shell
    # O script PowerShell vai pausar aqui e entregar o controle ao shell.exe
    ./shell.exe
    
    Write-Host "--------------------------------"
    Write-Host "Shell finalizado." -ForegroundColor Yellow
    
} else {
    Write-Host ""
    Write-Host "Erro durante a compilação. Os programas não serão executados." -ForegroundColor Red
}