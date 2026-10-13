param([switch]$Build, [switch]$RenderSmoke, [switch]$RuntimeSmoke, [int]$Screen = -1, [string]$ScreenName, [int]$Scale = 2, [string]$Capture, [string]$MenuEvents)
# TASK-0018: the front end runs through scenes/frontend_native.tscn -- the native runtime and the
# RenderingServer backend. The menu, activation, submenu, localisation, audio, animation and
# event-tree checks that used to live here as GDScript smokes are now native and run under
# `ctest --preset native-debug` (frontend_runtime / frame_builder). What is left needs the host.
$root = Split-Path -Parent $PSScriptRoot
$godot = Join-Path $root 'local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe'
$scene = 'res://scenes/frontend_native.tscn'
Push-Location -LiteralPath $root
try {
    if ($Build) {
        & cmake --preset godot-debug
        if ($LASTEXITCODE -ne 0) { throw 'Godot bridge configure failed' }
        & cmake --build --preset godot-debug --parallel 4
        if ($LASTEXITCODE -ne 0) { throw 'Godot bridge build failed' }
    }
    if (-not (Test-Path -LiteralPath $godot)) { throw 'Pinned Godot 4.4.1 runtime missing; see docs/DEVELOPMENT.md' }
    if ($RenderSmoke -or $RuntimeSmoke) {
        $flag = if ($RuntimeSmoke) { '--runtime-smoke' } else { '--native-render-smoke' }
        $marker = if ($RuntimeSmoke) { 'FRONTEND_RUNTIME_SMOKE_OK' } else { 'NATIVE_RENDER_SMOKE_OK' }
        $out = & $godot --headless --path godot $scene --quit-after 600 -- $flag 2>&1
        $code = $LASTEXITCODE
        $out | Write-Output
        $log = $out -join "`n"
        if ($code -ne 0 -or $log -match 'ERROR:|SCRIPT ERROR' -or $log -notmatch $marker) {
            throw 'Frontend host smoke failed; inspect the output above'
        }
    } elseif ($Capture) {
        $args = @("--scale=$Scale", "--capture=$Capture")
        if ($Screen -ge 0) { $args += "--screen=$Screen" }
        if ($MenuEvents) { $args += "--menu-events=$MenuEvents" }
        $out = & $godot --path godot $scene --quit-after 600 -- @args 2>&1
        $out | Write-Output
        if ($LASTEXITCODE -ne 0 -or ($out -join "`n") -notmatch 'FRONTEND_CAPTURE_OK') { throw 'Frontend capture failed' }
    } else {
        $extra = @("--scale=$Scale")
        if ($Screen -ge 0) { $extra += "--screen=$Screen" }
        if ($ScreenName) { $extra = @("--screen-name=$ScreenName", "--scale=$Scale") }
        & $godot --path godot $scene -- @extra
        if ($LASTEXITCODE -ne 0) { throw 'Frontend exited with an error' }
    }
} finally { Pop-Location }
