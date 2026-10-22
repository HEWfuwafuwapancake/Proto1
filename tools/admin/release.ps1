# 【管理者用】リリースブランチを作る / 仕上げる
# 使い方:
#   .\tools\admin\release.ps1 start 1.0   … develop から release/1.0 を作る（ここでバグ修正・調整だけする）
#   .\tools\admin\release.ps1 finish      … release/1.0 を main に入れて v1.0 タグを付け、develop にも反映して消す
param(
    [Parameter(Mandatory = $true, Position = 0)][ValidateSet("start", "finish")][string]$Action,
    [Parameter(Position = 1)][string]$Version = ""
)

. "$PSScriptRoot\admin-common.ps1"
Set-Location $RepoRoot
Assert-Admin
$cfg = Get-TeamConfig

if ($Action -eq "start") {
    if (-not $Version) { Stop-Team "バージョンを指定してください。" ".\tools\admin\release.ps1 start 1.0" }
    Start-FlowBranch "release" $Version $cfg.developBranch
    Write-Host "  調整が終わったら: .\tools\admin\release.ps1 finish"
} else {
    Complete-FlowBranch "release"
}
