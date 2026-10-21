"""TASK-0038: real Win32 input acceptance for an owned Godot test window.

Uses the explicit --driving-os-smoke harness; no input is sent to unrelated windows.
Runtime reports and optional screenshots stay under ignored local/ by default.
"""
from __future__ import annotations
import argparse
import ctypes as c
from ctypes import wintypes as w
import json
import math
from pathlib import Path
import subprocess
import sys
import time


def main() -> int:
    if sys.platform != "win32":
        raise RuntimeError("Owned-window OS test requires Windows")
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=root / "local/task0038/third-person-os")
    parser.add_argument("--surfaces", action="store_true", help="Also exercise five material choices and world-zone grip diagnostics through OS input")
    parser.add_argument("--controls", action="store_true", help="Exercise manual/automatic GUI, Q/E/N, reverse, handbrake and original skid rendering")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    state_path = output / f"state-{time.time_ns()}.json"
    engine = root / "local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64.exe"
    if not engine.is_file():
        raise RuntimeError(f"Pinned Godot executable missing: {engine}")
    user = c.WinDLL("user32", use_last_error=True)
    user.SetProcessDPIAware()
    user.GetForegroundWindow.restype = w.HWND
    user.SetForegroundWindow.argtypes = [w.HWND]
    user.BringWindowToTop.argtypes = [w.HWND]
    user.SetWindowPos.argtypes = [w.HWND, w.HWND, c.c_int, c.c_int, c.c_int, c.c_int, w.UINT]
    user.AttachThreadInput.argtypes = [w.DWORD, w.DWORD, w.BOOL]
    kernel = c.WinDLL("kernel32", use_last_error=True)
    kernel.GetCurrentThreadId.restype = w.DWORD
    user.GetWindowThreadProcessId.argtypes = [w.HWND, c.POINTER(w.DWORD)]
    user.GetClientRect.argtypes = [w.HWND, c.POINTER(w.RECT)]
    user.SetCursorPos.argtypes = [c.c_int, c.c_int]
    user.WindowFromPoint.argtypes = [w.POINT]
    user.WindowFromPoint.restype = w.HWND
    user.ClientToScreen.argtypes = [w.HWND, c.POINTER(w.POINT)]
    user.GetWindowRect.argtypes = [w.HWND, c.POINTER(w.RECT)]
    user.PostMessageW.argtypes = [w.HWND, w.UINT, w.WPARAM, w.LPARAM]
    user.ShowWindow.argtypes = [w.HWND, c.c_int]
    user.DestroyWindow.argtypes = [w.HWND]
    user.CreateWindowExW.restype = w.HWND
    user.CreateWindowExW.argtypes = [w.DWORD, w.LPCWSTR, w.LPCWSTR, w.DWORD,
                                   c.c_int, c.c_int, c.c_int, c.c_int,
                                   w.HWND, w.HMENU, w.HINSTANCE, w.LPVOID]
    class Mouse(c.Structure):
        _fields_ = [("dx", w.LONG), ("dy", w.LONG), ("data", w.DWORD),
                    ("flags", w.DWORD), ("time", w.DWORD), ("extra", c.c_size_t)]
    class Keyboard(c.Structure):
        _fields_ = [("vk", w.WORD), ("scan", w.WORD), ("flags", w.DWORD),
                    ("time", w.DWORD), ("extra", c.c_size_t)]
    class Payload(c.Union):
        _fields_ = [("mouse", Mouse), ("key", Keyboard)]
    class Input(c.Structure):
        _anonymous_ = ("payload",)
        _fields_ = [("kind", w.DWORD), ("payload", Payload)]
    user.SendInput.argtypes = [w.UINT, c.POINTER(Input), c.c_int]
    callback_type = c.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
    user.EnumWindows.argtypes = [callback_type, w.LPARAM]
    trace: list[dict] = []
    held: set[int] = set()
    hwnd = None
    probe = None
    log = (output / "godot.log").open("w", encoding="utf-8")
    process = subprocess.Popen([str(engine), "--path", str(root / "godot"),
        "res://scenes/debug/car_viewer.tscn", "--max-fps", "60", "--quit-after", "18000", "--",
        "--driving-os-smoke", f"--drive-os-state={state_path}"], stdout=log, stderr=subprocess.STDOUT)

    def state() -> dict:
        try:
            return json.loads(state_path.read_text(encoding="utf-8"))
        except (FileNotFoundError, PermissionError, json.JSONDecodeError):
            return {}

    def wait(predicate, label: str, timeout: float = 6) -> dict:
        deadline = time.monotonic() + timeout
        last = {}
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError(f"Godot exited before {label}; see {output / 'godot.log'}")
            last = state()
            if last and predicate(last):
                trace.append({"check": label, "state": last})
                print(f"OS_CHECK_OK: {label}", flush=True)
                return last
            time.sleep(0.04)
        raise AssertionError(f"Timeout: {label}: {last}")

    def foreground() -> None:
        if user.GetForegroundWindow() != hwnd:
            raise RuntimeError("Refusing input: owned Godot window is not foreground")
        pid = w.DWORD()
        user.GetWindowThreadProcessId(hwnd, c.byref(pid))
        if pid.value != process.pid:
            raise RuntimeError("Refusing input: Godot window ownership changed")

    def activate_owned() -> None:
        current = kernel.GetCurrentThreadId()
        foreground_thread = user.GetWindowThreadProcessId(user.GetForegroundWindow(), None)
        attached = foreground_thread != current and bool(user.AttachThreadInput(current, foreground_thread, True))
        try:
            user.ShowWindow(hwnd, 9)
            user.SetWindowPos(hwnd, w.HWND(-1), 40, 40, 0, 0, 1)
            user.BringWindowToTop(hwnd)
            user.SetForegroundWindow(hwnd)
        finally:
            if attached:
                user.AttachThreadInput(current, foreground_thread, False)
        time.sleep(0.15)
        foreground()

    def send(event: Input) -> None:
        foreground()
        if user.SendInput(1, c.byref(event), c.sizeof(Input)) != 1:
            raise c.WinError(c.get_last_error())

    def key(scan: int, down: bool) -> None:
        event = Input()
        event.kind = 1
        event.key = Keyboard(0, scan, 8 | (0 if down else 2), 0, 0)
        send(event)
        if down:
            held.add(scan)
        else:
            held.discard(scan)

    def tap(scan: int) -> None:
        key(scan, True)
        time.sleep(0.08)
        key(scan, False)

    def click(x: int, y: int) -> None:
        # Repeated entry/focus tests can change foreground ownership.
        # Re-activate only our PID-bound test window, then retain every send guard.
        activate_owned()
        rect, point = w.RECT(), w.POINT()
        user.GetClientRect(hwnd, c.byref(rect))
        point.x, point.y = round(x * rect.right / 1280), round(y * rect.bottom / 800)
        user.ClientToScreen(hwnd, c.byref(point))
        target = user.WindowFromPoint(point)
        target_pid = w.DWORD()
        user.GetWindowThreadProcessId(target, c.byref(target_pid))
        if target_pid.value != process.pid:
            raise RuntimeError(f"Refusing click: client target ({point.x},{point.y}) belongs to PID {target_pid.value}, expected {process.pid}")
        print(f"OS_CLICK: client {x},{y} / {rect.right},{rect.bottom}; screen {point.x},{point.y}", flush=True)
        user.SetCursorPos(point.x, point.y)
        time.sleep(0.12)
        for flag in (2, 4):
            event = Input()
            event.kind = 0
            event.mouse = Mouse(0, 0, 0, flag, 0, 0)
            send(event)
            time.sleep(0.08)

    def widget(name: str) -> None:
        current = wait(lambda row: name in row.get("ui_targets", {}), f"visible ImGui target {name}")
        point = current["ui_targets"][name]
        # ImGui popup auto-fit can reposition after its first measured frame.
        # Observe a stable rendered target before sending an actual mouse click.
        stable_since = time.monotonic()
        deadline = stable_since + 5
        while time.monotonic() - stable_since < 0.25:
            if time.monotonic() > deadline:
                raise AssertionError(f"ImGui target did not settle: {name}")
            time.sleep(0.04)
            current = state()
            if not current:
                # The observer file can be between writes. No valid frame has
                # established a disappeared widget; wait for another full read.
                stable_since = time.monotonic()
                continue
            next_point = current.get("ui_targets", {}).get(name)
            if next_point is None:
                raise AssertionError(f"ImGui target disappeared before click: {name}")
            if next_point != point:
                point = next_point
                stable_since = time.monotonic()
        if name == "drive_transmission_1":
            from PIL import ImageGrab
            rect = w.RECT()
            user.GetWindowRect(hwnd, c.byref(rect))
            ImageGrab.grab(bbox=(rect.left, rect.top, rect.right, rect.bottom)).save(output / "manual-popup-before-click.png")
        click(round(point[0]), round(point[1]))

    def release_all() -> None:
        if hwnd and process.poll() is None:
            user.SetForegroundWindow(hwnd)
            for scan in list(held):
                key(scan, False)

    try:
        wait(lambda row: row["ready"], "preview ready", 20)
        windows = []
        @callback_type
        def collect(handle, _parameter):
            pid = w.DWORD()
            user.GetWindowThreadProcessId(handle, c.byref(pid))
            if pid.value == process.pid and user.IsWindowVisible(handle):
                windows.append(handle)
            return True
        user.EnumWindows(collect, 0)
        if len(windows) != 1:
            raise RuntimeError(f"Expected one visible owned window; found {len(windows)}")
        hwnd = windows[0]
        activate_owned()
        time.sleep(0.3)
        wait(lambda row: row["focused"] and row["window_focus"], "owned window focused")
        widget("test_drive")
        initial = wait(lambda row: row["phase"] == 2 and row["mouse"] == 2 and row["capture"], "click enters captured driving")
        assert initial["camera_behind"] < -0.9
        key(0x11, True)  # physical W
        wait(lambda row: row["throttle"] == 1 and row["speed"] > 1, "OS W produces native motion")
        time.sleep(0.5)
        moving = state()
        assert math.dist(initial["position"], moving["position"]) > 0.1
        key(0x1E, True)  # A
        wait(lambda row: row["steering"] > 0, "OS A steers left")
        key(0x1E, False)
        key(0x20, True)  # D
        wait(lambda row: row["steering"] < 0, "OS D steers right")
        key(0x20, False)
        key(0x11, False)
        key(0x1F, True)  # S
        wait(lambda row: row["brake"] == 1, "OS S brakes")
        key(0x1F, False)
        wait(lambda row: row["throttle"] == 0 and row["brake"] == 0 and row["steering"] == 0, "OS releases clear input")
        event = Input()
        event.kind = 0
        event.mouse = Mouse(220, 40, 0, 1, 0, 0)
        send(event)
        wait(lambda row: abs(row["yaw"]) > 0.2, "OS relative mouse orbits camera")
        wait(lambda row: abs(row["yaw"]) < 0.03 and row["camera_behind"] < -0.9, "idle camera returns behind", 5)
        tap(0x01)
        wait(lambda row: row["mouse"] == 0 and not row["capture"] and row["phase"] == 2, "Esc releases cursor without pause")
        click(900, 600)
        wait(lambda row: row["mouse"] == 2 and row["capture"], "world click recaptures cursor")
        tap(0x42)  # F8
        suspended = wait(lambda row: row["console"] and row["phase"] == 3 and row["mouse"] == 0, "F8 temporarily suspends")
        time.sleep(0.25)
        assert state()["step"] == suspended["step"]
        tap(0x42)
        wait(lambda row: not row["console"] and row["phase"] == 2 and row["mouse"] == 2 and row["step"] > suspended["step"], "F8 close automatically resumes")
        probe = user.CreateWindowExW(0, "STATIC", "Owned Test drive focus probe", 0x10CF0000,
                                     40, 40, 300, 140, None, None, None, None)
        if not probe:
            raise c.WinError(c.get_last_error())
        user.SetForegroundWindow(probe)
        suspended = wait(lambda row: not row["focused"] and row["phase"] == 3, "actual focus loss suspends")
        time.sleep(0.25)
        assert state()["step"] == suspended["step"]
        user.SetForegroundWindow(hwnd)
        user.DestroyWindow(probe)
        probe = None
        wait(lambda row: row["focused"] and row["phase"] == 2 and row["mouse"] == 2 and row["step"] > suspended["step"], "actual focus return automatically resumes")
        key(0x11, True)
        wait(lambda row: row["throttle"] == 1, "W works after automatic resume")
        key(0x11, False)
        from PIL import ImageGrab
        rect = w.RECT()
        user.GetWindowRect(hwnd, c.byref(rect))
        ImageGrab.grab(bbox=(rect.left, rect.top, rect.right, rect.bottom)).save(output / "driving.png")
        tap(0x01)
        wait(lambda row: row["mouse"] == 0, "release before return")
        widget("drive_exit")
        wait(lambda row: row["phase"] == 0 and row["ready"] and row["car"] == initial["car"], "return restores selected CarViewer model")
        time.sleep(0.3)
        widget("test_drive")
        wait(lambda row: row["phase"] == 2 and row["mouse"] == 2, "re-entry is immediately drivable")
        key(0x11, True)
        wait(lambda row: row["throttle"] == 1, "W works after re-entry")
        key(0x11, False)
        tap(0x01)
        wait(lambda row: row["mouse"] == 0, "release on second exit")
        if args.controls:
            widget("drive_transmission")
            widget("drive_transmission_1")
            wait(lambda row: row.get("powertrain", {}).get("manual_transmission") == 1, "OS GUI selects original manual transmission")
            tap(0x12)  # E upshift
            up = wait(lambda row: row.get("powertrain", {}).get("gear", -1) >= 3, "OS E upshifts actual forward gear")
            tap(0x10)  # Q downshift
            wait(lambda row: row.get("powertrain", {}).get("gear") == up["powertrain"]["gear"] - 1, "OS Q downshifts actual gear")
            widget("drive_neutral")
            neutral = wait(lambda row: row.get("powertrain", {}).get("gear") == 1, "OS GUI N selects original neutral")
            assert math.isfinite(neutral["powertrain"]["rpm"]) and math.isfinite(neutral["powertrain"]["speed"])
            key(0x11, True)
            time.sleep(0.4)
            neutral_throttle = state()
            assert neutral_throttle["powertrain"]["gear"] == 1
            key(0x11, False)
            key(0x1F, True)
            wait(lambda row: row.get("powertrain", {}).get("gear") == 0 and row["powertrain"].get("forward_speed", 0) < -0.1,
                 "OS S follows original braking into reverse motion", 15)
            key(0x1F, False)
            key(0x11, True)
            wait(lambda row: row.get("powertrain", {}).get("gear", 0) >= 2 and row["powertrain"].get("forward_speed", 0) > 0.1,
                 "OS W returns through original reverse-to-forward logic", 15)
            wait(lambda row: row["speed"] > 4, "OS forward acceleration before handbrake", 15)
            key(0x39, True)  # Space held handbrake
            wait(lambda row: row.get("handbrake", 0) == 1 and row.get("powertrain", {}).get("handbrake", False), "OS Space applies original handbrake")
            key(0x1E, True)
            wait(lambda row: row.get("skid_quads", 0) > 0, "Original skid commands reach visible mesh", 10)
            # Retain an actual rendered trail/HUD artifact while the same OS
            # W/Space/A inputs remain held; no control-state injection.
            time.sleep(0.3)
            rect = w.RECT()
            user.GetWindowRect(hwnd, c.byref(rect))
            ImageGrab.grab(bbox=(rect.left, rect.top, rect.right, rect.bottom)).save(output / "controls-skids.png")
            key(0x1E, False)
            key(0x39, False)
            key(0x11, False)
            wait(lambda row: row.get("handbrake", 0) == 0 and not row.get("powertrain", {}).get("handbrake", True), "OS Space release clears handbrake")
            widget("drive_transmission")
            widget("drive_transmission_0")
            wait(lambda row: row.get("powertrain", {}).get("manual_transmission") == 0, "OS GUI restores automatic transmission")
            # A new entry gives the zone run its own unchanged initial position/heading.
            widget("drive_exit")
            wait(lambda row: row["phase"] == 0 and row["ready"], "control test return")
            widget("test_drive")
            wait(lambda row: row["phase"] == 2 and row["mouse"] == 2, "control test clean re-entry")
            tap(0x01)
            wait(lambda row: row["mouse"] == 0, "release for surface controls")
        if args.surfaces:
            # TASK-0045: newly re-entered car is near the initial world origin.
            # Selector positions follow the pinned ImGui layout below Return.
            for index, terrain in enumerate((1, 2, 6, 7, 8)):
                widget("drive_material")
                widget(f"drive_material_{terrain}")
                selected = wait(lambda row: row.get("surface", {}).get("mode") == 0
                    and row.get("surface", {}).get("terrain") == terrain
                    and len(row.get("grip", [])) == 4
                    and all(g.get("terrain") == terrain for g in row["grip"]), f"OS uniform material {terrain} commits with four-wheel diagnostics")
                assert selected["surface_visual"] == f"0:{terrain}" and selected["surface_labels"] == 1
                for grip in selected["grip"]:
                    for field in ("static_coefficient", "dynamic_coefficient", "z_force", "current_slip", "road_noise"):
                        assert math.isfinite(grip[field])
            widget("drive_surface_mode")
            widget("drive_surface_mode_1")
            wait(lambda row: row.get("surface", {}).get("mode") == 1 and row["surface_labels"] == 5, "OS zones selection commits world-anchored presentation")
            widget("drive_grip")  # Open the read-only diagnostics child.
            click(900, 600)
            key(0x11, True)
            seen = set()
            mixed = False
            def crossed(row):
                nonlocal mixed
                materials = {g.get("terrain") for g in row.get("grip", [])}
                seen.update(materials)
                mixed = mixed or len(materials) > 1
                return row["position"][2] > 110 and {1, 2, 6, 7, 8}.issubset(seen) and mixed
            wait(crossed, "OS acceleration traverses all five world zones with mixed front/rear grip", 30)
            key(0x11, False)
            rect = w.RECT()
            user.GetWindowRect(hwnd, c.byref(rect))
            ImageGrab.grab(bbox=(rect.left, rect.top, rect.right, rect.bottom)).save(output / "surfaces.png")
            tap(0x01)
            wait(lambda row: row["mouse"] == 0 and not row["capture"], "release after surface traversal")
        widget("drive_exit")
        wait(lambda row: row["phase"] == 0 and row["ready"] and row.get("surface_labels", 0) == 0, "second return succeeds and surface labels are removed")
        (output / "report.json").write_text(json.dumps({"result": "passed", "owned_pid": process.pid,
            "checks": trace}, indent=2), encoding="utf-8")
        print(f"DRIVING_OS_INPUT_OK: {len(trace)} checks; {output}")
        return 0
    finally:
        (output / "last-attempt.json").write_text(json.dumps({"returncode": process.poll(), "checks": trace}, indent=2), encoding="utf-8")
        if probe:
            user.DestroyWindow(probe)
        try:
            release_all()
        finally:
            if process.poll() is None:
                if hwnd:
                    user.PostMessageW(hwnd, 0x10, 0, 0)
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.terminate()
                    process.wait(timeout=5)
            log.close()



if __name__ == "__main__":
    raise SystemExit(main())
