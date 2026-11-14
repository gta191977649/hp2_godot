"""Run the Scenic Drive reflection comparison via Godot's built-in Visual Profiler."""

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import socket
import struct
import subprocess


def encode(value):
    if isinstance(value, bool):
        return struct.pack("<II", 1, value)
    if isinstance(value, int):
        return struct.pack("<II", 2, value)
    if isinstance(value, str):
        data = value.encode()
        return struct.pack("<II", 4, len(data)) + data + bytes(-len(data) % 4)
    return struct.pack("<II", 28, len(value)) + b"".join(map(encode, value))


def decode(data, offset=0):
    tag, = struct.unpack_from("<I", data, offset)
    offset += 4
    kind = tag & 255
    if kind in (1, 2, 3):
        wide = tag & 65536
        layout = struct.Struct(("<d" if wide else "<f") if kind == 3 else ("<q" if wide else "<i"))
        return layout.unpack_from(data, offset)[0], offset + layout.size
    if kind in (4, 21):
        size, = struct.unpack_from("<I", data, offset)
        offset += 4
        return data[offset:offset + size].decode(), offset + (size + 3) // 4 * 4
    if kind == 28:
        size, = struct.unpack_from("<I", data, offset)
        offset += 4
        result = []
        for _ in range(size & 0x7fffffff):
            value, offset = decode(data, offset)
            result.append(value)
        return result, offset
    raise ValueError(f"Unexpected Godot Variant type {kind}")


def collect(listener, path):
    listener.settimeout(20)
    client, _ = listener.accept()
    with client, client.makefile("rb") as stream, path.open("w", encoding="utf-8") as output:
        client.settimeout(180)
        packet = encode(["profiler:visual", 1, [True]])
        client.sendall(struct.pack("<I", len(packet)) + packet)
        while header := stream.read(4):
            length, = struct.unpack("<I", header)
            data = stream.read(length)
            try:
                message, _ = decode(data)
            except ValueError:
                continue
            if message[0] in ("visual:profile_frame", "visual:hardware_info"):
                output.write(json.dumps(message) + "\n")


def percentiles(values):
    values = sorted(values)
    return [round(values[int((len(values) - 1) * p)], 3) for p in (.5, .95, .99)]


def report(run, visual, grid=False):
    log = (run / "run.log").read_text(errors="replace")
    for line in log.splitlines():
        if line.startswith(("REFLECTION_", "GRID_", "ERROR", "SCRIPT ERROR")):
            print(line, flush=True)
    fog_trace = run / "fog.json"
    rows = []
    if fog_trace.exists():
        rows = json.loads(fog_trace.read_text())
        for tick in range(0, 720, 60):
            window = [row for row in rows if tick <= row[1] < tick + 60]
            if not window:
                continue
            fog = window[len(window) // 2][3]
            intervals = [row[2] for row in window]
            print(f"FOG ticks={tick}:{tick + 60} {fog} frame={percentiles(intervals)} "
                  f"over={sum(ms > 1000 / 120 for ms in intervals)}/{len(intervals)}", flush=True)
    if not visual:
        return
    start = int(re.search(r"REFLECTION_BEGIN (\d+)", log)[1])
    end = int(re.search(r"REFLECTION_END (\d+)", log)[1])
    frames = []
    with (run / "visual.jsonl").open(encoding="utf-8") as data:
        for line in data:
            kind, _, frame = json.loads(line)
            if kind == "visual:profile_frame" and start + 6 <= frame[0] < end - 6:
                frames.append(frame)
    if len(frames) < 330:
        raise RuntimeError(f"Expected matched Visual Profiler frames, found {len(frames)}")
    print(f"VISUAL {len(frames)} frames CPU P50/P95/P99={percentiles(f[-2] for f in frames)} "
          f"GPU P50/P95/P99={percentiles(f[-1] for f in frames)}", flush=True)
    if grid:
        names = ["Render Depth Pre-Pass", "Render Opaque Pass"]
        if rows:
            names += ["Render 3D Transparent Pass", "Render CanvasItems", "Resolve MSAA"]
        scopes = {name: [] for name in names}
        for frame in frames:
            spans = list(zip(frame[2::3], frame[3::3], frame[4::3]))
            # The main race viewport submits the most geometry. Reflection and
            # mirror viewports also emit these stage names in the same frame.
            for name, times in scopes.items():
                durations = [(spans[i + 1][1] - cpu, spans[i + 1][2] - gpu)
                             for i, (stage, cpu, gpu) in enumerate(spans[:-1]) if stage == name]
                if durations:
                    times.append(max(durations, key=lambda value: value[0]))
        for name, times in scopes.items():
            if times:
                print(f"VISUAL {name} longest CPU scope per frame "
                      f"CPU={percentiles(cpu for cpu, _ in times)} GPU={percentiles(gpu for _, gpu in times)}", flush=True)
    if rows:
        ticks = {row[0]: row[1] for row in rows}
        for tick in range(0, 720, 60):
            window = [frame for frame in frames if tick <= ticks.get(frame[0], -1) < tick + 60]
            if window:
                print(f"VISUAL ticks={tick}:{tick + 60} n={len(window)} "
                      f"CPU={percentiles(f[-2] for f in window)} GPU={percentiles(f[-1] for f in window)}", flush=True)


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--assets", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--pairs", type=int, default=3)
    parser.add_argument("--driver", default="vulkan", choices=("vulkan", "d3d12"))
    parser.add_argument("--resolution", default="1920x1080")
    parser.add_argument("--fullscreen", action="store_true")
    pacing = parser.add_mutually_exclusive_group()
    pacing.add_argument("--vsync", action="store_true")
    pacing.add_argument("--max-fps", type=int, help="Apply Godot's frame rate limiter with VSync off")
    parser.add_argument("--quiet", action="store_true", help="Run without Visual Profiler")
    parser.add_argument("--section-tree", action="store_true", help="Compare section tree culling with the existing zone and LOD path")
    parser.add_argument("--fog", choices=("ab", "baseline", "optimized", "no-fog", "no-focus", "no-history"),
                        help="Compare the fog paths or isolate a stage during the Starting Grid flyby")
    parser.add_argument("--grid-effect", choices=("no-shafts", "no-sun", "no-particles", "legacy-sun-bounds", "compare-sun-bounds"),
                        help="Isolate an effect during the Starting Grid flyby")
    parser.add_argument("--capture-tick", type=int, help="Capture a flyby image in a separate run")
    parser.add_argument("--stage", type=Path, help="Use an alternate staged Godot build")
    parser.add_argument("--fxaa", action="store_true", help="Test FXAA in the race viewport")
    scene = parser.add_mutually_exclusive_group()
    scene.add_argument("--demo", action="store_true", help="Watch the demo through its natural end")
    scene.add_argument("--grid", action="store_true", help="Measure the track flyby before confirming Starting Grid")
    args = parser.parse_args()
    if args.section_tree and not args.grid:
        parser.error("--section-tree requires --grid")
    if args.fog and (not args.grid or args.section_tree):
        parser.error("--fog requires --grid and its default scenery path")
    if (args.grid_effect or args.capture_tick is not None) and not args.grid:
        parser.error("--grid-effect and --capture-tick require --grid")
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    project = out / "project"
    staged = args.stage.resolve() if args.stage else repo / "build/godot-mvp-release/mvp-project"
    project.mkdir()
    for file in ("project.godot", "hp2_track12.gdextension", "native_game.gd", "native_game.tscn"):
        shutil.copy2(staged / file, project / file)
    for directory in ("bin", "shader"):
        shutil.copytree(staged / directory, project / directory)
    (project / ".godot").mkdir()
    (project / ".godot/extension_list.cfg").write_text("res://hp2_track12.gdextension\n")
    (project / "override.cfg").write_text(
        "[debug]\nsettings/profiler/max_timestamp_query_elements=1024\n"
        f"\n[display]\nwindow/size/mode={3 if args.fullscreen else 0}\n"
        f"window/vsync/vsync_mode={int(args.vsync)}\n"
        f"\n[hp2]\nrace_fxaa={str(args.fxaa).lower()}\n"
    )
    engine = repo / "local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64.exe"
    script = repo / "tools" / ("test_mvp_starting_grid.gd" if args.grid else "test_mvp_reflection_performance.gd")
    variants = ["isolated"] if args.demo or args.pairs == 0 else [
        variant for pair in range(args.pairs)
        for variant in (("shared", "isolated") if pair % 2 == 0 else ("isolated", "shared"))
    ]
    if args.grid:
        variants = [
            ("baseline" if variant == "shared" else "tree") if args.section_tree
            else ("legacy" if variant == "shared" else "lod")
            for variant in variants
        ]
    if args.fog:
        variants = [args.fog] if args.fog != "ab" else [
            "baseline" if variant == "legacy" else "optimized" for variant in variants
        ]
    for i, variant in enumerate(variants, 1):
        run = out / f"{i}-{variant}"
        profile = run / "profile"
        profile.mkdir(parents=True)
        (profile / "config.ini").write_text("format=1\ntrack_selection=13\n")
        env = dict(os.environ, HP2_PS2_ASSET_ROOT=str(args.assets.resolve()), HP2_PROFILE_DIR=str(profile),
                   HP2_MVP_PROFILE_REFLECTIONS="1", HP2_MVP_PROFILE_SHARED_REFLECTION_WORLD=str(int(variant == "shared")),
                   HP2_MVP_REFLECTION_CHECK="demo" if args.demo else "fixed")
        if args.grid:
            env.update(HP2_MVP_PROFILE_GRID="1", HP2_MVP_PROFILE_REFLECTIONS="0",
                       HP2_MVP_PROFILE_LEGACY_SCENERY=str(int(variant == "legacy")),
                       HP2_MVP_PROFILE_BASELINE_SCENERY_LOD=str(int(variant in ("baseline", "lod"))))
        if args.fog:
            env.update(HP2_MVP_PROFILE_BASELINE_SCENERY_LOD="0", HP2_MVP_PROFILE_FOG=variant,
                       HP2_FOG_TRACE=str(run / "fog.json"))
        if args.grid_effect:
            env["HP2_MVP_PROFILE_GRID_EFFECT"] = args.grid_effect
        if args.capture_tick is not None:
            env.update(HP2_GRID_CAPTURE_DIR=str(run), HP2_GRID_CAPTURE_TICK=str(args.capture_tick))
        visual = not (args.quiet or args.demo)
        command = [engine, "--path", project, "--rendering-driver", args.driver,
                   "--resolution", args.resolution, "--script", script]
        command.append("--fullscreen" if args.fullscreen else "--windowed")
        if not args.vsync:
            command.append("--disable-vsync")
        if args.max_fps:
            command += ["--max-fps", str(args.max_fps)]
        print(f"{i} {variant} {args.driver} {args.resolution} vsync={args.vsync} visual={visual}", flush=True)
        with socket.create_server(("127.0.0.1", 0)) as listener, (run / "run.log").open("wb") as log:
            if visual:
                command += ["--remote-debug", f"tcp://127.0.0.1:{listener.getsockname()[1]}"]
            child = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT, env=env)
            try:
                if visual:
                    collect(listener, run / "visual.jsonl")
                if child.wait(timeout=225) != 0:
                    raise RuntimeError(f"Godot exited {child.returncode}. See {run / 'run.log'}")
            finally:
                if child.poll() is None:
                    child.terminate()
                    child.wait(timeout=5)
        report(run, visual, args.grid)


if __name__ == "__main__":
    main()
