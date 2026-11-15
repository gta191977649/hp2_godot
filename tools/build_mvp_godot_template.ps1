param(
    [Parameter(Mandatory)][string]$Source,
    [Parameter(Mandatory)][string]$Mesa,
    [ValidateRange(1, 256)][int]$Jobs = 8
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath (Join-Path $Source 'SConstruct') -PathType Leaf)) {
    throw "Godot 4.7.2 source is required at $Source. See docs/DEVELOPMENT.md."
}
$version = Get-Content -LiteralPath (Join-Path $Source 'version.py') -Raw
if ($version -notmatch '(?m)^major = 4\r?$' -or $version -notmatch '(?m)^minor = 7\r?$' -or
    $version -notmatch '(?m)^patch = 2\r?$') {
    throw "The MVP export template requires Godot 4.7.2 source at $Source."
}

Push-Location -LiteralPath $Source
try {
    # FFmpeg and the native game core provide movie decoding and car physics.
    # Keep FileDialog, 3D, both Forward+ backends and GDExtension.
    $options = @(
        'platform=windows', 'target=template_release', 'arch=x86_64', 'use_mingw=yes',
        'optimize=size', 'debug_symbols=no', 'lto=full',
        'vulkan=yes', 'd3d12=yes', 'opengl3=no', 'angle=no', 'winrt=no', 'accesskit=no',
        'disable_physics_2d=yes', 'disable_physics_3d=yes', 'disable_navigation_2d=yes',
        'disable_navigation_3d=yes', 'disable_xr=yes',
        'modules_enabled_by_default=no', 'module_gdscript_enabled=yes', 'module_glslang_enabled=yes',
        'module_freetype_enabled=yes', 'module_text_server_adv_enabled=yes', 'module_svg_enabled=yes',
        "mesa_libs=$Mesa", '-j', $Jobs
    )
    & py -3 -m SCons @options
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
