"""Run isolated Quick Race physics traces at multiple display schedules."""

import argparse
import csv
import math
import os
from pathlib import Path
import re
import subprocess


def read_ticks(path, speed, distance, maneuver):
    with path.open(newline="", encoding="ascii") as stream:
        rows = list(csv.DictReader(stream))
    if not rows:
        raise ValueError(f"{path}: no committed ticks")
    for index, row in enumerate(rows):
        if int(row["case_tick"]) != index or int(row["step"]) != int(rows[0]["step"]) + index:
            raise ValueError(f"{path}: missing committed World tick at {index}")
        expected_dt = float(row["time_scale"]) / 60
        if int(row["quanta"]) != 60 or not math.isclose(float(row["world_dt"]), expected_dt, rel_tol=1e-6):
            raise ValueError(f"{path}: unexpected live timestep at {index}")
        if not all(math.isfinite(float(value)) for value in row.values()):
            raise ValueError(f"{path}: non-finite state at tick {index}")
    trigger = next((i for i, row in enumerate(rows) if row["pulse_tick"] == "0"), None)
    if trigger is None or len(rows) - trigger != 120:
        peak = max(float(row["speed"]) for row in rows)
        raise ValueError(f"{path}: incomplete maneuver at {speed:g} m/s and route {distance:g} m (peak {peak:.2f} m/s)")
    window = rows[trigger:]
    stick, length = {"left": (96, 3), "right": (160, 3), "key-left": (0, 1), "graze": (168, 18)}[maneuver]
    if [int(row["stick"]) for row in window] != [stick] * length + [128] * (120 - length):
        raise ValueError(f"{path}: unexpected controller pulse")
    if not any(abs(float(row["steering"])) > 0 for row in window[:length]):
        raise ValueError(f"{path}: controller pulse did not reach the source driver")
    if maneuver == "graze" and not any(int(row["impact_count"]) for row in window):
        raise ValueError(f"{path}: no recorded car impact during graze")
    return rows, window


def compare(reference, rows, label):
    for index, (expected, actual) in enumerate(zip(reference, rows)):
        for field in expected:
            if field != "generation" and expected[field] != actual[field]:
                raise ValueError(
                    f"{label}: first divergence at tick {index}, {field} "
                    f"expected {expected[field]}, got {actual[field]}"
                )
    if len(reference) != len(rows):
        raise ValueError(f"{label}: expected {len(reference)} ticks, got {len(rows)}")


def read_frame_gap(path, schedule):
    with path.open(newline="", encoding="ascii") as stream:
        frames = list(csv.DictReader(stream))
    if len(frames) < 2 or not all(math.isfinite(float(value)) for frame in frames for value in frame.values()):
        raise ValueError(f"{path}: missing or non-finite presentation samples")
    largest_gap = max(
        int(current["render_us"]) - int(previous["render_us"])
        for previous, current in zip(frames, frames[1:])
    )
    if schedule == "stall" and largest_gap < 180_000:
        raise ValueError(f"{path}: injected render stall was not observed")
    return largest_gap / 1000


def run_case(args, car):
    reference = None
    for schedule in args.schedules:
        prefix = args.output / f"{car}-{args.maneuver}-{args.speed:g}-{args.distance:g}-{schedule}"
        environment = {
            **os.environ,
            "HP2_MVP_TEST_PROFILE": "1",
            "HP2_MVP_DEBUG_MODE": "0",
            "HP2_MVP_PHYSICS_TRACE": str(prefix),
            "HP2_MVP_PHYSICS_CAR": car,
            "HP2_MVP_PHYSICS_SPEED": str(args.speed),
            "HP2_MVP_PHYSICS_DISTANCE": str(args.distance),
            "HP2_MVP_PHYSICS_MANEUVER": args.maneuver,
            "HP2_MVP_PHYSICS_SCHEDULE": schedule,
        }
        cap = int(schedule) if schedule.isdecimal() else 60
        script = Path(__file__).with_suffix(".gd").resolve()
        command = [str(args.godot), "--path", str(args.project), "--disable-vsync", "--max-fps", str(cap)]
        if args.headless:
            command.append("--headless")
        else:
            command.extend(["--resolution", "640x480", "--rendering-driver", args.driver])
        command.extend(["--script", str(script), "--", f"--race-track={args.track}"])
        log_path = Path(str(prefix) + ".log")
        with log_path.open("w", encoding="utf-8") as log:
            subprocess.run(command, env=environment, stdout=log, stderr=subprocess.STDOUT, timeout=100, check=True)
        log = log_path.read_text(encoding="utf-8", errors="replace")
        complete = re.search(r"HP2_MVP_PHYSICS_COMPLETE frames=(\d+) duration_us=(\d+) stall=(\w+)", log)
        if not complete or (schedule == "stall" and complete[3] != "true"):
            raise ValueError(f"{log_path}: requested schedule did not complete")
        rows, window = read_ticks(Path(str(prefix) + ".ticks.csv"), args.speed, args.distance, args.maneuver)
        largest_gap = read_frame_gap(Path(str(prefix) + ".frames.csv"), schedule)
        if reference is None:
            reference = rows
        else:
            compare(reference, rows, prefix.name)
        fps = int(complete[1]) * 1_000_000 / max(1, int(complete[2]))
        peak_yaw = max(abs(float(row["yaw_rate"])) for row in window)
        print(f"{prefix.name}  commits={len(rows)} fps={fps:.1f} "
              f"entry={float(window[0]['speed']):.2f}m/s peak_yaw={peak_yaw:.4f}rad/s "
              f"max_frame_gap={largest_gap:.1f}ms", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--godot", type=Path, required=True)
    parser.add_argument("--project", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--track", type=int, default=13)
    parser.add_argument("--cars", nargs="+", choices=["BMWM5", "MURCIELAGO", "ELISE"],
                        default=["BMWM5", "MURCIELAGO", "ELISE"])
    parser.add_argument("--schedules", nargs="+", choices=["30", "60", "120", "144", "jitter", "stall"],
                        default=["30", "60", "120", "144", "jitter", "stall"])
    parser.add_argument("--maneuver", choices=["left", "right", "key-left", "graze"], default="left")
    parser.add_argument("--speed", type=float, default=30, help="minimum pre-pulse forward speed in m/s")
    parser.add_argument("--distance", type=float, default=0, help="minimum authored route distance in metres")
    parser.add_argument("--driver", default="vulkan")
    parser.add_argument("--headless", action="store_true")
    args = parser.parse_args()
    if not all(math.isfinite(value) and value >= 0 for value in (args.speed, args.distance)):
        parser.error("speed and distance must be finite and nonnegative")
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    for car in args.cars:
        run_case(args, car)


if __name__ == "__main__":
    main()
