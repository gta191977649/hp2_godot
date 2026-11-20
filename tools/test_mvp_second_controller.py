"""Probe two Windows OS-level Xbox pads against the current single-player host.

Requires vgamepad and an installed ViGEmBus; never installs drivers/packages.
Use --vgamepad-dir for an existing local package. Logs/IPC stay under local/.
"""
import argparse
import gc
import os
from pathlib import Path
import subprocess
import sys
import time


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, default=root / "local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe")
    parser.add_argument("--project", type=Path, default=root / "build/godot-mvp-release/mvp-project")
    parser.add_argument("--vgamepad-dir", type=Path)
    parser.add_argument("--startup", action="store_true", help="Connect both pads before Godot starts")
    parser.add_argument("--reverse", action="store_true", help="Create logical second pad before logical first pad")
    args = parser.parse_args()
    if args.vgamepad_dir:
        sys.path.insert(0, str(args.vgamepad_dir.resolve()))
    import vgamepad as vg

    output = root / "local" / ("issue-5-" + time.strftime("%Y%m%d-%H%M%S") + f"-{os.getpid()}")
    output.mkdir(parents=True)
    ipc = output / "command.txt"
    ipc.write_text("")
    pads = {}

    def connect():
        for name in (["second", "first"] if args.reverse else ["first", "second"]):
            pads[name] = vg.VX360Gamepad()

    if args.startup:
        connect()
    env = dict(os.environ, HP2_MVP_TEST_PROFILE="1", HP2_MVP_TEST_CONTROLS="1",
               HP2_SECOND_CONTROLLER_COMMAND=str(ipc), HP2_SECOND_CONTROLLER_STARTUP=str(int(args.startup)))
    command = [str(args.engine.resolve()), "--path", str(args.project.resolve()),
               "--rendering-driver", "vulkan", "--resolution", "960x720", "--script",
               str(root / "tools/test_mvp_second_controller.gd"), "--", "--race-track=33"]
    log_path = output / "probe.log"
    with log_path.open("wb") as log:
        child = subprocess.Popen(command, env=env, stdout=log, stderr=subprocess.STDOUT)
        try:
            previous = ""
            deadline = time.monotonic() + 150
            while child.poll() is None:
                if time.monotonic() > deadline:
                    raise TimeoutError(f"Controller probe timed out; see {log_path}")
                action = ipc.read_text().strip()
                if action and action != previous:
                    previous = action
                    for pad in pads.values():
                        pad.reset()
                        pad.update()
                    # Do not retain the final loop reference across disconnect.
                    if pads:
                        del pad
                    if action == "connect":
                        connect()
                    elif action == "disconnect_first":
                        # GDScript addresses enumeration order, not creation labels.
                        del pads["second" if args.reverse else "first"]
                        gc.collect()
                    elif action == "disconnect_all":
                        pads.clear()
                        gc.collect()
                    elif action == "reconnect":
                        pads["remaining"] = vg.VX360Gamepad()
                    elif action != "neutral":
                        ordered = list(pads)
                        name = ordered[0] if action == "first_throttle" else ordered[-1]
                        if action == "second_axis":
                            pads[name].left_joystick(x_value=32767, y_value=0)
                        else:
                            button = vg.XUSB_BUTTON.XUSB_GAMEPAD_X if action == "second_brake" else vg.XUSB_BUTTON.XUSB_GAMEPAD_A
                            pads[name].press_button(button)
                        pads[name].update()
                time.sleep(0.02)
        finally:
            if child.poll() is None:
                child.terminate()
                child.wait(timeout=10)
            pads.clear()
            gc.collect()
    print(log_path.read_text(encoding="utf-8", errors="replace"))
    print(f"Evidence: {log_path}")
    return child.returncode


if __name__ == "__main__":
    raise SystemExit(main())
