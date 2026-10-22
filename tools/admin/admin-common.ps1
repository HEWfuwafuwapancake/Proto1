# 管理者用スクリプトが共通で使う関数（直接は実行しない）

. "$PSScriptRoot\..\common.ps1"

# このリポジトリの管理者（Admin 権限）でなければ止める
function Assert-Admin {
    $cfg = Get-TeamConfig
    if (-not $cfg.org) { Stop-Team "tools/team.json がありません。" "先に .\tools\admin\setup-repo.ps1 を実行してください" }
    $isAdmin = gh api "repos/$($cfg.org)/$($cfg.repo)" --jq ".permissions.admin" 2>$null
    if ($isAdmin -ne "true") { Stop-Team "このスクリプトは管理者しか使えません。" "管理者に頼んでください" }
}

# head → base のプルリクエストを作って（あれば再利用して）マージする
function Merge-ByPr([string]$head, [string]$base, [string]$title, [string]$body) {
    Git-Run fetch -q origin
    if (Test-Contains "origin/$base" "origin/$head") { Write-Ok "$head はすでに $base に入っています。"; return }

    $url = gh pr list --head $head --base $base --state open --json url --jq ".[].url" | Select-Object -First 1
    if (-not $url) {
        $url = gh pr create --head $head --base $base --title $title --body $body
        if ($LASTEXITCODE -ne 0) { Stop-Team "$head → $base のプルリクエストを作れませんでした。" }
    }
    Write-Step "$head → $base をマージします: $url"
    gh pr merge $url --merge --admin
    if ($LASTEXITCODE -ne 0) {
        Stop-Team "$head → $base をマージできませんでした（競合の可能性）。" "git switch $head; git merge origin/$base で競合を直して push し、もう一度同じコマンドを実行してください"
    }
}

# release / hotfix ブランチを作る
function Start-FlowBranch([string]$kind, [string]$version, [string]$from) {
    if ($version -notmatch '^\d+\.\d+(\.\d+)?$') { Stop-Team "バージョンは 1.0 や 1.0.1 の形で指定してください。" }
    if (-not (Test-Clean)) { Stop-Team "保存していない変更があります。" "先にコミットするか、変更を戻してください" }
    $branch = "$kind/$version"
    Git-Run fetch --prune origin
    if (Test-RemoteBranch $branch) { Stop-Team "$branch はもうあります。" "git switch $branch で切り替えてください" }
    if (git tag --list "v$version") { Stop-Team "v$version はもうリリース済みです。" }
    Git-Run switch -c $branch "origin/$from"
    Git-Run push -u origin $branch
    Write-Ok "$branch を origin/$from から作りました。"
}

# release / hotfix ブランチを main と develop に入れて、タグを付けて消す
function Complete-FlowBranch([string]$kind) {
    $cfg = Get-TeamConfig
    $branch = Get-CurrentBranch
    if ($branch -notlike "$kind/*") { Stop-Team "$kind/〇〇 ブランチにいません（今は '$branch'）。" "git switch $kind/<バージョン> で切り替えてから実行してください" }
    $version = $branch.Substring($kind.Length + 1)
    if (-not (Test-Clean)) { Stop-Team "保存していない変更があります。" "git add -A; git commit -m `"...`" で保存してください" }
    Git-Run push -u origin $branch

    $label = if ($kind -eq "release") { "リリース" } else { "緊急修正" }
    Merge-ByPr $branch $cfg.mainBranch "$label v$version" "$branch を $($cfg.mainBranch) に入れて v$version として公開します。"

    Git-Run fetch -q origin
    if (-not (git tag --list "v$version")) {
        Git-Run tag -a "v$version" "origin/$($cfg.mainBranch)" -m "$label v$version"
        Git-Run push origin "v$version"
        Write-Ok "タグ v$version を付けました。"
    }

    Merge-ByPr $branch $cfg.developBranch "$label v$version を $($cfg.developBranch) に反映" "$branch の修正を $($cfg.developBranch) にも入れます。"

    Git-Run fetch -q origin
    if (Test-LocalBranch $cfg.developBranch) { Git-Run switch $cfg.developBranch; Git-Run merge -q --ff-only "origin/$($cfg.developBranch)" }
    else { Git-Run switch --track "origin/$($cfg.developBranch)" }
    git push -q origin --delete $branch 2>$null
    git branch -D $branch | Out-Null
    Write-Ok "$label v$version が完了しました（$branch は削除しました）。"
}
