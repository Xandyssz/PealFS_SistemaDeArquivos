# Script de limpeza do projeto PealFS
# Remove arquivos compilados e diretório do filesystem

Write-Host "=== Limpando projeto PealFS ===" -ForegroundColor Cyan

# Remover executáveis
if (Test-Path "shell.exe") {
    Remove-Item "shell.exe" -Force
    Write-Host "  [OK] Removido: shell.exe" -ForegroundColor Green
}

if (Test-Path "mkfs.exe") {
    Remove-Item "mkfs.exe" -Force
    Write-Host "  [OK] Removido: mkfs.exe" -ForegroundColor Green
}

# Remover arquivos objeto (.o)
Get-ChildItem -Filter "*.o" | ForEach-Object {
    Remove-Item $_.FullName -Force
    Write-Host "  [OK] Removido: $($_.Name)" -ForegroundColor Green
}

# Remover diretório fs/ e logs
if (Test-Path "fs") {
    Remove-Item "fs" -Recurse -Force
    Write-Host "  [OK] Removido: fs/" -ForegroundColor Green
}

if (Test-Path "fs_logs") {
    Remove-Item "fs_logs" -Recurse -Force
    Write-Host "  [OK] Removido: fs_logs/" -ForegroundColor Green
}

Write-Host "`n=== Limpeza concluída! ===" -ForegroundColor Cyan
