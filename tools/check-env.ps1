# 開発環境がそろっているかを確認するスクリプト
# 使い方: .\tools\check-env.ps1
# 表示の意味: [OK] 問題なし / [NG] 直す必要あり / [注意] 動くが確認したほうがいい

. "$PSScriptRoot\common.ps1"
Set-Location $RepoRoot
$cfg = Get-TeamConfig

$script:ngCount = 0
$script:warnCount = 0

function Show-Result([string]$status, [string]$item, [string]$detail, [string]$fix = "") {
    switch ($status) {
        "OK"   { Write-Host "[OK]   " -ForegroundColor Green -NoNewline }
        "NG"   { Write-Host "[NG]   " -ForegroundColor Red -NoNewline; $script:ngCount++ }
        "WARN" { Write-Host "[注意] " -ForegroundColor Yellow -NoNewline; $script:warnCount++ }
    }
    Write-Host "$item : $detail"
    if ($fix -and $status -ne "OK") { Write-Host "         → $fix" -ForegroundColor DarkGray }
}

function Test-Command([string]$name) { return [bool](Get-Command $name -ErrorAction SilentlyContinue) }

Write-Host ""
Write-Host "=== PowerShell ===" -ForegroundColor Cyan

$psv = $PSVersionTable.PSVersion
if ($psv.Major -eq 5 -and $psv.Minor -eq 1) {
    Show-Result "OK" "PowerShellのバージョン" "$psv"
} else {
    Show-Result "WARN" "PowerShellのバージョン" "$psv（チームの基準は 5.1）" "スタートメニューの「Windows PowerShell」で実行するとチームと同じになる"
}

$cuPolicy = Get-ExecutionPolicy -Scope CurrentUser
if ($cuPolicy -in @("RemoteSigned", "Unrestricted", "Bypass")) {
    Show-Result "OK" "スクリプトの実行許可（CurrentUser）" "$cuPolicy"
} else {
    Show-Result "NG" "スクリプトの実行許可（CurrentUser）" "$cuPolicy" "Set-ExecutionPolicy -Scope CurrentUser RemoteSigned"
}

# BOM がないと Windows PowerShell 5.1 で日本語が化けて文法エラーになる
$bad = @()
foreach ($f in Get-ChildItem (Join-Path $RepoRoot "tools") -Filter *.ps1 -Recurse) {
    $bytes = [IO.File]::ReadAllBytes($f.FullName)
    $hasBom = $bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF
    $errors = $null
    [System.Management.Automation.PSParser]::Tokenize((Get-Content $f.FullName -Raw -Encoding UTF8), [ref]$errors) | Out-Null
    if (-not $hasBom -or $errors.Count -gt 0) { $bad += $f.Name }
}
if ($bad.Count -eq 0) {
    Show-Result "OK" "tools のスクリプト" "BOM付きUTF-8、文法エラーなし"
} else {
    Show-Result "NG" "tools のスクリプト" "$($bad -join ', ') が壊れている" "git checkout origin/$($cfg.developBranch) -- tools で元に戻す"
}

Write-Host ""
Write-Host "=== Git ===" -ForegroundColor Cyan

if (Test-Command git) {
    Show-Result "OK" "Git" ((git --version) -replace "git version ", "")

    git lfs version 2>$null | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Show-Result "OK" "Git LFS" (((git lfs version) -split " ")[0])
    } else {
        Show-Result "NG" "Git LFS" "使えない" "Git for Windows を入れ直す（LFS は同梱されている）"
    }

    $name = git config --global user.name
    $mail = git config --global user.email
    if ($name) { Show-Result "OK" "user.name" $name } else { Show-Result "NG" "user.name" "未設定" 'git config --global user.name "GitHubのユーザー名"' }
    if ($mail) { Show-Result "OK" "user.email" $mail } else { Show-Result "NG" "user.email" "未設定" 'git config --global user.email "GitHubに登録したメール"' }

    # 改行コードの自動変換（人によって違うと、中身が同じでも全行が変更扱いになる）
    $autocrlf = git config core.autocrlf
    if ($autocrlf -eq "true") { Show-Result "OK" "core.autocrlf" "true" } else { Show-Result "NG" "core.autocrlf" "'$autocrlf'" "git config --global core.autocrlf true" }

    $cred = git config credential.helper
    if ($cred -in @("manager", "manager-core")) { Show-Result "OK" "credential.helper" $cred } else { Show-Result "WARN" "credential.helper" "'$cred'" "git config --global credential.helper manager" }
} else {
    Show-Result "NG" "Git" "インストールされていない" "winget install --id Git.Git -e を実行し、PowerShellを開き直す"
}

Write-Host ""
Write-Host "=== GitHub CLI ===" -ForegroundColor Cyan

if (Test-Command gh) {
    Show-Result "OK" "GitHub CLI" (((gh --version)[0] -split " ")[2])
    $status = (gh auth status 2>&1) -join "`n"
    if ($status -match "Logged in to github.com account (\S+)") {
        Show-Result "OK" "GitHubログイン" $Matches[1]
    } else {
        Show-Result "NG" "GitHubログイン" "ログインしていない" "gh auth login --web --git-protocol https"
    }
} else {
    Show-Result "NG" "GitHub CLI" "インストールされていない" "winget install --id GitHub.cli -e を実行し、PowerShellを開き直す"
}

Write-Host ""
Write-Host "=== Visual Studio / DirectX 11 ===" -ForegroundColor Cyan

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
$vs = if (Test-Path $vswhere) { & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format value -property displayName } else { "" }
if ($vs) {
    Show-Result "OK" "Visual Studio（C++）" "$vs"
} else {
    Show-Result "NG" "Visual Studio（C++）" "C++ が使える Visual Studio が見つからない" "Visual Studio Installer で「C++ によるデスクトップ開発」を入れる"
}
$sdk = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\Include"
$sdkVer = if (Test-Path $sdk) { Get-ChildItem $sdk -Directory | Where-Object { Test-Path (Join-Path $_.FullName "um\d3d11.h") } | Sort-Object Name | Select-Object -Last 1 } else { $null }
if ($sdkVer) {
    Show-Result "OK" "Windows SDK（d3d11.h）" $sdkVer.Name
} else {
    Show-Result "NG" "Windows SDK（d3d11.h）" "見つからない" "Visual Studio Installer で Windows 11 SDK を入れる"
}

Write-Host ""
Write-Host "=== このリポジトリ ===" -ForegroundColor Cyan

git rev-parse --is-inside-work-tree 2>$null | Out-Null
if ($LASTEXITCODE -eq 0) {
    Show-Result "OK" "git管理" $RepoRoot

    $origin = git remote get-url origin 2>$null
    $expected = if ($cfg.org) { "https://github.com/$($cfg.org)/$($cfg.repo).git" } else { "" }
    if (-not $origin) {
        Show-Result "NG" "接続先（origin）" "未設定" "管理者は .\tools\admin\setup-repo.ps1、メンバーは gh repo clone で取り直す"
    } elseif ($expected -and $origin -ne $expected) {
        Show-Result "WARN" "接続先（origin）" "$origin（チームの設定は $expected）" "git remote set-url origin $expected"
    } else {
        Show-Result "OK" "接続先（origin）" $origin
        git ls-remote --heads origin $cfg.developBranch 2>$null | Out-Null
        if ($LASTEXITCODE -eq 0) { Show-Result "OK" "GitHubへの接続" "できる" } else { Show-Result "NG" "GitHubへの接続" "できない" "ネットワーク、gh auth status、招待を承認したかを確認" }
    }

    $hook = Join-Path (git rev-parse --git-dir) "hooks\pre-push"
    if ((Test-Path $hook) -and (Select-String -Path $hook -Pattern "git lfs" -Quiet)) {
        Show-Result "OK" "Git LFS の有効化" "このリポジトリで有効"
    } else {
        Show-Result "NG" "Git LFS の有効化" "このリポジトリで無効" "git lfs install; git lfs pull"
    }

    $branch = Get-CurrentBranch
    if ($branch -like "feature/*") {
        Show-Result "OK" "今のブランチ" $branch
    } elseif ($branch -eq $cfg.developBranch -or $branch -eq $cfg.mainBranch) {
        Show-Result "WARN" "今のブランチ" "$branch（ここでは作業しない）" ".\tools\start-work.ps1 <作業名> でフィーチャーブランチを作る"
    } else {
        Show-Result "OK" "今のブランチ" $branch
    }
} else {
    Show-Result "NG" "git管理" "このフォルダはgitで管理されていない" "メンバーは gh repo clone <組織名>/<リポジトリ名> で取り直す"
}

Write-Host ""
if ($script:ngCount -eq 0 -and $script:warnCount -eq 0) {
    Write-Host "環境は整っています。" -ForegroundColor Green
} elseif ($script:ngCount -eq 0) {
    Write-Host "使えますが、確認したほうがいい点が $($script:warnCount) 件あります。" -ForegroundColor Yellow
} else {
    Write-Host "直す必要がある項目が $($script:ngCount) 件あります。→ の指示に従って直し、もう一度実行してください。" -ForegroundColor Red
    exit 1
}
