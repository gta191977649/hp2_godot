"""Measure the seeded Track 13 launch with Godot's Visual Profiler."""

import argparse
from collections import defaultdict
import json
import math
import os
from pathlib import Path
import shutil
import socket
import subprocess

from profile_mvp_reflections import collect


def quantiles(values):
    ordered = sorted(values)
    return [round(ordered[math.ceil(len(ordered) * p) - 1], 3) for p in (.50, .95, .99)]


def report(run, visual, vsync):
    trace = json.loads((run / "frames.json").read_text())
    live = [row for row in trace["rows"] if row[3] == 3]
    intervals = [row[4] for row in live]
    fps = 1000 * len(intervals) / sum(intervals)
    wall = quantiles(intervals)
    missed = sum(ms >= 12.5 for ms in intervals)
    consecutive = sum(a >= 12.5 and b >= 12.5 for a, b in zip(intervals, intervals[1:]))
    passed = (fps >= 119.5 and wall[2] <= 9 and missed / len(intervals) <= 0.001 and not consecutive
              if vsync else wall[1] <= 7.5 and wall[2] <= 8.33)
    print(f"LAUNCH_TIMING fps={fps:.2f} frame_ms={wall} max={max(intervals):.3f} "
          f"missed={missed}/{len(intervals)} consecutive={consecutive} "
          f"gate={'instrumented' if visual else 'pass' if passed else 'miss'}")
    for second in range(5):
        window = [row for row in live if second * 60 <= row[2] - trace["go"] < (second + 1) * 60]
        if window:
            print(f"LAUNCH_SECOND {second} n={len(window)} tyre_active={sum(row[5] > 0 for row in window)} "
                  f"frame_ms={quantiles(row[4] for row in window)}")
    if visual:
        source = {row[0]: row for row in live}
        profile = [data for kind, _, data in map(json.loads, (run / "visual.jsonl").read_text().splitlines())
                   if kind == "visual:profile_frame" and data[0] in source]
        if len(profile) < len(live) * 0.90:
            raise RuntimeError(f"Incomplete Visual Profiler capture, matched {len(profile)} of {len(live)} frames")
        print(f"LAUNCH_VISUAL samples={len(profile)} CPU={quantiles(f[-2] for f in profile)} "
              f"GPU={quantiles(f[-1] for f in profile)}")
        scopes = defaultdict(list)
        for frame in profile:
            if not source[frame[0]][5]:
                continue
            totals = defaultdict(lambda: [0.0, 0.0])
            spans = list(zip(frame[2::3], frame[3::3], frame[4::3]))
            for (name, cpu, gpu), (_, next_cpu, next_gpu) in zip(spans, spans[1:]):
                if name.startswith((">", "<")) or next_cpu < cpu or next_gpu < gpu:
                    continue
                totals[name][0] += next_cpu - cpu
                totals[name][1] += next_gpu - gpu
            for name, pair in totals.items():
                scopes[name].append(pair)
        by_gpu = sorted(scopes.items(), key=lambda item: quantiles(p[1] for p in item[1])[1], reverse=True)
        for index, (name, values) in enumerate(by_gpu):
            if index < 8 or "particle" in name.lower():
                print(f"LAUNCH_SCOPE {name} CPU={quantiles(p[0] for p in values)} "
                      f"GPU={quantiles(p[1] for p in values)}")
    return passed


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--assets", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--stage", type=Path, default=repo / "build/godot-mvp-release/mvp-project")
    parser.add_argument("--engine", type=Path,
                        default=repo / "local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe")
    parser.add_argument("--driver", choices=("vulkan", "d3d12"), default="vulkan")
    parser.add_argument("--resolution", default="native", help="Native maximized or a WxH client size")
    parser.add_argument("--quiet", action="store_true")
    parser.add_argument("--vsync", action="store_true")
    pose = parser.add_mutually_exclusive_group()
    pose.add_argument("--capture-tick", type=int, help="Capture smoke and controls at a tick in the in-progress session")
    pose.add_argument("--hold-tick", type=int, help="Profile a fixed tick in the in-progress session for 240 render frames")
    parser.add_argument("--repeats", type=int, default=1)
    parser.add_argument("variants", choices=("normal", "no-tyre", "no-exhaust", "full-quad"), nargs="+")
    args = parser.parse_args()
    if args.repeats < 1:
        parser.error("--repeats must be positive")
    if args.capture_tick is not None and (not 0 < args.capture_tick < 300 or args.variants != ["normal"]):
        parser.error("--capture-tick needs a tick from 1 to 299 and the normal launch")
    if args.hold_tick is not None and (not 0 < args.hold_tick < 300 or args.quiet):
        parser.error("--hold-tick needs a tick from 1 to 299 and the Visual Profiler")
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    project = out / "project"
    project.mkdir()
    staged = args.stage.resolve()
    for file in ("project.godot", "hp2_blackbox.gdextension", "native_game.gd",
                 "native_game.tscn", "graphics_settings.gd"):
        shutil.copy2(staged / file, project / file)
    for directory in ("bin", "shader", "addons"):
        shutil.copytree(staged / directory, project / directory)
    (project / ".godot").mkdir()
    (project / ".godot/extension_list.cfg").write_text("res://hp2_blackbox.gdextension\n")
    (project / "override.cfg").write_text("[debug]\nsettings/profiler/max_timestamp_query_elements=1024\n")
    (project / "launcher.cfg").write_text(
        f'[graphics]\nwindow_mode=2\naspect=3\nanti_aliasing="fxaa"\n'
        f'fps_limit=0\nvsync={str(args.vsync).lower()}\n')
    visual = not (args.quiet or args.capture_tick)
    results = []
    for pair in range(args.repeats):
        for variant in args.variants if pair % 2 == 0 else reversed(args.variants):
            run = out / f"{pair + 1}-{variant}"
            run.mkdir()
            env = dict(os.environ, HP2_PS2_ASSET_ROOT=str(args.assets.resolve()),
                       HP2_MVP_TEST_PROFILE="1", HP2_PROFILE_DIR=str(run / "profile"),
                       HP2_MVP_PROFILE_GRID="1", HP2_LAUNCH_VARIANT=variant,
                       HP2_LAUNCH_CAPTURE_TICK=str(args.capture_tick or 0),
                       HP2_LAUNCH_HOLD_TICK=str(args.hold_tick or 0), HP2_LAUNCH_OUTPUT=str(run))
            command = [args.engine.resolve(), "--path", project, "--rendering-driver", args.driver,
                       "--script", repo / "tools/test_mvp_launch_smoke.gd"]
            command += ["--maximized"] if args.resolution == "native" else ["--windowed", "--resolution", args.resolution]
            print(f"LAUNCH_RUN {run.name} {args.driver} {args.resolution} vsync={args.vsync} visual={visual}", flush=True)
            with socket.create_server(("127.0.0.1", 0)) as listener, (run / "run.log").open("wb") as log:
                if visual:
                    command += ["--remote-debug", f"tcp://127.0.0.1:{listener.getsockname()[1]}"]
                child = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT, env=env)
                try:
                    if visual:
                        collect(listener, run / "visual.jsonl")
                    code = child.wait(timeout=60)
                finally:
                    if child.poll() is None:
                        child.terminate()
                        child.wait(timeout=5)
            logfile = (run / "run.log").read_text(errors="replace")
            for line in logfile.splitlines():
                if line.startswith(("LAUNCH_", "ERROR", "SCRIPT ERROR")):
                    print(line, flush=True)
            if code or any(line.startswith(("ERROR", "SCRIPT ERROR")) for line in logfile.splitlines()):
                raise RuntimeError(f"Godot exited {code}. See {run / 'run.log'}")
            if not args.capture_tick:
                passed = report(run, visual, args.vsync)
                if not visual and variant == "normal":
                    results.append(passed)
    if results:
        print(f"LAUNCH_GATE passed={sum(results)}/{len(results)}")
        if not all(results):
            raise SystemExit(1)


if __name__ == "__main__":
    main()
