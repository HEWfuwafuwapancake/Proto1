# 作業を始めるときに実行するスクリプト
# 使い方:
#   .\tools\start-work.ps1 player-move   … develop の最新から feature/player-move を作って切り替える
#                                          （すでにあれば切り替えて develop の最新を取り込む）
#   .\tools\start-work.ps1               … 今のフィーチャーブランチに develop の最新を取り込む
param([Parameter(Position = 0)][string]$Name = "")

. "$PSScriptRoot\common.ps1"
Set-Location $RepoRoot
$cfg = Get-TeamConfig
$dev = $cfg.developBranch

Write-Step "GitHub の最新を取得しています"
git fetch --prune origin
if ($LASTEXITCODE -ne 0) { Stop-Team "GitHub に接続できませんでした。" "ネットワークと gh auth status を確認してください" }

$current = Get-CurrentBranch

if ($Name -eq "") {
    if ($current -notlike "feature/*") {
        Write-Host "使い方: .\tools\start-work.ps1 <作業名>   （例: .\tools\start-work.ps1 player-move）"
        $mine = git branch --list "feature/*" --format "%(refname:short)"
        if ($mine) { Write-Host "このPCにあるフィーチャーブランチ:"; $mine | ForEach-Object { Write-Host "  $_" } }
        exit 1
    }
    $target = $current
} else {
    $Name = $Name -replace '^feature/', ''
    Assert-BranchName $Name
    $target = "feature/$Name"
}

$created = $false
if ($target -ne $current) {
    if (Test-LocalBranch $target) {
        if (-not (Test-Clean)) { Stop-Team "保存していない変更があります。" "先に .\tools\save-work.ps1 `"メモ`" で保存してから切り替えてください" }
        Git-Run switch $target
    } elseif (Test-RemoteBranch $target) {
        if (-not (Test-Clean)) { Stop-Team "保存していない変更があります。" "先に .\tools\save-work.ps1 `"メモ`" で保存してから切り替えてください" }
        Git-Run switch --track "origin/$target"
    } else {
        # 新しく作る。保存していない変更があれば、そのまま新しいブランチに持っていく
        # （develop で間違えて作業を始めてしまったときも、これで救える）
        Write-Step "origin/$dev から $target を作ります"
        git switch --no-track -c $target "origin/$dev"
        if ($LASTEXITCODE -ne 0) {
            Stop-Team "ブランチを作れませんでした。" "保存していない変更が develop の最新とぶつかっています。管理者に相談してください"
        }
        git push -u origin $target
        if ($LASTEXITCODE -ne 0) { Stop-Team "GitHub にブランチを作れませんでした。" "リポジトリへの招待を承認しているか確認してください" }
        $created = $true
    }
}

if (-not $created) {
    if (-not (Test-Clean)) {
        Write-Warn "保存していない変更があるため、最新の取り込みは飛ばしました（保存後にもう一度実行すると取り込めます）。"
    } else {
        # 別のPCで push した自分の作業を取り込む
        if ((Test-RemoteBranch $target) -and -not (Test-Contains HEAD "origin/$target")) {
            Write-Step "GitHub 上の $target の変更を取り込みます"
            git merge --no-edit "origin/$target"
            if ($LASTEXITCODE -ne 0) { Stop-Team "競合しました。" "VS で競合したファイルを直し、.\tools\save-work.ps1 `"競合を解決`" を実行してください" }
        }
        # develop の最新を取り込む
        if (-not (Test-Contains HEAD "origin/$dev")) {
            Write-Step "$dev の最新を取り込みます"
            git merge --no-edit "origin/$dev"
            if ($LASTEXITCODE -ne 0) { Stop-Team "$dev の最新と競合しました。" "VS で競合したファイルを直し、.\tools\save-work.ps1 `"競合を解決`" を実行してください" }
        }
    }
}

Write-Ok "$target で作業できます。"
Write-Host "  保存:       .\tools\save-work.ps1 `"やったこと`""
Write-Host "  レビュー依頼: .\tools\submit-pr.ps1"
git log --oneline -3
