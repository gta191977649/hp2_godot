"""Stage an FFmpeg 8 SDK whose non-system dependencies fit the HP2 package."""

from pathlib import Path
import re
import subprocess
import sys

ABIS = {"avformat": 62, "avcodec": 62, "avutil": 60, "swresample": 6}


def run(*args: str) -> str:
    return subprocess.check_output(args, text=True)


def stage_macos(folder: Path) -> None:
    names = {f"lib{name}.{abi}.dylib" for name, abi in ABIS.items()}
    for name in sorted(names | {"hp2_blackbox.dylib"}):
        file = folder / name
        dependencies = run("otool", "-L", str(file)).splitlines()[1:]
        for line in dependencies:
            dependency = line.strip().split(" (compatibility version", 1)[0]
            basename = Path(dependency).name
            if basename == name:
                subprocess.run(["install_name_tool", "-id", f"@rpath/{name}", file], check=True)
            elif basename in names:
                replacement = f"@loader_path/{basename}"
                if dependency != replacement:
                    subprocess.run(["install_name_tool", "-change", dependency, replacement, file], check=True)
            elif not dependency.startswith(("/usr/lib/", "/System/Library/")):
                raise SystemExit(f"{name} requires {dependency}. Use an FFmpeg SDK built with --disable-autodetect.")
        subprocess.run(["codesign", "--force", "--sign", "-", file], check=True)


def check_linux(folder: Path) -> None:
    names = {f"lib{name}.so.{abi}" for name, abi in ABIS.items()}
    system = {
        "ld-linux-x86-64.so.2", "libm.so.6", "libc.so.6", "libpthread.so.0",
        "librt.so.1", "libdl.so.2", "libstdc++.so.6", "libgcc_s.so.1", "libatomic.so.1",
    }
    for name in sorted(names | {"hp2_blackbox.so"}):
        tags = run("readelf", "-d", str(folder / name))
        needed = set(re.findall(r"\(NEEDED\).*\[(.*)\]", tags))
        missing = sorted(needed - names - system)
        if missing:
            raise SystemExit(f"{name} requires {', '.join(missing)}. Use an FFmpeg SDK built with --disable-autodetect.")


if __name__ == "__main__":
    target, path = sys.argv[1:]
    (stage_macos if target == "macos" else check_linux)(Path(path))
