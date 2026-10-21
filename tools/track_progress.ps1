# TASK-0073. Port-side progress entry point. No game behavior.
param(
    [switch]$Once,
    [switch]$Check,
    [switch]$RefreshGhidra,
    [string]$Python = '',
    [string]$Endpoint = 'http://127.0.0.1:8080/mcp'
)
$ErrorActionPreference = 'Stop'
$progressRoot = Split-Path -Parent $PSScriptRoot
if (-not $Python) { $Python = $env:HP2_PROGRESS_PYTHON }
if (-not $Python) {
    foreach ($progressPreset in @('native-debug', 'godot-debug', 'godot-release')) {
        $progressCache = Join-Path $progressRoot "build/$progressPreset/CMakeCache.txt"
        if (Test-Path -LiteralPath $progressCache) {
            $progressMatch = Select-String -LiteralPath $progressCache -Pattern '^Python3_EXECUTABLE:[^=]+=(.+)$' | Select-Object -First 1
            if ($progressMatch -and (Test-Path -LiteralPath $progressMatch.Matches[0].Groups[1].Value)) {
                $Python = $progressMatch.Matches[0].Groups[1].Value
                break
            }
        }
    }
}
if (-not $Python) {
    $progressCommand = Get-Command python -ErrorAction SilentlyContinue
    if ($progressCommand -and $progressCommand.Source -notlike '*WindowsApps*') { $Python = $progressCommand.Source }
}
if (-not $Python) { throw 'Python 3 is required. Set HP2_PROGRESS_PYTHON or pass -Python with an interpreter path.' }
if ($Once -and $Check) { throw 'Choose -Once or -Check.' }
$progressArguments = @('--watch')
if ($Once) { $progressArguments = @('--once') }
if ($Check) { $progressArguments = @('--check') }
if ($RefreshGhidra) { $progressArguments += '--refresh-ghidra' }
& $Python (Join-Path $progressRoot 'reverse/scripts/track_progress.py') @progressArguments --endpoint $Endpoint
if ($LASTEXITCODE -ne 0) { throw "Progress command failed (exit $LASTEXITCODE)." }
