# 【管理者用】メンバーのプルリクエストを develop にマージする
# 使い方:
#   .\tools\admin\merge-pr.ps1          … develop 向けの開いているプルリクエストを一覧表示
#   .\tools\admin\merge-pr.ps1 12       … 12番を確認してマージし、フィーチャーブランチを消す
# ブラウザの「Merge pull request」ボタンでマージしても同じ。
param([Parameter(Position = 0)][string]$Number = "")

. "$PSScriptRoot\admin-common.ps1"
Set-Location $RepoRoot
Assert-Admin
$cfg = Get-TeamConfig

if (-not $Number) {
    gh pr list --base $cfg.developBranch --state open
    Write-Host ""
    Write-Host "中身を見る:       gh pr view <番号> --web"
    Write-Host "手元で動かす:     gh pr checkout <番号>"
    Write-Host "マージする:       .\tools\admin\merge-pr.ps1 <番号>"
    exit 0
}

$pr = gh pr view $Number --json number,title,author,headRefName,baseRefName,mergeable | ConvertFrom-Json
if (-not $pr) { Stop-Team "プルリクエスト $Number が見つかりません。" }
Write-Host "#$($pr.number) $($pr.title)  （$($pr.author.login): $($pr.headRefName) → $($pr.baseRefName)）"

if ($pr.baseRefName -ne $cfg.developBranch -or $pr.headRefName -notlike "feature/*") {
    Stop-Team "feature/* → $($cfg.developBranch) 以外のプルリクエストです。" "release / hotfix は release.ps1 / hotfix.ps1 の finish を使ってください"
}
if ($pr.mergeable -eq "CONFLICTING") {
    Stop-Team "$($cfg.developBranch) と競合しています。" "作者に .\tools\start-work.ps1 で develop を取り込み、競合を直してもらってください"
}

$ans = Read-Host "マージしますか？ (y/N)"
if ($ans -ne "y") { Write-Host "やめました。"; exit 0 }

gh pr merge $Number --merge --admin --delete-branch
if ($LASTEXITCODE -ne 0) { Stop-Team "マージできませんでした。" }
Write-Ok "#$Number を $($cfg.developBranch) にマージしました。"
