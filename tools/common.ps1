# tools/ のスクリプトが共通で使う関数（直接は実行しない）
# 各スクリプトの先頭で  . "$PSScriptRoot\common.ps1"  のように読み込む

# git / gh の日本語出力を文字化けさせない
[Console]::OutputEncoding = [Text.Encoding]::UTF8
$OutputEncoding = [Text.Encoding]::UTF8

# インストール直後でもコマンドが見つかるように、PATHを読み直す
$env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [Environment]::GetEnvironmentVariable("Path", "User")

# リポジトリのルート（tools フォルダの1つ上）
$script:RepoRoot = Split-Path -Parent $PSScriptRoot
$script:ConfigPath = Join-Path $PSScriptRoot "team.json"

function Write-Step([string]$text) { Write-Host "▶ $text" -ForegroundColor Cyan }
function Write-Ok([string]$text)   { Write-Host "✔ $text" -ForegroundColor Green }
function Write-Warn([string]$text) { Write-Host "⚠ $text" -ForegroundColor Yellow }

# エラーを表示して終了する
function Stop-Team([string]$text, [string]$hint = "") {
    Write-Host "✖ $text" -ForegroundColor Red
    if ($hint) { Write-Host "  → $hint" -ForegroundColor DarkGray }
    exit 1
}

# git を実行し、失敗したら止める
function Git-Run {
    & git @args
    if ($LASTEXITCODE -ne 0) { Stop-Team "git $($args -join ' ') に失敗しました。" }
}

# チームの設定（tools/team.json）を読む。管理者の setup-repo.ps1 が作る
function Get-TeamConfig {
    $cfg = [ordered]@{ org = ""; repo = ""; mainBranch = "main"; developBranch = "develop" }
    if (Test-Path $script:ConfigPath) {
        $json = Get-Content $script:ConfigPath -Raw -Encoding UTF8 | ConvertFrom-Json
        foreach ($p in $json.PSObject.Properties) { $cfg[$p.Name] = $p.Value }
    }
    return $cfg
}

function Get-CurrentBranch { return (git branch --show-current 2>$null) }

# まだコミットしていない変更がなければ $true
function Test-Clean { return -not (git status --porcelain) }

function Test-LocalBranch([string]$name)  { git show-ref --verify --quiet "refs/heads/$name";         return $LASTEXITCODE -eq 0 }
function Test-RemoteBranch([string]$name) { git show-ref --verify --quiet "refs/remotes/origin/$name"; return $LASTEXITCODE -eq 0 }

# $ancestor が $ref に含まれていれば $true
function Test-Contains([string]$ref, [string]$ancestor) {
    git merge-base --is-ancestor $ancestor $ref 2>$null
    return $LASTEXITCODE -eq 0
}

# 作業名をブランチ名に使える形かチェックする（英小文字・数字・- _ .）
function Assert-BranchName([string]$name) {
    if ($name -notmatch '^[a-z0-9][a-z0-9._-]*$') {
        Stop-Team "作業名 '$name' はブランチ名に使えません。" "英小文字・数字・ - _ . だけで付けてください（例: player-move, title-scene）"
    }
}
