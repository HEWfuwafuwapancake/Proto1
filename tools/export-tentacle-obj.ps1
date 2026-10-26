# 触腕のモデル（assets/models/source/takoasi.mb）を、ゲームで読む OBJ に書き出すスクリプト
# 使い方: .\tools\export-tentacle-obj.ps1
#   Maya（mayapy.exe）が入っている PC で使う。書き出すのは assets/models/ の
#   tentacle_base.obj（付け根）・tentacle_segment.obj（途中の節）・tentacle_tip.obj（先っぽ）。
#   Maya でモデルを直したら .mb を上書きして、これを実行し直す。中身は tools/maya/export_tentacle_obj.py。

. "$PSScriptRoot\common.ps1"
Set-Location $RepoRoot

$mayapy = Get-ChildItem "C:\Program Files\Autodesk\Maya*\bin\mayapy.exe" -ErrorAction SilentlyContinue |
    Sort-Object FullName -Descending | Select-Object -First 1
if (-not $mayapy) { Stop-Team "mayapy.exe が見つかりませんでした。" "Maya が入っている PC で実行してください" }

$source = Join-Path $RepoRoot "assets\models\source\takoasi.mb"
if (-not (Test-Path $source)) { Stop-Team "$source がありません。" "git lfs pull を試してください" }

Write-Step "$($mayapy.FullName) で OBJ を書き出しています（数十秒かかります）"
$oldEncoding = [Console]::OutputEncoding
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8   # mayapy は UTF-8 で表示する
try {
    & $mayapy.FullName -I (Join-Path $RepoRoot "tools\maya\export_tentacle_obj.py") $source (Join-Path $RepoRoot "assets\models")
} finally {
    [Console]::OutputEncoding = $oldEncoding
}
if ($LASTEXITCODE -ne 0) { Stop-Team "書き出しに失敗しました。" "上のエラーを確認してください" }
Write-Host "✔ assets/models/ に書き出しました。" -ForegroundColor Green
