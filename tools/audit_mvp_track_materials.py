"""Load every retail route with the MVP catalogue and write resolved material inventories."""

import argparse
import csv
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import os
from pathlib import Path
import re
import subprocess


ROOT = Path(__file__).resolve().parents[1]
TRACKS = [10 * family + route for family in (1, 2, 3, 4, 6) for route in range(1, 7)]
FIELDS = "model solid lod texture flags blend bias sort depth test gloss mip replaced strips".split()
ROW = re.compile("TRACK_MATERIAL " + " ".join(rf"{key}=(.*?)" for key in FIELDS) + r"$")
SHAFTS = {"885adba5", "ba0e31b3", "ba0e31b4"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--assets", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--project", type=Path, default=ROOT / "build/godot-mvp-release/mvp-project")
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    engine = ROOT / "local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe"

    def run(track):
        env = dict(os.environ, HP2_PS2_ASSET_ROOT=str(args.assets.resolve()),
                   HP2_PROFILE_DIR=str(out / f"profile-{track}"), HP2_MVP_TRACK_MATERIAL_AUDIT="1")
        result = subprocess.run(
            [engine, "--headless", "--path", args.project.resolve(), "--script",
             ROOT / "tools/audit_mvp_track_materials.gd", "--", f"--race-track={track}"],
            env=env, capture_output=True, text=True, errors="replace", timeout=90)
        (out / f"{track}.log").write_text(result.stdout + result.stderr, encoding="utf-8")
        rows = [m.groups() for line in result.stderr.splitlines() if (m := ROW.fullmatch(line))]
        if result.returncode or not rows or f"TRACK_AUDIT_READY {track}" not in result.stdout:
            raise RuntimeError(f"Track {track} failed; see {out / f'{track}.log'}")
        return track, rows

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        with (out / "materials.csv").open("w", encoding="utf-8", newline="") as detail, (
            out / "coverage.csv").open("w", encoding="utf-8", newline="") as summary:
            writer = csv.writer(detail)
            writer.writerow(["track", *FIELDS])
            coverage = csv.writer(summary)
            coverage.writerow(["track", "strips", "shafts", "road40", "blend44", "blend48", "bias1", "bias2",
                               "replacement", "unresolved"])
            for track, rows in pool.map(run, TRACKS):
                counts = Counter()
                for row in rows:
                    writer.writerow([track, *row])
                    data = dict(zip(FIELDS, row))
                    n = int(data["strips"])
                    counts["strips"] += n
                    counts["shafts"] += n * (data["texture"] in SHAFTS)
                    counts["road40"] += n * bool(int(data["flags"], 16) & 0x40)
                    counts["blend44"] += n * (data["blend"] == "68")
                    counts["blend48"] += n * (data["blend"] == "72")
                    counts["bias1"] += n * (data["bias"] == "1")
                    counts["bias2"] += n * (data["bias"] == "2")
                    counts["replacement"] += n * (data["replaced"] == "1")
                    counts["unresolved"] += n * (data["blend"] == "-1")
                coverage.writerow([track, *(counts[k] for k in ("strips", "shafts", "road40",
                                  "blend44", "blend48", "bias1", "bias2", "replacement", "unresolved"))])
                print(f"Track {track}: {dict(counts)}", flush=True)


if __name__ == "__main__":
    main()
