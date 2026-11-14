$ErrorActionPreference = 'Stop'
$preset = 'godot-mvp-release'
$run = $false
$editor = $false
$clean = $false
$jobs = 8
$raceArgument = $null
$nativeDebug = $false
$fullscreen = $false
$fxaa = $false
$fps120 = $false
$renderingDriver = 'vulkan'
for ($index = 0; $index -lt $args.Count; $index++) {
    switch ($args[$index]) {
        '--mvp' { }
        '--run' { $run = $true }
        '--fullscreen' { $fullscreen = $true; $run = $true }
        '--fxaa' { $fxaa = $true; $run = $true }
        '--fps120' { $fps120 = $true; $run = $true }
        '--editor' { $editor = $true }
        { $_ -match '^--race-(track|event)=' } {
            if ($raceArgument) { throw 'Choose one race diagnostic.' }
            $raceArgument = [string]$_
            $run = $true
        }
        '--debug' { $run = $true; $preset = 'godot-mvp-debug'; $nativeDebug = $true }
        '--clean' { $clean = $true }
        '--rendering-driver' {
            if (++$index -ge $args.Count -or $args[$index] -notin @('vulkan', 'd3d12')) {
                throw '--rendering-driver requires vulkan or d3d12.'
            }
            $renderingDriver = $args[$index].ToLowerInvariant()
        }
        '--preset' {
            if (++$index -ge $args.Count) { throw '--preset requires a name.' }
            $preset = [string]$args[$index]
            if ($preset -in @('godot-release', 'godot-debug')) {
                $preset = $preset.Replace('godot-', 'godot-mvp-')
            }
        }
        '--jobs' {
            if (++$index -ge $args.Count -or
                -not [int]::TryParse([string]$args[$index], [ref]$jobs) -or $jobs -lt 1) {
                throw '--jobs requires a positive integer.'
            }
        }
        default { throw "Unknown option $($args[$index])." }
    }
}
if ($editor -and ($raceArgument -or $fullscreen -or $fxaa -or $fps120)) {
    throw '--editor cannot be combined with race diagnostics or game display options.'
}
if ($preset -notin @('godot-mvp-release', 'godot-mvp-debug')) { throw "Unsupported preset $preset." }

Push-Location -LiteralPath $PSScriptRoot
try {
    cmake --preset $preset
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
    if ($clean) {
        cmake --build --preset $preset --target clean
        if ($LASTEXITCODE) { exit $LASTEXITCODE }
    }
    cmake --build --preset $preset --target stage_mvp --parallel $jobs
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
    if ($run -or $editor) {
        $engine = Join-Path $PSScriptRoot 'local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe'
        if (-not (Test-Path -LiteralPath $engine -PathType Leaf)) { throw 'Godot 4.7.2 console executable is required.' }
        $project = Join-Path $PSScriptRoot "build/$preset/mvp-project"
        $engineArgs = @('--path', $project, '--rendering-driver', $renderingDriver)
        if ($editor) { $engineArgs += '--editor' }
        if ($fullscreen) { $engineArgs += '--fullscreen' }
        if ($fps120) { $engineArgs += @('--disable-vsync', '--max-fps', '120') }
        if ($nativeDebug -or $raceArgument -or $fxaa) { $engineArgs += '--' }
        if ($nativeDebug) { $engineArgs += '--debug' }
        if ($fxaa) { $engineArgs += '--fxaa' }
        if ($raceArgument) { $engineArgs += $raceArgument }
        & $engine @engineArgs
        exit $LASTEXITCODE
    }
} finally {
    Pop-Location
}
