"""TASK-0005 infrastructure CLI. Synthetic passes never imply PS2 equivalence."""
import argparse
import datetime
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
LOGS = ROOT / 'local/pipeline'
GODOT = Path(os.environ.get('PORT_GODOT', ROOT / 'local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe'))

def write_json(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix('.tmp')
    temporary.write_text(json.dumps(data, indent=2), encoding='utf-8')
    temporary.replace(path)

def run(command, label, timeout=300, cwd=ROOT):
    LOGS.mkdir(parents=True, exist_ok=True)
    try:
        result = subprocess.run([str(x) for x in command], cwd=cwd, capture_output=True, text=True, timeout=timeout)
        output = result.stdout + result.stderr
        code = result.returncode
    except subprocess.TimeoutExpired as error:
        output = str(error)
        code = 124
    (LOGS / (label + '.log')).write_text(output, encoding='utf-8')
    write_json(LOGS / (label + '.json'), {'command': [str(x) for x in command], 'exit_code': code, 'log': str(LOGS / (label + '.log'))})
    if code or 'ERROR:' in output or 'SCRIPT ERROR:' in output:
        raise RuntimeError(f'{label} failed ({code}): {output[-2000:]}')
    return output

def compare(expected, actual, rules):
    differences = []
    for key, value in expected.items():
        rule = rules[key]
        got = actual.get(key)
        if rule['kind'] == 'integer':
            passed = type(value) is int and type(got) is int and value == got
        elif rule['kind'] == 'float':
            absolute, relative = rule.get('absolute', 0), rule.get('relative', 0)
            if absolute < 0 or relative < 0:
                raise ValueError('negative tolerance')
            passed = isinstance(got, (int, float)) and math.isfinite(got) and math.isfinite(value) and math.isclose(value, got, abs_tol=absolute, rel_tol=relative)
        else:
            raise ValueError('unknown comparison kind')
        if not passed:
            differences.append({'field': key, 'expected': value, 'actual': got, 'rule': rule})
    return differences

def verify(runner, fixture=ROOT / 'tools/fixtures/demo.json', actual=None):
    fixture = Path(fixture)
    raw = fixture.read_bytes()
    data = json.loads(raw)
    # The demo runner deliberately supports one initial state and no PS2 state.
    if data['provenance']['kind'] != 'synthetic' or data['initial'] != {'registers': {}, 'memory': {}, 'frame': 0, 'position': 0} or data['memory_side_effects']:
        raise ValueError('unsupported fixture execution contract; real oracle adapter required')
    if any(type(x) is not int or x not in (-1, 0, 1) for x in data['inputs']):
        raise ValueError('invalid input')
    if actual is None:
        actual = json.loads(run([runner, *data['inputs']], 'native-fixture', 30))
    differences = compare(data['expected'], actual, data['rules'])
    report = {'fixture': data['id'], 'fixture_sha256': hashlib.sha256(raw).hexdigest(), 'provenance': data['provenance'], 'result': 'FAIL' if differences else 'PASS', 'verification_status': 'failed' if differences else 'tested_synthetic', 'differences': differences, 'actual': actual}
    write_json(LOGS / 'differential.json', report)
    return report

def build(preset='godot-debug'):
    sources = sorted([*ROOT.glob('src/**/*.cpp'), *ROOT.glob('src/**/*.hpp'), *ROOT.glob('godot/scripts/*.gd'), ROOT / 'CMakeLists.txt'])
    digest = hashlib.sha256()
    for path in sources:
        digest.update(str(path.relative_to(ROOT)).encode())
        digest.update(path.read_bytes())
    write_json(ROOT / 'godot/build_info.json', {'id': preset + '-' + digest.hexdigest()[:16], 'source_sha256': digest.hexdigest(), 'provenance': 'synthetic infrastructure'})
    run(['cmake', '--preset', preset, '-DPython3_EXECUTABLE=' + sys.executable], 'configure-' + preset)
    run(['cmake', '--build', '--preset', preset, '--parallel', '4'], 'build-' + preset, 1800)

def test():
    run(['ctest', '--preset', 'godot-debug'], 'ctest', 120)
    run([GODOT, '--headless', '--path', 'godot', '--editor', '--import'], 'godot-import', 120)
    output = run([GODOT, '--headless', '--path', 'godot', 'res://scenes/main.tscn', '--', '--smoke-test'], 'godot-smoke', 30)
    if 'PORT_SMOKE_OK' not in output:
        raise RuntimeError('missing smoke marker')
    feedback([GODOT, '--headless', '--path', 'godot', 'res://scenes/main.tscn'], 'feedback-headless')

def feedback(command, label):
    output = run([*command, '--', '--feedback-test'], label, 60)
    lines = [x.split('FEEDBACK_OK: ', 1)[1].strip() for x in output.splitlines() if 'FEEDBACK_OK: ' in x]
    if len(lines) != 1:
        raise RuntimeError('missing feedback marker')
    report = json.loads(Path(lines[0]).read_text(encoding='utf-8'))
    if not report['input_history'] or not report['build_id'] or report['schema_version'] != 1:
        raise RuntimeError('invalid feedback report')
    if '--headless' not in command and not Path(report['screenshot']).is_file():
        raise RuntimeError('missing screenshot')
    write_json(LOGS / (label + '-report.json'), report)

def export(release=False):
    # Rebuild intended variant before sharing the DLL output path.
    preset = 'godot-release' if release else 'godot-debug'
    build(preset)
    if not release:
        test()
    destination = ROOT / ('out/windows-release' if release else 'out/windows-debug')
    destination.mkdir(parents=True, exist_ok=True)
    flag, name = ('--export-release', 'Windows Release') if release else ('--export-debug', 'Windows Debug')
    run([GODOT, '--headless', '--path', 'godot', flag, name, destination / 'port.exe'], 'export-' + preset, 180)
    output = run([destination / 'port.exe', '--headless', 'res://scenes/main.tscn', '--', '--smoke-test'], 'launch-' + preset, 30)
    if 'PORT_SMOKE_OK' not in output:
        raise RuntimeError('export did not load native extension')
    feedback([str(destination / 'port.exe'), 'res://scenes/main.tscn'], 'playable-feedback-' + preset)

def apply_proposal(root, proposal):
    """Agent receives content interface only; never executes in trusted checkout."""
    root = Path(root).resolve()
    checked = []
    for name, content in proposal.items():
        path = (root / name).resolve()
        if not path.is_relative_to(root / 'src/game_core') or path.suffix not in ('.hpp', '.cpp') or path.is_symlink():
            raise ValueError('proposal outside allowed core source scope')
        checked.append((path, content))
    snapshot = {str(path): path.read_text(encoding='utf-8') if path.exists() else None for path, _ in checked}
    for path, content in checked:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding='utf-8')
    return snapshot

def rollback(snapshot):
    for name, content in snapshot.items():
        path = Path(name)
        if content is None:
            path.unlink(missing_ok=True)
        else:
            path.write_text(content, encoding='utf-8')

def controller(limit=2):
    if not 1 <= limit <= 10:
        raise ValueError('attempt limit must be 1..10')
    state_path = LOGS / 'queue-state.json'
    state = json.loads(state_path.read_text()) if state_path.exists() else json.loads((ROOT / 'tools/queue.json').read_text())
    for task in state['tasks']:
        if task['status'] == 'tested_synthetic':
            continue
        if any(next((x['status'] for x in state['tasks'] if x['id'] == dep), 'missing') not in ('verified', 'tested_synthetic') for dep in task['dependencies']):
            task.update(status='blocked', reason='dependency unavailable')
            continue
        task['status'] = 'not_tested'
        write_json(state_path, state)
        for attempt in range(1, limit + 1):
            evidence = {'attempt': attempt, 'task': task['id'], 'repair': 'blocked: no reconstruction agent provider configured', 'time_utc': datetime.datetime.now(datetime.timezone.utc).isoformat()}
            try:
                build('native-debug')
                run(['ctest', '--preset', 'native-debug'], f'controller-tests-{attempt}', 120)
                result = verify(ROOT / 'build/native-debug/demo_runner.exe', ROOT / task['fixture'])
                task.update(status=result['verification_status'], attempts=attempt)
                evidence['result'] = result
            except (RuntimeError, ValueError, OSError) as error:
                task.update(status='failed', attempts=attempt, reason=str(error))
                evidence['error'] = str(error)
            write_json(LOGS / f"{task['id']}-attempt-{attempt}.json", evidence)
            write_json(state_path, state)
            if task['status'] == 'tested_synthetic':
                break
    write_json(state_path, state)
    return state

def doctor():
    result = {'python': sys.version, 'godot': str(GODOT), 'godot_exists': GODOT.is_file(), 'tools': {x: shutil.which(x) for x in ('cmake', 'ninja', 'g++', 'git', 'pcsx2-qt')}, 'templates': str(Path(os.environ['APPDATA']) / 'Godot/export_templates/4.4.1.stable'), 'original_oracle': 'not validated'}
    for tool in ('cmake', 'ninja', 'g++', 'git'):
        if result['tools'][tool]:
            result[tool] = run([result['tools'][tool], '--version'], 'doctor-' + tool, 10).splitlines()[0]
    if GODOT.is_file():
        result['godot_version'] = run([GODOT, '--version'], 'doctor-godot', 10).strip()
    write_json(LOGS / 'doctor.json', result)
    return result

def demo_repair(limit=2):
    """Bounded scripted provider demonstration in an isolated source directory."""
    if limit < 2 or limit > 10:
        raise ValueError('demo needs 2..10 attempts to demonstrate failure and repair')
    workspace = LOGS / 'synthetic-repair'
    source = ROOT / 'src/game_core/demo.hpp'
    trusted = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in [ROOT / 'tools/fixtures/demo.json', ROOT / 'src/tests/demo_runner.cpp', source]}
    correct = source.read_text()
    incorrect = correct.replace('axis < 0 ? -1 : axis > 0 ? 1 : 0', 'axis != 0 ? 1 : 0')
    snapshots = apply_proposal(workspace, {'src/game_core/demo.hpp': incorrect})
    write_json(workspace / 'checkpoint.json', snapshots)
    attempts = []
    for index in range(1, limit + 1):
        command = ['g++', '-std=c++17', '-I' + str(workspace / 'src'), ROOT / 'src/tests/demo_runner.cpp', '-o', workspace / 'runner.exe']
        run(command, f'repair-build-{index}', 60)
        result = verify(workspace / 'runner.exe')
        attempts.append({'attempt': index, 'provider': 'scripted synthetic demonstration, not Codex reconstruction', 'result': result})
        write_json(LOGS / f'repair-attempt-{index}.json', attempts[-1])
        if result['result'] == 'PASS': break
        # Equivalent rollback checkpoint restores previous contents before proposal.
        rollback(snapshots)
        snapshots = apply_proposal(workspace, {'src/game_core/demo.hpp': correct})
        write_json(workspace / 'checkpoint.json', snapshots)
    unchanged = all(hashlib.sha256(Path(name).read_bytes()).hexdigest() == digest for name, digest in trusted.items())
    report = {'status': 'tested_synthetic' if attempts[-1]['result']['result'] == 'PASS' and unchanged else 'failed', 'attempts': attempts, 'trusted_files_unchanged': unchanged, 'real_agent': 'blocked: no provider configured', 'checkpoint': str(workspace / 'checkpoint.json')}
    write_json(LOGS / 'repair-cycle.json', report)
    return report

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('command', choices=['setup', 'doctor', 'build', 'test', 'verify', 'export', 'status', 'cycle', 'demo-repair'])
    parser.add_argument('--runner', type=Path, default=ROOT / 'build/godot-debug/demo_runner.exe')
    parser.add_argument('--release', action='store_true')
    parser.add_argument('--attempts', type=int, default=2)
    args = parser.parse_args()
    if args.command == 'setup':
        archive = ROOT / 'local/templates/godot-4.4.1.tpz'
        destination = Path(os.environ['APPDATA']) / 'Godot/export_templates/4.4.1.stable'
        if archive.exists():
            destination.mkdir(parents=True, exist_ok=True)
            with zipfile.ZipFile(archive) as source:
                for name in ('windows_debug_x86_64.exe', 'windows_release_x86_64.exe', 'version.txt'):
                    (destination / name).write_bytes(source.read('templates/' + name))
        print(json.dumps(doctor(), indent=2))
    elif args.command == 'doctor': print(json.dumps(doctor(), indent=2))
    elif args.command == 'build': build()
    elif args.command == 'test': test()
    elif args.command == 'export': export(args.release)
    elif args.command == 'cycle': print(json.dumps(controller(args.attempts), indent=2))
    elif args.command == 'demo-repair': print(json.dumps(demo_repair(args.attempts), indent=2))
    elif args.command == 'status':
        path = LOGS / 'queue-state.json'
        print(path.read_text() if path.exists() else (ROOT / 'tools/queue.json').read_text())
    elif args.command == 'verify':
        report = verify(args.runner)
        print(json.dumps(report, indent=2))
        return int(report['result'] != 'PASS')
    return 0

if __name__ == '__main__':
    try:
        sys.exit(main())
    except (RuntimeError, ValueError, OSError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
