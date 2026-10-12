param([switch]$Build, [switch]$Smoke, [int]$Screen = 0, [int]$Scale = 2, [string]$Capture)
$root = Split-Path -Parent $PSScriptRoot
$godot = Join-Path $root 'local/tools/godot-4.3/Godot_v4.3-stable_win64_console.exe'
Push-Location -LiteralPath $root
try {
    if ($Build) {
        & cmake --preset godot-debug
        if ($LASTEXITCODE -ne 0) { throw 'Godot bridge configure failed' }
        & cmake --build --preset godot-debug --parallel 4
        if ($LASTEXITCODE -ne 0) { throw 'Godot bridge build failed' }
    }
    if (-not (Test-Path -LiteralPath $godot)) { throw 'Pinned Godot 4.3 runtime missing; see docs/DEVELOPMENT.md' }
    if ($Smoke) {
        $out = & $godot --headless --path godot res://scenes/frontend.tscn --quit-after 600 -- --frontend-smoke 2>&1
        $code = $LASTEXITCODE
        $out | Write-Output
        $log = $out -join "`n"
        if ($code -ne 0 -or $log -match 'ERROR:|SCRIPT ERROR' -or $log -notmatch 'FRONTEND_SMOKE_OK') {
            throw 'Frontend smoke failed; inspect the output above'
        }
    } elseif ($Capture) {
        $out = & $godot --path godot res://scenes/frontend.tscn --quit-after 600 -- "--screen=$Screen" "--scale=$Scale" --no-info "--capture=$Capture" 2>&1
        $out | Write-Output
        if ($LASTEXITCODE -ne 0 -or ($out -join "`n") -notmatch 'FRONTEND_CAPTURE_OK') { throw 'Frontend capture failed' }
    } else {
        & $godot --path godot res://scenes/frontend.tscn -- "--screen=$Screen" "--scale=$Scale"
        if ($LASTEXITCODE -ne 0) { throw 'Frontend exited with an error' }
    }
} finally { Pop-Location }
