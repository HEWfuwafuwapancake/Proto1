# 【管理者用】公開済みのバージョン（main）の緊急修正をする
# 使い方:
#   .\tools\admin\hotfix.ps1 start 1.0.1   … main から hotfix/1.0.1 を作る（ここで修正する）
#   .\tools\admin\hotfix.ps1 finish        … hotfix/1.0.1 を main に入れて v1.0.1 タグを付け、develop にも反映して消す
param(
    [Parameter(Mandatory = $true, Position = 0)][ValidateSet("start", "finish")][string]$Action,
    [Parameter(Position = 1)][string]$Version = ""
)

. "$PSScriptRoot\admin-common.ps1"
Set-Location $RepoRoot
Assert-Admin
$cfg = Get-TeamConfig

if ($Action -eq "start") {
    if (-not $Version) { Stop-Team "バージョンを指定してください。" ".\tools\admin\hotfix.ps1 start 1.0.1" }
    Start-FlowBranch "hotfix" $Version $cfg.mainBranch
    Write-Host "  修正が終わったら: .\tools\admin\hotfix.ps1 finish"
} else {
    Complete-FlowBranch "hotfix"
}
