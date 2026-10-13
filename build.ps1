# Port-side build entry point. No original-game behavior is implemented here.
# Accept GNU-style options in both Windows PowerShell and PowerShell 7.
$portOptions = @($args)
$portPreset = 'godot-debug'
$portJobs = 4
$portClean = $false
$portRun = $false
$portRoot = $PSScriptRoot
$ErrorActionPreference = 'Stop'

function Show-PortBuildHelp {
    @'
Usage: ./build.ps1 [--clean] [--run] [--preset NAME] [--jobs N]

  --clean         Clean generated build outputs, then rebuild.
  --run           Build successfully, then run the frontend.
  --preset NAME   godot-debug (default), godot-release, or native-debug.
  --jobs N        Parallel build jobs (default: 4).
  --help, -h      Show this help.

Examples:
  ./build.ps1
  ./build.ps1 --run
  ./build.ps1 --clean --run
  ./build.ps1 --clean --preset native-debug

Run mode requires the pinned Godot 4.4.1 runtime under local/tools/godot-4.4.1.
Original assets are configured through HP2_PS2_ASSET_ROOT or the local config.
'@ | Write-Output
}

try {
    for ($portIndex = 0; $portIndex -lt $portOptions.Count; ++$portIndex) {
        $portOption = [string]$portOptions[$portIndex]
        switch -Regex ($portOption) {
            '^(--help|-h)$' { Show-PortBuildHelp; exit 0 }
            '^--clean$' { $portClean = $true; continue }
            '^--run$' { $portRun = $true; continue }
            '^--preset=(.+)$' { $portPreset = $Matches[1]; continue }
            '^--preset$' {
                if (++$portIndex -ge $portOptions.Count) { throw '--preset requires a value.' }
                $portPreset = [string]$portOptions[$portIndex]
                continue
            }
            '^--jobs=(.+)$' {
                $portJobsText = $Matches[1]
                if (-not [int]::TryParse($portJobsText, [ref]$portJobs) -or $portJobs -lt 1) {
                    throw '--jobs requires a positive integer.'
                }
                continue
            }
            '^(--jobs|-j)$' {
                if (++$portIndex -ge $portOptions.Count) { throw '--jobs requires a value.' }
                $portJobsText = [string]$portOptions[$portIndex]
                if (-not [int]::TryParse($portJobsText, [ref]$portJobs) -or $portJobs -lt 1) {
                    throw '--jobs requires a positive integer.'
                }
                continue
            }
            default { throw "Unknown option: $portOption. Use --help for usage." }
        }
    }

    if ($portPreset -notin @('godot-debug', 'godot-release', 'native-debug')) {
        throw "Unsupported preset: $portPreset. Use --help for supported presets."
    }
    if ($portRun -and $portPreset -eq 'native-debug') {
        throw '--run needs a Godot preset; native-debug builds the independent core only.'
    }
    Get-Command cmake -ErrorAction Stop | Out-Null

    $portBuildRoot = [IO.Path]::GetFullPath((Join-Path $portRoot 'build'))
    $portBuildDir = [IO.Path]::GetFullPath((Join-Path $portBuildRoot $portPreset))
    if (-not $portBuildDir.StartsWith($portBuildRoot + [IO.Path]::DirectorySeparatorChar,
                                    [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Resolved build directory is outside the project build directory.'
    }
    $portEngine = Join-Path $portRoot 'local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe'
    if ($portRun -and -not (Test-Path -LiteralPath $portEngine -PathType Leaf)) {
        throw 'Pinned Godot 4.4.1 is missing; see docs/DEVELOPMENT.md.'
    }

    Push-Location -LiteralPath $portRoot
    try {
        Write-Output "Configuring $portPreset"
        & cmake --preset $portPreset
        if ($LASTEXITCODE -ne 0) { throw "CMake configure failed (exit $LASTEXITCODE)." }

        if ($portClean) {
            # Use the generator's manifest, preserving arbitrary user files in build/.
            Write-Output "Cleaning generated outputs in $portBuildDir"
            & cmake --build --preset $portPreset --target clean
            if ($LASTEXITCODE -ne 0) { throw "CMake clean failed (exit $LASTEXITCODE)." }
        }

        Write-Output "Building $portPreset with $portJobs jobs"
        & cmake --build --preset $portPreset --parallel $portJobs
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed (exit $LASTEXITCODE). If ps2_port.dll is in use, close the running frontend and retry."
        }
        Write-Output 'Build completed.'

        if ($portRun) {
            Write-Output 'Starting the frontend.'
            & $portEngine --path (Join-Path $portRoot 'godot') 'res://scenes/frontend_native.tscn'
            if ($LASTEXITCODE -ne 0) { throw "Frontend exited with code $LASTEXITCODE." }
        }
    } finally {
        Pop-Location
    }
    exit 0
} catch {
    Write-Error "build.ps1: $($_.Exception.Message)" -ErrorAction Continue
    exit 1
}
