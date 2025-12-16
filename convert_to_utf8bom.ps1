$utf8BomEncoding = New-Object System.Text.UTF8Encoding($true)
$extensions = @('*.cpp', '*.h', '*.hpp', '*.c', '*.cc', '*.cxx')
$fileCount = 0
$errorCount = 0

Write-Host "UTF-8 BOM変換を開始します..." -ForegroundColor Cyan

foreach ($ext in $extensions) {
    Get-ChildItem -Path (Get-Location) -Filter $ext -File | ForEach-Object {
        try {
            $content = [System.IO.File]::ReadAllText($_.FullName)
            [System.IO.File]::WriteAllText($_.FullName, $content, $utf8BomEncoding)
            Write-Host "OK: $($_.Name)" -ForegroundColor Green
            $fileCount++
        } catch {
            Write-Host "ERROR: $($_.Name)" -ForegroundColor Red
            $errorCount++
        }
    }
}

Write-Host ""
Write-Host "変換完了: $fileCount ファイル" -ForegroundColor Green
if ($errorCount -gt 0) {
    Write-Host "エラー: $errorCount ファイル" -ForegroundColor Red
}