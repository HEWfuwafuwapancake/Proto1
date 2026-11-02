# ビルドの前に Proto1.vcxproj から呼ばれ、src のソースの中身から識別番号を作ってヘッダーに書く。
# オンラインで、ソースが違う exe どうしをつながないため（同じソースなら、どの PC でビルドしても同じ番号）。
# 改行（CRLF / LF）の違いは無視する。番号が変わらないときはファイルを書き換えない（毎回の作り直しを防ぐ）。
param(
	[Parameter(Mandatory = $true)][string]$Root,
	[Parameter(Mandatory = $true)][string]$OutFile
)
$ErrorActionPreference = 'Stop'

$src = Join-Path $Root 'src'
$prefix = (Get-Item $src).FullName.TrimEnd('\') + '\'	# FullName と同じ書き方（短いパス名を使わない）にそろえる
$files = Get-ChildItem $src -Recurse -File -Include *.cpp, *.h |
	ForEach-Object { [pscustomobject]@{ Name = $_.FullName.Substring($prefix.Length).Replace('\', '/'); Path = $_.FullName } } |
	Sort-Object Name -CaseSensitive

$latin1 = [System.Text.Encoding]::GetEncoding(28591)
$stream = New-Object System.IO.MemoryStream
foreach ($file in $files) {
	$name = [System.Text.Encoding]::UTF8.GetBytes($file.Name + "`n")
	$stream.Write($name, 0, $name.Length)
	# Latin-1 ならバイトと文字が1対1なので、CR だけを除いてそのままバイトに戻せる。
	$content = $latin1.GetString([System.IO.File]::ReadAllBytes($file.Path)).Replace("`r", '')
	$bytes = $latin1.GetBytes($content)
	$stream.Write($bytes, 0, $bytes.Length)
	$stream.WriteByte(10)
}
$sha = [System.Security.Cryptography.SHA256]::Create()
$hash = $sha.ComputeHash($stream.ToArray())
$id = [BitConverter]::ToUInt32($hash, 0)
if ($id -eq 0) { $id = 1 }

$text = "#pragma once`r`n// tools/make-sourceid.ps1 がビルドの前に作る（git には入れない）。src のソースの中身から作った識別番号。`r`n#define SOURCE_ID 0x{0:X8}u`r`n" -f $id
$dir = Split-Path $OutFile -Parent
if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force $dir | Out-Null }
$encoding = New-Object System.Text.UTF8Encoding($true)
if ((Test-Path $OutFile) -and ([System.IO.File]::ReadAllText($OutFile, $encoding) -eq $text)) { exit 0 }
[System.IO.File]::WriteAllText($OutFile, $text, $encoding)
Write-Host ("SOURCE_ID 0x{0:X8}" -f $id)
