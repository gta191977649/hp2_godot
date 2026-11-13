$ErrorActionPreference = 'Stop'
$preset = 'godot-mvp-release'
$run = $false
$editor = $false
$clean = $false
$jobs = 8
$raceArgument = $null
for ($index = 0; $index -lt $args.Count; $index++) {
    switch ($args[$index]) {
        '--mvp' { }
        '--run' { $run = $true }
        '--editor' { $editor = $true }
        { $_ -match '^--race-track=\d+$' } { $raceArgument = [string]$_; $run = $true }
        '--debug' { $run = $true; $preset = 'godot-mvp-debug' }
        '--clean' { $clean = $true }
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
if ($editor -and $raceArgument) { throw '--editor cannot be combined with --race-track.' }
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
        if ($editor) { & $engine --editor --path $project }
        elseif ($raceArgument) { & $engine --path $project -- $raceArgument }
        else { & $engine --path $project }
        exit $LASTEXITCODE
    }
} finally {
    Pop-Location
}
