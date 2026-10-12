param([switch]$Build, [switch]$Smoke)
$observerRoot = Split-Path -Parent $PSScriptRoot
$observerGodot = Join-Path $observerRoot 'local/tools/godot-4.3/Godot_v4.3-stable_win64_console.exe'
Push-Location -LiteralPath $observerRoot
try {
    if ($Build) {
        & cmake --preset godot-debug
        if ($LASTEXITCODE -ne 0) { throw 'Godot bridge configure failed' }
        & cmake --build --preset godot-debug --parallel 4
        if ($LASTEXITCODE -ne 0) { throw 'Godot bridge build failed' }
    }
    if (-not (Test-Path -LiteralPath $observerGodot)) { throw 'Pinned Godot 4.3 runtime missing; see docs/DEVELOPMENT.md' }
    if ($Smoke) {
        $observerOutput = & $observerGodot --headless --path godot res://scenes/main_loop_observer.tscn --quit-after 120 -- --observer-smoke 2>&1
        $observerExit = $LASTEXITCODE
        $observerOutput | Write-Output
        $observerLog = $observerOutput -join "`n"
        if ($observerExit -ne 0 -or $observerLog -match 'ERROR:|SCRIPT ERROR' -or $observerLog -notmatch 'MAIN_LOOP_OBSERVER_SMOKE_OK') {
            throw 'Observer smoke failed; inspect the output above'
        }
    } else {
        & $observerGodot --path godot res://scenes/main_loop_observer.tscn
        if ($LASTEXITCODE -ne 0) { throw 'Observer exited with an error' }
    }
} finally { Pop-Location }
