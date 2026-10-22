# はじめての PC で1回だけ実行する、開発環境の設定スクリプト（メンバー・管理者共通）
# 使い方: gh repo clone <組織名>/<リポジトリ名> でクローンしたフォルダで
#         .\tools\setup-member.ps1
# 何度実行しても大丈夫。

. "$PSScriptRoot\common.ps1"
Set-Location $RepoRoot
$cfg = Get-TeamConfig

foreach ($c in "git", "gh") {
    if (-not (Get-Command $c -ErrorAction SilentlyContinue)) {
        Stop-Team "$c が見つかりません。" "メンバー向けガイドの「1. インストール」をやってから、PowerShell を開き直してください"
    }
}

Write-Step "Git の設定"
if (-not (git config --global user.name)) {
    $n = Read-Host "GitHub のユーザー名を入力してください"
    git config --global user.name $n
}
if (-not (git config --global user.email)) {
    $m = Read-Host "GitHub に登録したメールアドレスを入力してください"
    git config --global user.email $m
}
git config --global core.autocrlf true
git config --global credential.helper manager
git config --global pull.ff only
git config --global init.defaultBranch main
Write-Ok "user.name=$(git config --global user.name) / user.email=$(git config --global user.email)"

Write-Step "GitHub へのログイン"
gh auth status 2>$null | Out-Null
if ($LASTEXITCODE -ne 0) {
    gh auth login --web --git-protocol https
    if ($LASTEXITCODE -ne 0) { Stop-Team "ログインできませんでした。" "もう一度実行してください" }
}
gh auth setup-git 2>$null | Out-Null
Write-Ok "ログイン済み"

Write-Step "Git LFS（画像・モデル・音声用）"
Git-Run lfs install
Git-Run lfs pull
Write-Ok "LFS を有効にしました"

Write-Step "$($cfg.developBranch) を最新にします"
Git-Run fetch --prune origin
$current = Get-CurrentBranch
if ($current -eq $cfg.developBranch -or $current -eq $cfg.mainBranch -or -not $current) {
    if (Test-Clean) {
        if (Test-LocalBranch $cfg.developBranch) { Git-Run switch $cfg.developBranch; Git-Run merge --ff-only "origin/$($cfg.developBranch)" }
        else { Git-Run switch --track "origin/$($cfg.developBranch)" }
    }
}

Write-Host ""
& "$PSScriptRoot\check-env.ps1"
if ($LASTEXITCODE -ne 0) { exit 1 }

Write-Host ""
Write-Ok "準備完了です。作業を始めるときは:"
Write-Host "  .\tools\start-work.ps1 <作業名>    （例: .\tools\start-work.ps1 player-move）"
