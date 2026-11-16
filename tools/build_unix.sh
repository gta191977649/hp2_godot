#!/usr/bin/env bash
set -euo pipefail

target=$1
shift
cd "$(dirname "${BASH_SOURCE[0]}")/.."
root=$PWD
fail() { printf 'HP2: %s\n' "$*" >&2; exit 1; }

action=export
config=Release
verify=false
jobs=8
clean=false
debug=false
driver=
track=
fullscreen=false
fxaa=false
select_iso=false
while (($#)); do
    case $1 in
        --mvp|--stock-template) ;;
        --run) action=run ;;
        --editor) action=editor ;;
        --debug) config=Debug; debug=true ;;
        --verify) verify=true ;;
        --preset)
            shift
            case ${1:-} in
                godot-mvp-release) config=Release ;;
                godot-mvp-debug) config=Debug ;;
                *) fail 'Choose godot-mvp-release or godot-mvp-debug.' ;;
            esac ;;
        --clean) clean=true ;;
        --jobs)
            shift
            [[ ${1:-} =~ ^[1-9][0-9]*$ ]] || fail '--jobs requires a positive integer.'
            jobs=$1 ;;
        --fullscreen) fullscreen=true ;;
        --fxaa) fxaa=true ;;
        --select-iso) select_iso=true ;;
        --rendering-driver)
            shift
            driver=${1:-}
            [[ $driver == vulkan || ( $target == macos && $driver == metal ) ]] || fail 'Choose an available rendering driver.' ;;
        --race-track=*|--race-event=*)
            [[ -z $track ]] || fail 'Choose one race diagnostic.'
            track=$1 ;;
        --help|-h)
            printf 'Usage: bash build_%s.sh [--run | --editor] [--debug] [--preset PRESET] [--jobs N] [--clean] [--select-iso] [--verify]\n' "$target"
            exit 0 ;;
        *) fail "Unknown option: $1" ;;
    esac
    shift
done
if [[ $action == editor ]]; then
    [[ -z $track && $fullscreen == false && $fxaa == false && $select_iso == false ]] ||
        fail 'Game launch options cannot be combined with --editor.'
elif [[ $action == export && ( -n $track || $debug == true || $fullscreen == true || $fxaa == true || $select_iso == true ) ]]; then
    action=run
fi
if $verify && [[ $action != export ]]; then fail '--verify runs the exported package.'; fi

case $target:$(uname -s):$(uname -m) in
    linux:Linux:x86_64) preset='Linux MVP'; arch=(); default_driver=vulkan ;;
    macos:Darwin:arm64) preset='macOS MVP'; arch=(-DCMAKE_OSX_ARCHITECTURES=arm64); default_driver=metal ;;
    *) fail "Run this script on native $target ($([[ $target == linux ]] && echo x86_64 || echo arm64))." ;;
esac
for command in cmake ninja python3; do
    command -v "$command" >/dev/null || fail "Install $command and put it on PATH."
done
command -v "${CXX:-c++}" >/dev/null || fail 'Install a native C++17 compiler or set CXX.'
case $target in
    linux) command -v readelf >/dev/null || fail 'Install binutils (readelf).' ;;
    macos)
        for command in otool install_name_tool codesign; do
            command -v "$command" >/dev/null || fail 'Install the Xcode Command Line Tools.'
        done ;;
esac
engine=${GODOT_BIN:-}
if [[ -z $engine ]]; then
    if [[ $target == macos ]]; then
        engine=$root/local/tools/godot-4.7.2/Godot.app/Contents/MacOS/Godot
    else
        engine=$root/local/tools/godot-4.7.2/Godot_v4.7.2-stable_linux.x86_64
    fi
    if [[ ! -x $engine ]]; then engine=$(command -v godot || command -v godot4 || true); fi
fi
[[ -n $engine && -x $engine ]] || fail 'Set GODOT_BIN to the Godot 4.7.2 editor executable.'
engine=$(cd "$(dirname "$engine")" && printf '%s/%s' "$PWD" "$(basename "$engine")")
version=$("$engine" --version)
[[ $version == 4.7.2.stable* ]] || fail "Expected Godot 4.7.2 stable, found $version."

sdk=${HP2_FFMPEG_SDK:-}
[[ -n $sdk && -f $sdk/include/libavformat/avformat.h && -d $sdk/lib ]] ||
    fail 'Set HP2_FFMPEG_SDK to the shared FFmpeg 8 SDK prefix (include/ and lib/).'
sdk=$(cd "$sdk" && pwd -P)
configuration=release
[[ $config != Debug ]] || configuration=debug
abi=editor
godot_target=editor
if [[ $action == export ]]; then
    abi=export
    godot_target=template_$configuration
    if [[ $target == macos ]]; then
        template="$HOME/Library/Application Support/Godot/export_templates/4.7.2.stable/macos.zip"
    else
        template="${XDG_DATA_HOME:-$HOME/.local/share}/godot/export_templates/4.7.2.stable/linux_$configuration.x86_64"
    fi
    [[ -f $template ]] || fail "Install the official Godot 4.7.2 export templates (missing $template)."
fi
build=$root/build/godot-mvp-$target-$configuration-$abi
project=$build/mvp-project
cmake -S . -B "$build" -G Ninja -DCMAKE_BUILD_TYPE="$config" \
    -DHP2_FFMPEG_SDK="$sdk" -DGODOTCPP_TARGET="$godot_target" \
    ${arch[@]+"${arch[@]}"}
if $clean; then cmake --build "$build" --target clean; fi
cmake --build "$build" --target stage_mvp hp2_unix_launcher --parallel "$jobs"
python3 tools/stage_unix_ffmpeg.py "$target" "$project/bin"

if [[ $action != export ]]; then
    if [[ $target == linux ]]; then
        export LD_LIBRARY_PATH="$project/bin${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    fi
    args=(--path "$project")
    user_args=()
    if [[ $action == editor ]]; then args+=(--editor); fi
    if $fullscreen; then args+=(--fullscreen); fi
    if $debug; then user_args+=(--debug); fi
    if $fxaa; then user_args+=(--fxaa); fi
    if $select_iso; then user_args+=(--select-iso); fi
    if [[ -n $track ]]; then user_args+=("$track"); fi
    if [[ -z $driver ]]; then
        driver=$(python3 - "$project/launcher.cfg" "$target" "$default_driver" <<'PY'
import configparser
import sys

config = configparser.ConfigParser(interpolation=None, strict=False)
try:
    config.read(sys.argv[1], encoding="utf-8")
    key = "rendering_device/driver" + (".macos" if sys.argv[2] == "macos" else "")
    saved = config.get("rendering", key, fallback="").strip('"')
except configparser.Error:
    saved = ""
available = {"vulkan", "metal"} if sys.argv[2] == "macos" else {"vulkan"}
print(saved if saved in available else sys.argv[3])
PY
)
    fi
    args+=(--rendering-driver "$driver")
    if [[ -n ${user_args[0]:-} ]]; then args+=(-- "${user_args[@]}"); fi
    exec "$engine" "${args[@]}"
fi

output=$root/out/mvp/$target-$configuration
mkdir -p "$output"
staged=$(mktemp -d "$build/export.XXXXXXXX")
trap 'rm -rf -- "$staged"' EXIT
mkdir -p "$staged/project" "$staged/package"
for item in project.godot export_presets.cfg native_game.gd graphics_settings.gd native_game.tscn \
    hp2_blackbox.gdextension hp2_blackbox.gdextension.uid shader bin addons; do
    cp -R "$project/$item" "$staged/project/"
done
package=$staged/package
if [[ $target == linux ]]; then
    "$engine" --headless --path "$staged/project" "--export-$configuration" "$preset" "$package/HP2Runtime.x86_64"
    install -m 755 "$build/hp2_unix_launcher" "$package/HP2"
    [[ -f $package/HP2Runtime.pck && -f $package/hp2_blackbox.so ]] || fail 'Linux export is missing the PCK or extension.'
else
    # Official macOS templates contain universal executables. Godot's arm64
    # export expects executables named for that architecture inside the ZIP.
    python3 - "$template" "$staged" <<'PY'
import configparser
from pathlib import Path
import subprocess
import sys
import zipfile

source, staged = map(Path, sys.argv[1:])
template = staged / "macos-arm64.zip"
with zipfile.ZipFile(source) as original, zipfile.ZipFile(template, "w", zipfile.ZIP_DEFLATED) as output:
    names = set(original.namelist())
    for entry in original.infolist():
        data = original.read(entry)
        if entry.filename.endswith(("godot_macos_release.universal", "godot_macos_debug.universal")):
            destination = entry.filename.removesuffix(".universal") + ".arm64"
            if destination in names:
                continue
            fat = staged / "template-universal"
            thin = staged / "template-arm64"
            fat.write_bytes(data)
            subprocess.run(["lipo", str(fat), "-thin", "arm64", "-output", str(thin)], check=True)
            data = thin.read_bytes()
            entry.filename = destination
        output.writestr(entry, data)

presets = staged / "project/export_presets.cfg"
config = configparser.ConfigParser(interpolation=None)
config.read(presets)
section = next(section for section in config.sections() if config.get(section, "name", fallback="") == '"macOS MVP"')
for mode in ("debug", "release"):
    config.set(section + ".options", "custom_template/" + mode, '"' + str(template) + '"')
with presets.open("w") as output:
    config.write(output)
PY
    "$engine" --headless --path "$staged/project" "--export-$configuration" "$preset" "$package/HP2.app"
    bundle=$package/HP2.app/Contents
    entry=$(/usr/libexec/PlistBuddy -c 'Print CFBundleExecutable' "$bundle/Info.plist")
    mv "$bundle/MacOS/$entry" "$bundle/MacOS/HP2Runtime"
    install -m 755 "$build/hp2_unix_launcher" "$bundle/MacOS/$entry"
    for pack in "$bundle"/Resources/*.pck; do
        [[ -f $pack ]] || fail 'macOS export is missing its PCK.'
        if [[ ${pack##*/} != HP2Runtime.pck ]]; then mv "$pack" "$bundle/Resources/HP2Runtime.pck"; fi
    done
    codesign --force --sign - "$bundle/MacOS/HP2Runtime"
    for library in "$bundle"/Frameworks/*.dylib; do codesign --force --sign - "$library"; done
    codesign --force --sign - "$package/HP2.app"
    codesign --verify --deep --strict "$package/HP2.app"
fi
if $verify; then
    if [[ $target == macos ]]; then launcher=$package/HP2.app/Contents/MacOS/$entry; else launcher=$package/HP2; fi
    (cd "$staged"; "$launcher" --headless --quit-after 10 --log-file "$staged/package-check.log" -- --verify-extension)
    grep -q 'HP2_MVP_EXTENSION_READY' "$staged/package-check.log" ||
        fail 'The exported project did not load the native extension.'
    if grep -E 'GDExtension.*(failed|error)|HP2_MVP_ASSERT|SCRIPT ERROR' "$staged/package-check.log"; then
        fail 'The exported project reported a startup error.'
    fi
    rm -rf -- "$package/.hp2-runtime"
    printf 'Verified native extension startup for %s %s\n' "$target" "$configuration"
fi
cp -R "$project/licenses" "$project/THIRD_PARTY_NOTICES.md" "$package/"
if [[ $target == macos && -e $output/HP2.app ]]; then
    # Replace only the generated bundle. The sibling profile directory stays in place.
    rm -rf -- "$output/HP2.app"
fi
cp -R "$package/." "$output/"
printf 'Built %s\n' "$output"
