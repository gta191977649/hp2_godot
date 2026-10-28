"""TASK-0098: run the actual GPU game window with scripted production inputs.

Returns failure on runtime errors, missing checkpoints, timeout or missing image,
even if Godot itself exits zero. Does not use desktop automation or fabricate pages.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--godot', type=Path, default=ROOT / 'local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe')
    parser.add_argument('--output', type=Path, default=ROOT / 'local/task0098/selection-roundtrip')
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(ROOT / 'local'):
        parser.error('Private screenshots and logs must remain under ignored local/')
    # Every invocation has a fresh directory: old success/images cannot satisfy a run.
    import tempfile
    output.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='run-', dir=output))
    dll = ROOT / 'godot/bin/ps2_port.dll'
    command = [str(args.godot.resolve()), '--path', str(ROOT / 'godot'),
        '--script', 'res://scripts/runtime/frontend_selection_roundtrip.gd', '--',
        '--screen=0', '--scale=1', '--expect-debug=true', '--roundtrip-output=' + str(run)]
    env = os.environ.copy()
    # Force the original empty-profile startup and isolate any private writes.
    # The nonexistent read-only input never modifies a real player save.
    env['HP2_PS2_PROFILE_PATH'] = str(run / 'absent-input-profile')
    env['HP2_PORT_SAVE_ROOT'] = str(run / 'saves')
    manifest = {'command': command, 'dll_sha256': hashlib.sha256(dll.read_bytes()).hexdigest(),
        'godot_sha256': hashlib.sha256(args.godot.read_bytes()).hexdigest(), 'scope': 'actual window and production input; no original-runtime equivalence'}
    with (run / 'godot.log').open('w', encoding='utf-8') as log:
        try:
            process = subprocess.Popen(command, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT)
            try:
                manifest['exit_code'] = process.wait(timeout=100)
            except subprocess.TimeoutExpired:
                # Only the process created by this runner is terminated.
                process.terminate()
                process.wait(timeout=10)
                manifest['timeout'] = True
                manifest['exit_code'] = process.returncode
        except OSError as error:
            manifest['launch_error'] = str(error)
    log = (run / 'godot.log').read_text(encoding='utf-8', errors='replace')
    errors = [line for line in log.splitlines() if re.search(r'NOT_IMPLEMENTED|SCRIPT ERROR:|^ERROR:|FATAL|CrashHandlerException|RID.*leaked', line)]
    manifest['runtime_errors'] = errors
    result_path = run / 'result.json'
    result = json.loads(result_path.read_text(encoding='utf-8')) if result_path.exists() else {}
    png = run / 'final.png'
    image_valid = False
    if png.exists():
        with png.open('rb') as image:
            header = image.read(24)
        image_valid = len(header) == 24 and header[:8] == b'\x89PNG\r\n\x1a\n' and min(struct.unpack('>II', header[16:24])) > 0
    labels = {checkpoint['label'] for checkpoint in result.get('checkpoints', [])}
    required = {'main-menu', 'mode-select', 'event-tree', 'event-details', 'final'} | {
        f'{prefix}-{cycle}' for prefix in ('select-car', 'returned') for cycle in range(3)}
    ok = manifest.get('exit_code') == 0 and not errors and result.get('ok') is True and image_valid and required <= labels
    manifest.update(ok=ok, result=result, screenshot_valid=image_valid, missing_checkpoints=sorted(required - labels))
    (run / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print(('PASS' if ok else 'FAIL') + ' production selection roundtrip: ' + str(run))
    if not ok:
        print(result.get('reason', manifest.get('launch_error', 'missing valid result')))
        for error in errors[:8]:
            print(error)
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
