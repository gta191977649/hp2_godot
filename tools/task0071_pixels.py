"""Port-side cross-backend evidence report; image differences never imply PS2 verification."""
import argparse
import json
from pathlib import Path

import numpy as np
from PIL import Image


def compare(baseline, candidate):
    result = {"native_equal": True, "pixel_exact": True, "poses": {},
              "policy": "No automatic cross-backend visual-equivalence claim; inspect every reported difference."}
    for pose in ("pose", "nearby"):
        first = json.loads((baseline / (pose + ".json")).read_text())
        second = json.loads((candidate / (pose + ".json")).read_text())
        keys = ("packet_digest", "commands", "sections", "zone", "visible_instances")
        native = {key: bool(first["summary"].get(key) == second["summary"].get(key)
                            and key in first["summary"] and key in second["summary"])
                  for key in keys}
        result["native_equal"] &= all(native.values())
        row = {"native_fields": native, "images": {}}
        for kind in ("final", "colour", "native_depth"):
            a = np.array(Image.open(baseline / f"{pose}-{kind}.png").convert("RGB"), dtype=np.int32)
            b = np.array(Image.open(candidate / f"{pose}-{kind}.png").convert("RGB"), dtype=np.int32)
            if a.shape != b.shape:
                row["images"][kind] = {"error": "Image dimensions differ"}
                result["pixel_exact"] = False
                continue
            delta = np.abs(a - b)
            report = {"size": [a.shape[1], a.shape[0]], "maximum_channel_delta": int(delta.max()),
                      "mean_channel_delta": float(delta.mean()),
                      "different_pixel_fraction": float(np.any(delta != 0, axis=2).mean()),
                      "above_one_byte_pixel_fraction": float(np.any(delta > 1, axis=2).mean())}
            if kind == "native_depth":
                za = a[:, :, 0] + (a[:, :, 1] << 8) + (a[:, :, 2] << 16)
                zb = b[:, :, 0] + (b[:, :, 1] << 8) + (b[:, :, 2] << 16)
                zd = np.abs(za-zb)
                report.update(maximum_z_delta=int(zd.max()), median_z_delta=float(np.median(zd)),
                              above_one_z_unit_fraction=float((zd > 1).mean()))
            result["pixel_exact"] &= not np.any(delta)
            row["images"][kind] = report
        result["poses"][pose] = row
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("candidate", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args()
    report = compare(arguments.baseline, arguments.candidate)
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    arguments.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    raise SystemExit(0 if report["native_equal"] else 1)
