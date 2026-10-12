param([switch]$Build, [switch]$Smoke, [switch]$MenuSmoke, [switch]$ActivationSmoke, [switch]$AnimationSmoke, [switch]$RuntimeSmoke, [switch]$SubmenuSmoke, [int]$Screen = -1, [string]$ScreenName, [int]$Scale = 2, [string]$Capture, [string]$MenuEvents)
$root = Split-Path -Parent $PSScriptRoot
$godot = Join-Path $root 'local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe'
Push-Location -LiteralPath $root
try {
    if ($Build) {
        & cmake --preset godot-debug
        if ($LASTEXITCODE -ne 0) { throw 'Godot bridge configure failed' }
        & cmake --build --preset godot-debug --parallel 4
        if ($LASTEXITCODE -ne 0) { throw 'Godot bridge build failed' }
    }
    if (-not (Test-Path -LiteralPath $godot)) { throw 'Pinned Godot 4.4.1 runtime missing; see docs/DEVELOPMENT.md' }
    if ($ActivationSmoke -or $AnimationSmoke -or $RuntimeSmoke -or $SubmenuSmoke) {
        $flag = if ($SubmenuSmoke) { "--submenu-smoke" } elseif ($RuntimeSmoke) { '--runtime-smoke' } elseif ($ActivationSmoke) { '--activation-smoke' } else { '--animation-smoke' }
        $marker = if ($SubmenuSmoke) { "FRONTEND_SUBMENU_SMOKE_OK" } elseif ($RuntimeSmoke) { 'FRONTEND_RUNTIME_SMOKE_OK' } elseif ($ActivationSmoke) { 'FRONTEND_ACTIVATION_SMOKE_OK' } else { 'FRONTEND_ANIMATION_SMOKE_OK' }
        $out = & $godot --headless --path godot res://scenes/frontend.tscn --quit-after 600 -- $flag 2>&1
        $code = $LASTEXITCODE
        $out | Write-Output
        $log = $out -join "`n"
        if ($code -ne 0 -or $log -match 'ERROR:|SCRIPT ERROR' -or $log -notmatch $marker) {
            throw 'Frontend integration smoke failed; inspect the output above'
        }
    } elseif ($MenuSmoke) {
        $out = & $godot --headless --path godot res://scenes/frontend.tscn --quit-after 600 -- --menu-smoke 2>&1
        $code = $LASTEXITCODE
        $out | Write-Output
        $log = $out -join "`n"
        if ($code -ne 0 -or $log -match 'ERROR:|SCRIPT ERROR' -or $log -notmatch 'FRONTEND_MENU_SMOKE_OK') {
            throw 'Frontend menu smoke failed; inspect the output above'
        }
    } elseif ($Smoke) {
        $out = & $godot --headless --path godot res://scenes/frontend.tscn --quit-after 600 -- --frontend-smoke 2>&1
        $code = $LASTEXITCODE
        $out | Write-Output
        $log = $out -join "`n"
        if ($code -ne 0 -or $log -match 'ERROR:|SCRIPT ERROR' -or $log -notmatch 'FRONTEND_SMOKE_OK') {
            throw 'Frontend smoke failed; inspect the output above'
        }
    } elseif ($Capture) {
        $args = @("--scale=$Scale", "--no-info", "--capture=$Capture")
        if ($Screen -ge 0) { $args += "--screen=$Screen" }
        if ($MenuEvents) { $args += "--menu-events=$MenuEvents" }
        $out = & $godot --path godot res://scenes/frontend.tscn --quit-after 600 -- @args 2>&1
        $out | Write-Output
        if ($LASTEXITCODE -ne 0 -or ($out -join "`n") -notmatch 'FRONTEND_CAPTURE_OK') { throw 'Frontend capture failed' }
    } else {
        $extra = @("--scale=$Scale")
        if ($Screen -ge 0) { $extra += "--screen=$Screen" }
        if ($ScreenName) { $extra = @("--screen-name=$ScreenName", "--scale=$Scale") }
        & $godot --path godot res://scenes/frontend.tscn -- @extra
        if ($LASTEXITCODE -ne 0) { throw 'Frontend exited with an error' }
    }
} finally { Pop-Location }
