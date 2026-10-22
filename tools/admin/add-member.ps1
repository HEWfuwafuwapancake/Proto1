# 【管理者用】リポジトリにメンバーを招待する
# 使い方:
#   .\tools\admin\add-member.ps1 suzuki           … 開発者として招待（feature/* で作業・プルリクエストまで）
#   .\tools\admin\add-member.ps1 tanaka -Admin    … 管理者として招待（develop / main へのマージ、release / hotfix）
#   .\tools\admin\add-member.ps1 -List            … 今のメンバーと権限を表示
param(
    [Parameter(Position = 0)][string]$User = "",
    [switch]$Admin,
    [switch]$List
)

. "$PSScriptRoot\admin-common.ps1"
Set-Location $RepoRoot
$cfg = Get-TeamConfig
if (-not $cfg.org) { Stop-Team "tools/team.json がありません。" "先に .\tools\admin\setup-repo.ps1 を実行してください" }
$full = "$($cfg.org)/$($cfg.repo)"

if ($List -or -not $User) {
    Write-Host "=== メンバー ==="
    (gh api "repos/$full/collaborators" | ConvertFrom-Json) | ForEach-Object { Write-Host "  $($_.login)`t$($_.role_name)" }
    Write-Host "=== 招待中 ==="
    (gh api "repos/$full/invitations" | ConvertFrom-Json) | ForEach-Object { Write-Host "  $($_.invitee.login)`t$($_.permissions)" }
    exit 0
}

# GitHub の権限名: admin = 管理者 / push = 書き込み（開発者）
$perm = if ($Admin) { "admin" } else { "push" }
$role = if ($Admin) { "管理者" } else { "開発者" }

gh api "users/$User" --jq ".login" 2>$null | Out-Null
if ($LASTEXITCODE -ne 0) { Stop-Team "GitHub ユーザー '$User' が見つかりません。" "ユーザー名のつづりを確認してください" }

gh api -X PUT "repos/$full/collaborators/$User" -f "permission=$perm" | Out-Null
if ($LASTEXITCODE -ne 0) { Stop-Team "$User を招待できませんでした。" }
# 組織のメンバーは招待なしですぐ追加される。招待中の一覧にいるかで分ける
$invited = @(gh api "repos/$full/invitations" --jq ".[].invitee.login" 2>$null) | Where-Object { $_ -eq $User }
if ($invited) {
    Write-Ok "$User を${role}として招待しました（本人がメールか https://github.com/$full/invitations で承認すると有効）。"
} else {
    Write-Ok "$User を${role}として追加しました（すぐ使えます）。"
}
