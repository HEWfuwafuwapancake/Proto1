# 新しいアーム5種の Maya モデルをゲームで読む OBJ に書き出す。
# 使い方: .\tools\export-arm-obj.ps1（全部） / .\tools\export-arm-obj.ps1 grapple（1つ）
# -Inspect を付けると transform ごとの親・範囲・三角形数も表示する。
param(
    [Parameter(Position = 0)]
    [ValidateSet("all", "grapple", "drill", "shield", "magnet", "hasami")]
    [string]$Arm = "all",
    [switch]$Inspect
)

. "$PSScriptRoot\common.ps1"
Set-Location $RepoRoot

$mayapy = Get-ChildItem "C:\Program Files\Autodesk\Maya*\bin\mayapy.exe" -ErrorAction SilentlyContinue |
    Sort-Object FullName -Descending | Select-Object -First 1
if (-not $mayapy) { Stop-Team "mayapy.exe が見つかりませんでした。" "Maya が入っている PC で実行してください" }

$sourceDir = Join-Path $RepoRoot "assets\models\source"
$sources = @{ grapple = "grappling.mb"; drill = "doriru.mb"; shield = "dekaitate.mb"; magnet = "magnet.mb"; hasami = "hasami.mb" }
$names = if ($Arm -eq "all") { @("grapple", "drill", "shield", "magnet", "hasami") } else { @($Arm) }
foreach ($name in $names) {
    $source = Join-Path $sourceDir $sources[$name]
    if (-not (Test-Path -LiteralPath $source)) { Stop-Team "$source がありません。" "git lfs pull を試してください" }
}

Write-Step "$($mayapy.FullName) で $Arm の OBJ を書き出しています"
$oldEncoding = [Console]::OutputEncoding
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
try {
    $exportArgs = @("-I", (Join-Path $RepoRoot "tools\maya\export_arm_obj.py"), $sourceDir, (Join-Path $RepoRoot "assets\models"), $Arm)
    if ($Inspect) { $exportArgs += "--inspect" }
    & $mayapy.FullName @exportArgs
    $exportExitCode = $LASTEXITCODE
} finally {
    [Console]::OutputEncoding = $oldEncoding
}
if ($exportExitCode -ne 0) { Stop-Team "書き出しに失敗しました。" "上のエラーを確認してください" }
Write-Host "✔ assets/models/ に書き出しました。" -ForegroundColor Green
