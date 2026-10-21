param(
    [Parameter(Mandatory=$true)][string]$SourceBin,
    [Parameter(Mandatory=$true)][string]$ProjectPath
)
# Port-side Windows runtime staging; keep extension and transitive DLLs together.
$ErrorActionPreference = 'Stop'
$runtimeSource = (Resolve-Path -LiteralPath $SourceBin).Path
$runtimeProject = (Resolve-Path -LiteralPath $ProjectPath).Path
if (-not (Test-Path -LiteralPath (Join-Path $runtimeSource 'ps2_port.dll'))) { throw 'Source has no ps2_port.dll' }
$runtimeTarget = Join-Path $runtimeProject 'bin'
New-Item -ItemType Directory -Path $runtimeTarget -Force | Out-Null
foreach ($runtimeDll in Get-ChildItem -LiteralPath $runtimeSource -Filter '*.dll' -File) {
    Copy-Item -LiteralPath $runtimeDll.FullName -Destination (Join-Path $runtimeTarget $runtimeDll.Name) -Force
}
foreach ($requiredDll in @('ps2_port.dll','avcodec-62.dll','avutil-60.dll','swscale-9.dll','swresample-6.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $runtimeTarget $requiredDll))) { throw "Missing runtime DLL: $requiredDll" }
}
Get-ChildItem -LiteralPath $runtimeTarget -Filter '*.dll' -File | Get-FileHash -Algorithm SHA256 | Select-Object Hash,Path
