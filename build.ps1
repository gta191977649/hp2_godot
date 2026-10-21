# Port-side build entry point. No original-game behavior is implemented here.
# Accept GNU-style options in both Windows PowerShell and PowerShell 7.
$portOptions = @($args)
$portPreset = 'godot-debug'
$portJobs = 4
$portClean = $false
$portRun = $false
$portProfile = $false
$portEditor = $false
$portPrepareWorkers = 0
$portRenderThread = "safe"
$portRenderCandidates = @()
$portRoot = $PSScriptRoot
$ErrorActionPreference = 'Stop'

function Show-PortBuildHelp {
    @'
Usage: ./build.ps1 [--clean] [--run | --profile | --editor] [--preset NAME] [--jobs N]

  --clean         Clean generated build outputs, then rebuild.
  --run           Build successfully, then run the frontend.
  --profile       Run with the global ImGui profiler, function scopes and monitors.
  --editor        Open the project editor for Profiler / Visual Profiler / Monitors.
  --preset NAME   godot-debug (default), godot-release, or native-debug.
  --jobs N        Parallel build jobs (default: 4).
  --render-prepare-workers=0|2|4  CPU preparation concurrency (default: 0).
  --render-thread=safe|separate   Godot render thread mode (default: safe).
  --track-submit-math=true|false CPU submission math candidate (default: false).
  --track-vehicle-depth-staging=true|false  Bulk depth parameters (default: false).
  --track-depth-discovery-cache=true|false  Source/field depth cache (default: false).
  --help, -h      Show this help.

Examples:
  ./build.ps1
  ./build.ps1 --run
  ./build.ps1 --clean --run
  ./build.ps1 --clean --preset native-debug

Run mode requires the pinned Godot 4.7.2 runtime under local/tools/godot-4.7.2.
Original assets are configured through HP2_PS2_ASSET_ROOT or the local config.
'@ | Write-Output
}

try {
    for ($portIndex = 0; $portIndex -lt $portOptions.Count; ++$portIndex) {
        $portOption = [string]$portOptions[$portIndex]
        switch -Regex ($portOption) {
            '^(--help|-h)$' { Show-PortBuildHelp; exit 0 }
            '^--render-prepare-workers=(0|2|4)$' { $portPrepareWorkers = [int]$Matches[1]; continue }
            '^--(?:track-submit-math|track-vehicle-depth-staging|track-depth-discovery-cache)=(?:true|false)$' { $portRenderCandidates += $portOption; continue }
            '^--render-thread=(safe|separate)$' { $portRenderThread = $Matches[1]; continue }
            '^--clean$'  { $portClean = $true; continue }
            '^--run$' { $portRun = $true; continue }
            '^--profile$' { $portRun = $true; $portProfile = $true; continue }
            '^--editor$' { $portRun = $true; $portEditor = $true; continue }
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
    if ($portEditor -and $portRenderThread -eq 'separate') { throw 'Separate render mode must be tested standalone; the editor forces Safe.' }
    if ($portProfile -and $portEditor) { throw 'Choose --profile or --editor.' }
    if ($portProfile -and $portPreset -ne 'godot-debug') { throw '--profile requires godot-debug for native ImGui tooling.' }
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
    $portEngine = Join-Path $portRoot 'local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe'
    if ($portRun -and -not (Test-Path -LiteralPath $portEngine -PathType Leaf)) {
        throw 'Pinned Godot 4.7.2 is missing; see docs/DEVELOPMENT.md.'
    }

    Push-Location -LiteralPath $portRoot
    try {
        Write-Output "Configuring $portPreset"
        if ($portPreset -eq 'native-debug') {
            & cmake --preset $portPreset
        } else {
            # This entry point launches the workspace project, so override any
            # cached output directory left by an isolated validation build.
            $portProjectDir = Join-Path $portRoot 'godot'
            $portBinDir = Join-Path $portProjectDir 'bin'
            & cmake --preset $portPreset "-DPORT_GODOT_PROJECT_DIR=$portProjectDir" "-DPORT_GODOT_BIN_DIR=$portBinDir" "-DPORT_GODOT_EXECUTABLE=$portEngine"
        }
        if ($LASTEXITCODE -ne 0) { throw "CMake configure failed (exit $LASTEXITCODE)." }

        & (Join-Path $portRoot 'tools/track_progress.ps1') -Once

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
            $portEngineArgs = @('--render-thread', $portRenderThread)
            $portUserArgs = @("--render-prepare-workers=$portPrepareWorkers", "--render-thread-requested=$portRenderThread") + $portRenderCandidates
            if ($portEditor) {
                & $portEngine @portEngineArgs --editor --path (Join-Path $portRoot 'godot') -- @portUserArgs
            } elseif ($portProfile) {
                & $portEngine @portEngineArgs --gpu-profile --profiling --path (Join-Path $portRoot 'godot') 'res://scenes/frontend_native.tscn' -- --profile-debug @portUserArgs
            } else {
                & $portEngine @portEngineArgs --path (Join-Path $portRoot 'godot') 'res://scenes/frontend_native.tscn' -- @portUserArgs
            }
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
