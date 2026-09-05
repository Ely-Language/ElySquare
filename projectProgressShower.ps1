param (
    [Parameter(Mandatory=$false)]
    [string]$TargetDirectory = "."
)

$extensions = @("*.cpp", "*.h", "*.hpp")
$files = Get-ChildItem -Path $TargetDirectory -Include $extensions -Recurse -File

$filled = @()
$empty = @()

foreach ($file in $files) {
    # Читаем содержимое, учитывая пустые файлы и файлы только с пробелами/переносами строк
    $content = Get-Content -LiteralPath $file.FullName -Raw -ErrorAction SilentlyContinue
    if ($null -ne $content -and $content.Trim().Length -gt 0) {
        $filled += $file.FullName
    } else {
        $empty += $file.FullName
    }
}

Write-Host "=== СТАТИСТИКА C++ ФАЙЛОВ ===" -ForegroundColor Cyan
[PSCustomObject]@{
    "Всего файлов"   = $files.Count
    "Заполненные"    = $filled.Count
    "Пустые"         = $empty.Count
} | Format-List

if ($empty.Count -gt 0) {
    Write-Host "`nПустые файлы ($($empty.Count)):" -ForegroundColor Yellow
    $empty | ForEach-Object { Write-Host "  $_" }
} else {
    Write-Host "`nПустые файлы отсутствуют." -ForegroundColor Green
}

if ($filled.Count -gt 0) {
    Write-Host "`nЗаполненные файлы ($($filled.Count)):" -ForegroundColor Green
    $filled | ForEach-Object { Write-Host "  $_" }
}