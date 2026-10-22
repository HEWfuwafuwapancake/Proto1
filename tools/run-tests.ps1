# 単体テストをビルドして実行するスクリプト
# 使い方:
#   .\tools\run-tests.ps1              … Debug でビルドしてテストする
#   .\tools\run-tests.ps1 -Release     … Release でビルドしてテストする
# tests\ の中の .vcxproj（テストのプロジェクト）を全部ビルドして、まとめて実行する。
# Visual Studio の「テスト エクスプローラー」から実行しても同じテストが動く。
param([switch]$Release)

. "$PSScriptRoot\common.ps1"
Set-Location $RepoRoot
$config = if ($Release) { "Release" } else { "Debug" }

# Visual Studio に入っている MSBuild と vstest.console を探す
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { Stop-Team "Visual Studio が見つかりません。" "Visual Studio 2022 を入れてください" }
$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
$vstest = & $vswhere -latest -find "**\TestWindow\vstest.console.exe" | Select-Object -First 1
if (-not $msbuild) { Stop-Team "MSBuild が見つかりません。" "Visual Studio Installer で「C++ によるデスクトップ開発」を入れてください" }
if (-not $vstest) { Stop-Team "vstest.console.exe が見つかりません。" "Visual Studio Installer で「テスト ツール」を入れてください" }

$projects = @(Get-ChildItem (Join-Path $RepoRoot "tests") -Filter *.vcxproj -Recurse -ErrorAction SilentlyContinue)
if ($projects.Count -eq 0) { Stop-Team "tests の中にテストのプロジェクトがありません。" }

# ビルド
$dlls = @()
foreach ($p in $projects) {
    Write-Step "$($p.BaseName) をビルドしています（$config）"
    # 毎回作り直す（ヘッダーだけ変えたときに「最新」と判断されて、古いテストが動くのを防ぐ。テストは小さいので数秒）
    $out = & $msbuild $p.FullName /t:Rebuild /p:Configuration=$config /p:Platform=x64 /v:minimal /nologo 2>&1
    if ($LASTEXITCODE -ne 0) {
        $out | Select-String -Pattern "error" | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
        Stop-Team "$($p.BaseName) のビルドに失敗しました。"
    }
    $dlls += Join-Path $p.DirectoryName "bin\$config\$($p.BaseName).dll"
}

# 実行
Write-Step "テストを実行しています"
$resultsArg = "/ResultsDirectory:" + (Join-Path $RepoRoot "TestResults")	# 1つの引数になるよう、先に文字列にする
& $vstest @dlls /Platform:x64 $resultsArg
if ($LASTEXITCODE -ne 0) {
    Stop-Team "失敗したテストがあります。" "上の「Failed」の行を見て直してください"
}
Write-Ok "すべてのテストが通りました。"
