# 【管理者用】プロジェクトを GitHub に作り、ブランチと権限のルールを設定するスクリプト
# 使い方（プロジェクトのフォルダで、最初に1回）:
#   .\tools\admin\setup-repo.ps1 -Org <組織名> -Repo <リポジトリ名> -Admins <2人目の管理者> -Members <メンバー1>,<メンバー2>
# 例:
#   .\tools\admin\setup-repo.ps1 -Org HEW2026-TeamA -Repo HEW2026Game -Admins tanaka -Members suzuki,sato
# 何度実行しても大丈夫（足りないものだけ作り、ルールは最新の内容で上書きする）。
param(
    [Parameter(Mandatory = $true)][string]$Org,
    [Parameter(Mandatory = $true)][string]$Repo,
    [string[]]$Admins = @(),
    [string[]]$Members = @(),
    [switch]$Private,
    [string]$MainBranch = "main",
    [string]$DevelopBranch = "develop"
)

. "$PSScriptRoot\..\common.ps1"
Set-Location $RepoRoot

# ---------------------------------------------------------------
Write-Step "1. 準備の確認"
foreach ($c in "git", "gh") {
    if (-not (Get-Command $c -ErrorAction SilentlyContinue)) { Stop-Team "$c が見つかりません。" "管理者向けガイドの「1. インストール」を参照" }
}
gh auth status 2>$null | Out-Null
if ($LASTEXITCODE -ne 0) { Stop-Team "GitHub にログインしていません。" "gh auth login --web --git-protocol https" }
$me = gh api user --jq ".login"
$orgRole = gh api "user/memberships/orgs/$Org" --jq ".role" 2>$null
if ($orgRole -ne "admin") {
    Stop-Team "$me は組織 '$Org' のオーナーではありません（または組織がありません）。" "管理者向けガイドの「2. Organization を作る」を参照"
}
if (-not (git config --global user.name) -or -not (git config --global user.email)) {
    Stop-Team "Git の名前とメールアドレスが設定されていません。" "管理者向けガイドの「1-6. Git の初期設定」を参照"
}
git config --global core.autocrlf true
Write-Ok "$me（$Org のオーナー）として実行します。"

# ---------------------------------------------------------------
Write-Step "2. チームの設定ファイル（tools/team.json）"
$cfg = [ordered]@{ org = $Org; repo = $Repo; mainBranch = $MainBranch; developBranch = $DevelopBranch }
[IO.File]::WriteAllText($ConfigPath, (ConvertTo-Json -InputObject $cfg) + "`n", (New-Object Text.UTF8Encoding $false))
Write-Ok "$ConfigPath"
# ガイドの {{ORG}} {{REPO}} を実際の名前に置き換える
foreach ($md in Get-ChildItem (Join-Path $RepoRoot "docs\team") -Filter *.md -ErrorAction SilentlyContinue) {
    $text = [IO.File]::ReadAllText($md.FullName)
    $new = $text.Replace("{{ORG}}", $Org).Replace("{{REPO}}", $Repo)
    if ($new -ne $text) { [IO.File]::WriteAllText($md.FullName, $new, (New-Object Text.UTF8Encoding $false)); Write-Ok "docs\team\$($md.Name) に組織名・リポジトリ名を書き込みました" }
}

# ---------------------------------------------------------------
Write-Step "3. ローカルの git"
git rev-parse --is-inside-work-tree 2>$null | Out-Null
if ($LASTEXITCODE -ne 0) { Git-Run init -q -b $MainBranch }
Git-Run lfs install --local
git rev-parse --verify -q HEAD 2>$null | Out-Null
if ($LASTEXITCODE -ne 0) {
    Git-Run add -A
    Git-Run commit -q -m "プロジェクトの初期化"
    Write-Ok "最初のコミットを作りました。"
} elseif (-not (Test-Clean)) {
    Write-Warn "コミットしていない変更があります。GitHub には今までのコミットだけが送られます。"
}
if (-not (Test-LocalBranch $MainBranch)) {
    Git-Run branch -m $MainBranch
}

# ---------------------------------------------------------------
Write-Step "4. GitHub のリポジトリ"
$full = "$Org/$Repo"
gh repo view $full --json name 2>$null | Out-Null
if ($LASTEXITCODE -ne 0) {
    $vis = if ($Private) { "--private" } else { "--public" }
    if (git remote) { git remote remove origin }
    gh repo create $full $vis --source . --remote origin
    if ($LASTEXITCODE -ne 0) { Stop-Team "リポジトリを作れませんでした。" }
    Write-Ok "https://github.com/$full を作りました。"
} else {
    if (git remote) { Git-Run remote set-url origin "https://github.com/$full.git" } else { Git-Run remote add origin "https://github.com/$full.git" }
    Write-Ok "https://github.com/$full はもうあります。"
}

Git-Run fetch -q origin
if (-not (Test-RemoteBranch $MainBranch)) { Git-Run push -u origin $MainBranch }
if (-not (Test-RemoteBranch $DevelopBranch)) {
    if (-not (Test-LocalBranch $DevelopBranch)) { Git-Run branch $DevelopBranch $MainBranch }
    Git-Run push -u origin $DevelopBranch
}
Write-Ok "$MainBranch と $DevelopBranch を GitHub に置きました。"

# 既定ブランチを develop に（クローンすると develop が開き、プルリクエストも develop 向けになる）
# マージはマージコミットだけにする（図のように枝分かれの履歴が残る）
gh repo edit $full --default-branch $DevelopBranch --enable-merge-commit --enable-squash-merge=false --enable-rebase-merge=false --delete-branch-on-merge=false --enable-wiki=false | Out-Null
Write-Ok "既定ブランチ: $DevelopBranch / マージ方法: マージコミットのみ"

# ---------------------------------------------------------------
Write-Step "5. ブランチのルール（Rulesets）"
# 管理者 = リポジトリの Admin 権限を持つ人（actor_id 5）と、組織のオーナー
function New-Bypass([string]$mode) {
    return @(
        @{ actor_id = 5; actor_type = "RepositoryRole";    bypass_mode = $mode },
        @{ actor_id = 1; actor_type = "OrganizationAdmin"; bypass_mode = $mode }
    )
}
$rulesets = @(
    # main / develop：プルリクエストでしか変更できず、マージできるのは管理者だけ
    [ordered]@{
        name = "team: main と develop は管理者だけがマージ"; target = "branch"; enforcement = "active"
        conditions = @{ ref_name = @{ include = @("refs/heads/$MainBranch", "refs/heads/$DevelopBranch"); exclude = @() } }
        bypass_actors = New-Bypass "pull_request"
        rules = @(
            @{ type = "deletion" },
            @{ type = "non_fast_forward" },
            @{ type = "update"; parameters = @{ update_allows_fetch_and_merge = $false } },
            @{ type = "pull_request"; parameters = @{
                required_approving_review_count = 0; dismiss_stale_reviews_on_push = $false
                require_code_owner_review = $false; require_last_push_approval = $false
                required_review_thread_resolution = $false } }
        )
    },
    # release/* と hotfix/*：作る・更新する・消すのは管理者だけ
    [ordered]@{
        name = "team: release と hotfix は管理者だけ"; target = "branch"; enforcement = "active"
        conditions = @{ ref_name = @{ include = @("refs/heads/release/**/*", "refs/heads/hotfix/**/*"); exclude = @() } }
        bypass_actors = New-Bypass "always"
        rules = @( @{ type = "creation" }, @{ type = "update"; parameters = @{ update_allows_fetch_and_merge = $false } }, @{ type = "deletion" }, @{ type = "non_fast_forward" } )
    },
    # それ以外の名前のブランチ：メンバーは feature/* しか作れない
    [ordered]@{
        name = "team: メンバーが作れるのは feature だけ"; target = "branch"; enforcement = "active"
        conditions = @{ ref_name = @{ include = @("~ALL"); exclude = @("refs/heads/$MainBranch", "refs/heads/$DevelopBranch", "refs/heads/feature/**/*", "refs/heads/release/**/*", "refs/heads/hotfix/**/*") } }
        bypass_actors = New-Bypass "always"
        rules = @( @{ type = "creation" } )
    },
    # タグ（v1.0 など）：管理者だけ
    [ordered]@{
        name = "team: タグは管理者だけ"; target = "tag"; enforcement = "active"
        conditions = @{ ref_name = @{ include = @("~ALL"); exclude = @() } }
        bypass_actors = New-Bypass "always"
        rules = @( @{ type = "creation" }, @{ type = "update"; parameters = @{ update_allows_fetch_and_merge = $false } }, @{ type = "deletion" } )
    }
)

$existing = gh api "repos/$full/rulesets" 2>&1 | Out-String
if ($LASTEXITCODE -ne 0) {
    if ($existing -match "Upgrade|upgrade") {
        Write-Warn "このプランの private リポジトリではルールを設定できません（public にするか、組織を GitHub Team にする）。"
        Write-Warn "ルールなしで続けます。運用ルールとして守ってもらってください。"
        $existing = $null
    } else {
        Stop-Team "ルールの一覧を取得できませんでした: $existing"
    }
} else {
    $existing = $existing | ConvertFrom-Json
}
if ($null -ne $existing) {
    $tmp = Join-Path $env:TEMP "team-ruleset.json"
    foreach ($rs in $rulesets) {
        [IO.File]::WriteAllText($tmp, (ConvertTo-Json -InputObject $rs -Depth 10), (New-Object Text.UTF8Encoding $false))
        $found = @($existing) | Where-Object { $_.name -eq $rs.name } | Select-Object -First 1
        if ($found) { $out = gh api -X PUT "repos/$full/rulesets/$($found.id)" --input $tmp 2>&1 }
        else        { $out = gh api -X POST "repos/$full/rulesets" --input $tmp 2>&1 }
        if ($LASTEXITCODE -ne 0) { Stop-Team "ルール「$($rs.name)」を設定できませんでした: $out" }
        Write-Ok $rs.name
    }
    Remove-Item $tmp -ErrorAction SilentlyContinue
}

# ---------------------------------------------------------------
Write-Step "6. メンバーの招待"
foreach ($u in $Admins)  { & "$PSScriptRoot\add-member.ps1" -User $u -Admin }
foreach ($u in $Members) { & "$PSScriptRoot\add-member.ps1" -User $u }
if ($Admins.Count + $Members.Count -eq 0) { Write-Host "  （あとから .\tools\admin\add-member.ps1 <ユーザー名> で招待できます）" }

# ---------------------------------------------------------------
Write-Step "7. team.json などの設定を develop に入れる"
Git-Run fetch -q origin
git add tools/team.json docs/team
git diff --cached --quiet
if ($LASTEXITCODE -ne 0) {
    # develop には直接 push できないので、設定用のブランチを作ってプルリクエストでマージする
    $stash = $false
    if (-not (Test-Clean)) { git stash push -q -m "setup-repo"; $stash = $true }
    Git-Run switch -q -C "feature/setup-team-config" "origin/$DevelopBranch"
    if ($stash) { git stash pop -q }
    Git-Run add tools/team.json docs/team
    Git-Run commit -q -m "チームの設定（team.json とガイドの組織名）を更新"
    Git-Run push -q -u origin "feature/setup-team-config"
    $url = gh pr create --head "feature/setup-team-config" --base $DevelopBranch --title "チームの設定を追加" --body "setup-repo.ps1 が作成"
    gh pr merge $url --merge --admin --delete-branch | Out-Null
    if ($LASTEXITCODE -ne 0) { Write-Warn "自動マージに失敗しました。ブラウザで $url をマージしてください。" }
    Git-Run fetch -q --prune origin
    Git-Run switch -q $DevelopBranch
    Git-Run merge -q --ff-only "origin/$DevelopBranch"
}
git branch --set-upstream-to "origin/$DevelopBranch" $DevelopBranch 2>$null | Out-Null
git branch --set-upstream-to "origin/$MainBranch" $MainBranch 2>$null | Out-Null

Write-Host ""
Write-Ok "セットアップ完了: https://github.com/$full"
Write-Host "  ルールの確認:   https://github.com/$full/settings/rules"
Write-Host "  メンバーに渡す: docs/team/02_メンバー向け_環境構築.md"
