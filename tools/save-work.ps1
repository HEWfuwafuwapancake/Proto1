# 作業を保存して GitHub に送るスクリプト（こまめに実行してOK）
# 使い方: .\tools\save-work.ps1 "やったこと"
#         .\tools\save-work.ps1 "やったこと" -NoProjectCheck   … プロジェクトに登録しないファイルを、わざと入れるとき
param([Parameter(Position = 0)][string]$Message = "", [switch]$NoProjectCheck)

. "$PSScriptRoot\common.ps1"
Set-Location $RepoRoot
$cfg = Get-TeamConfig

$current = Get-CurrentBranch
if (-not $current) { Stop-Team "ブランチにいません（detached HEAD）。" "管理者に相談してください" }
if ($current -eq $cfg.mainBranch -or $current -eq $cfg.developBranch) {
    Stop-Team "$current には直接保存できません。" ".\tools\start-work.ps1 <作業名> を実行すると、今の変更ごとフィーチャーブランチに移れます"
}
if ($Message -eq "") { $Message = "作業の保存 " + (Get-Date -Format "yyyy-MM-dd HH:mm") }

git add -A

# 50MB を超えるファイルが LFS 以外で入ろうとしていないか（GitHub は 100MB 超を拒否する）
$big = @()
foreach ($f in (git -c core.quotePath=false diff --cached --name-only --diff-filter=AM)) {
    if (-not (Test-Path -LiteralPath $f)) { continue }
    $size = (Get-Item -LiteralPath $f).Length
    $isLfs = (git check-attr filter -- $f) -match "filter: lfs$"
    if ($size -gt 50MB -and -not $isLfs) { $big += "$f ($([math]::Round($size / 1MB)) MB)" }
}
if ($big.Count -gt 0) {
    git reset -q
    Stop-Team "大きいファイルが LFS の対象になっていません: $($big -join ', ')" "管理者に .gitattributes への追加を頼んでください"
}

# 新しく追加した C++ / シェーダーのファイルが、Visual Studio のプロジェクト（.vcxproj）に登録されているか
# （登録されていないと、他の人が取り込んでも Visual Studio に表示されず、ビルドもされない）
if (-not $NoProjectCheck) {
    $projects = @(Get-ChildItem $RepoRoot -Filter *.vcxproj -Recurse | Where-Object { $_.FullName -notmatch '\\(\.vs|x64|x86|Debug|Release)\\' })
    $missing = @()
    foreach ($f in (git -c core.quotePath=false diff --cached --name-only --diff-filter=A)) {
        if ($f -notmatch '\.(c|cc|cpp|cxx|h|hh|hpp|hxx|inl|hlsl|hlsli|fx)$') { continue }
        $full = Join-Path $RepoRoot ($f -replace '/', '\')
        $registered = $false
        foreach ($p in $projects) {
            # .vcxproj には、.vcxproj のあるフォルダからの相対パスで書かれている（例: Include="src\Player.cpp"）
            $rel = [Uri]::UnescapeDataString((New-Object Uri ($p.DirectoryName + "\")).MakeRelativeUri((New-Object Uri $full)).ToString()) -replace '/', '\'
            if (Select-String -LiteralPath $p.FullName -SimpleMatch "Include=`"$rel`"" -Quiet) { $registered = $true; break }
        }
        if (-not $registered) { $missing += $f }
    }
    if ($projects.Count -gt 0 -and $missing.Count -gt 0) {
        git reset -q
        Write-Host "✖ 次のファイルが Visual Studio のプロジェクトに登録されていません:" -ForegroundColor Red
        $missing | ForEach-Object { Write-Host "    $_" -ForegroundColor Red }
        Write-Host "  → Visual Studio のソリューションエクスプローラーで「ソース ファイル」か「ヘッダー ファイル」を右クリック" -ForegroundColor DarkGray
        Write-Host "    → 追加 → 既存の項目 で追加し、Ctrl+Shift+S（すべて保存）してから、もう一度実行してください" -ForegroundColor DarkGray
        Write-Host "  → わざとプロジェクトに入れないファイルなら: .\tools\save-work.ps1 `"$Message`" -NoProjectCheck" -ForegroundColor DarkGray
        exit 1
    }
}

git diff --cached --quiet
if ($LASTEXITCODE -ne 0) {
    Git-Run commit -q -m $Message
    Write-Ok "コミットしました: $Message"
} else {
    Write-Host "新しい変更はありません。"
}

Write-Step "GitHub に送っています"
git push -u origin $current
if ($LASTEXITCODE -ne 0) {
    Stop-Team "GitHub に送れませんでした。" "別のPCで同じブランチを更新していれば .\tools\start-work.ps1 で取り込んでから、もう一度実行してください"
}
Write-Ok "$current を GitHub に保存しました。"
