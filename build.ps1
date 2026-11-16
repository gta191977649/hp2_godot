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
$selectIso = $false
$stockTemplate = $false
$renderingDriver = $null
for ($index = 0; $index -lt $args.Count; $index++) {
    switch ($args[$index]) {
        { $_ -in @('--help', '-h') } {
            Write-Output 'Usage: .\build.ps1 [--run | --editor] [--debug] [--preset PRESET] [--jobs N] [--clean] [--select-iso]'
            exit 0
        }
        '--mvp' { }
        '--run' { $run = $true }
        '--fullscreen' { $fullscreen = $true; $run = $true }
        '--fxaa' { $fxaa = $true; $run = $true }
        '--select-iso' { $selectIso = $true; $run = $true }
        '--stock-template' { $stockTemplate = $true }
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
if ($editor -and ($raceArgument -or $fullscreen -or $fxaa -or $selectIso)) {
    throw '--editor cannot be combined with race diagnostics or game display options.'
}
if ($preset -notin @('godot-mvp-release', 'godot-mvp-debug')) { throw "Unsupported preset $preset." }
if (-not ($run -or $editor) -and $preset -eq 'godot-mvp-release') {
    # The release engine and extension must agree on allocator padding.
    $preset = 'godot-mvp-export'
}

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
    $engine = Join-Path $PSScriptRoot 'local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe'
    if (-not (Test-Path -LiteralPath $engine -PathType Leaf)) { throw 'Godot 4.7.2 console executable is required.' }
    $project = Join-Path $PSScriptRoot "build/$preset/mvp-project"
    if ($run -or $editor) {
        $engineArgs = @('--path', $project)
        if ($renderingDriver) { $engineArgs += @('--rendering-driver', $renderingDriver) }
        if ($editor) { $engineArgs += '--editor' }
        if ($fullscreen) { $engineArgs += '--fullscreen' }
        if ($nativeDebug -or $raceArgument -or $fxaa -or $selectIso) { $engineArgs += '--' }
        if ($nativeDebug) { $engineArgs += '--debug' }
        if ($fxaa) { $engineArgs += '--fxaa' }
        if ($raceArgument) { $engineArgs += $raceArgument }
        if ($selectIso) { $engineArgs += '--select-iso' }
        if ($editor) {
            & $engine @engineArgs
        } else {
            $cache = Join-Path $project '.hp2-runtime'
            $null = New-Item -ItemType Directory -Force $cache
            $previousAppData = $env:APPDATA
            $previousLocalAppData = $env:LOCALAPPDATA
            try {
                $env:APPDATA = $cache
                $env:LOCALAPPDATA = $cache
                & $engine @engineArgs
            } finally {
                $env:APPDATA = $previousAppData
                $env:LOCALAPPDATA = $previousLocalAppData
            }
        }
        exit $LASTEXITCODE
    }
    $configuration = if ($preset -eq 'godot-mvp-debug') { 'debug' } else { 'release' }
    $useSlimTemplate = $configuration -eq 'release' -and -not $stockTemplate
    $package = Join-Path $PSScriptRoot "out/mvp/windows-$configuration"
    $null = New-Item -ItemType Directory -Force -Path $package
    $buildDirectory = (Resolve-Path -LiteralPath "build/$preset").Path
    $exportProject = Join-Path $buildDirectory ('export-' + [guid]::NewGuid().ToString('N'))
    $null = New-Item -ItemType Directory -Path $exportProject
    foreach ($item in @('project.godot', 'export_presets.cfg', 'native_game.gd', 'graphics_settings.gd', 'native_game.tscn', 'hp2_blackbox.gdextension', 'hp2_blackbox.gdextension.uid', 'shader', 'bin', 'addons')) {
        Copy-Item -LiteralPath (Join-Path $project $item) -Destination $exportProject -Recurse -Force
    }
    if ($useSlimTemplate) {
        $godotSource = Join-Path $PSScriptRoot 'local/tools/godot-4.7.2/godot-4.7.2-stable'
        $mesa = Join-Path $PSScriptRoot 'local/tools/godot-4.7.2/build-deps/mesa'
        & (Join-Path $PSScriptRoot 'tools/build_mvp_godot_template.ps1') -Source $godotSource -Mesa $mesa -Jobs $jobs
        if ($LASTEXITCODE) { exit $LASTEXITCODE }
        $binary = Join-Path $godotSource 'bin/godot.windows.template_release.x86_64.exe'
        if (-not (Test-Path -LiteralPath $binary -PathType Leaf)) { throw "Missing slim Godot template at $binary." }
        $exportPresets = Join-Path $exportProject 'export_presets.cfg'
        $presetsText = [IO.File]::ReadAllText($exportPresets)
        $windowsOptions = '[preset.0.options]'
        if (-not $presetsText.Contains($windowsOptions)) { throw 'The Windows export options are missing.' }
        $templateLine = 'custom_template/release="' + $binary.Replace('\', '/') + '"'
        # Godot rejects a leading BOM, which Windows PowerShell 5.1 adds with Set-Content -Encoding utf8.
        [IO.File]::WriteAllText($exportPresets,
            $presetsText.Replace($windowsOptions, "$windowsOptions`n$templateLine"), [Text.UTF8Encoding]::new($false))
    } else {
        $template = Join-Path $env:APPDATA "Godot/export_templates/4.7.2.stable/windows_$configuration`_x86_64.exe"
        if (-not (Test-Path -LiteralPath $template -PathType Leaf)) {
            throw "Install the Godot 4.7.2 Windows x86_64 $configuration export template ($template)."
        }
    }
    cmake --build --preset $preset --target hp2_windows_launcher
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
    & $engine --headless --path $exportProject "--export-$configuration" 'Windows MVP' (Join-Path $package 'HP2Runtime.exe')
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
    Copy-Item -LiteralPath (Join-Path $buildDirectory 'hp2_windows_launcher.exe') -Destination (Join-Path $package 'HP2.exe') -Force
    foreach ($obsolete in @('hp2_track12.dll', 'HP2.pck')) {
        $oldFile = Join-Path $package $obsolete
        if (Test-Path -LiteralPath $oldFile -PathType Leaf) { Remove-Item -LiteralPath $oldFile }
    }
    Copy-Item -LiteralPath (Join-Path $project 'licenses') -Destination $package -Recurse -Force
    Copy-Item -LiteralPath (Join-Path $project 'THIRD_PARTY_NOTICES.md') -Destination $package -Force
} finally {
    if ($exportProject -and (Test-Path -LiteralPath $exportProject -PathType Container)) {
        $staging = (Resolve-Path -LiteralPath $exportProject).Path
        if ($staging -eq $exportProject -and (Split-Path -Parent $staging) -eq $buildDirectory) {
            Remove-Item -LiteralPath $staging -Recurse -Force
        }
    }
    Pop-Location
}
