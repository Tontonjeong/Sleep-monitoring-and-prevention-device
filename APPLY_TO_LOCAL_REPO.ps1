# Apply this enhanced portfolio package to your local GitHub clone.
# Usage in PowerShell:
#   powershell -ExecutionPolicy Bypass -File .\APPLY_TO_LOCAL_REPO.ps1

$Source = Split-Path -Parent $MyInvocation.MyCommand.Path
$Target = "C:\Users\ROK\Downloads\repo_push"

if (!(Test-Path $Target)) {
    Write-Host "Target repo not found: $Target" -ForegroundColor Red
    Write-Host "Run: git clone https://github.com/Tontonjeong/Sleep-monitoring-and-prevention-device.git C:\Users\ROK\Downloads\repo_push"
    exit 1
}

Write-Host "Copying enhanced repo files..." -ForegroundColor Cyan
robocopy $Source $Target /E /XD .git | Out-Host

Set-Location $Target
Write-Host "Git status:" -ForegroundColor Cyan
git status
Write-Host "Now run:" -ForegroundColor Yellow
Write-Host "git add -A"
Write-Host "git commit -m 'Add full code formulas pipelines and infographics'"
Write-Host "git push origin main"
