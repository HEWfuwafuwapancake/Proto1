# 作業が終わったら、develop へのマージを管理者にお願いする（プルリクエストを作る）スクリプト
# 使い方:
#   .\tools\submit-pr.ps1                    … タイトルは最初のコミットのメッセージ
#   .\tools\submit-pr.ps1 "プレイヤーの移動"   … タイトルを指定
#   .\tools\submit-pr.ps1 -Web               … 作ったあとブラウザで開く
param([Parameter(Position = 0)][string]$Title = "", [switch]$Web)

. "$PSScriptRoot\common.ps1"
Set-Location $RepoRoot
$cfg = Get-TeamConfig
$dev = $cfg.developBranch

$current = Get-CurrentBranch
if ($current -notlike "feature/*") {
    Stop-Team "フィーチャーブランチ（feature/〇〇）にいません。今は '$current' です。" ".\tools\start-work.ps1 <作業名> で作業ブランチに移ってください"
}

# 保存していない変更があれば先に保存する
if (-not (Test-Clean)) { & "$PSScriptRoot\save-work.ps1" "プルリクエスト前の保存"; if ($LASTEXITCODE -ne 0) { exit 1 } }
Git-Run fetch -q origin
git push -q -u origin $current
if ($LASTEXITCODE -ne 0) { Stop-Team "GitHub に送れませんでした。" ".\tools\start-work.ps1 を実行してから、もう一度試してください" }

$commits = git log "origin/$dev..HEAD" --no-merges --reverse --format="%s"
if (-not $commits) { Stop-Team "$dev に対して新しいコミットがありません。" }

# すでにプルリクエストがあれば、それを表示するだけ
$existing = gh pr list --head $current --base $dev --state open --json url --jq ".[].url" | Select-Object -First 1
if ($existing) {
    Write-Ok "プルリクエストはもうあります（追加の保存は自動で反映されています）: $existing"
    if ($Web) { gh pr view $existing --web }
    exit 0
}

if ($Title -eq "") { $Title = @($commits)[0] }

$body = "## やったこと`n" + ((@($commits) | ForEach-Object { "- $_" }) -join "`n") + "`n`n" +
        "## 見てほしいところ`n- `n`n" +
        "## 確認したこと`n- [ ] Debug / Release でビルドが通る`n- [ ] 起動して動作を確認した`n"
$bodyFile = Join-Path $env:TEMP "team-pr-body.md"
[IO.File]::WriteAllText($bodyFile, $body, (New-Object Text.UTF8Encoding $false))

Write-Step "$current → $dev のプルリクエストを作ります"
$url = gh pr create --base $dev --head $current --title $Title --body-file $bodyFile
if ($LASTEXITCODE -ne 0) { Stop-Team "プルリクエストを作れませんでした。" "gh auth status でログインを確認してください" }
Remove-Item $bodyFile -ErrorAction SilentlyContinue

Write-Ok "プルリクエストを作りました: $url"
Write-Host "  管理者が確認して develop にマージします。直してと言われたら、そのまま作業して save-work.ps1 で保存すれば反映されます。"
if ($Web) { gh pr view $url --web }
