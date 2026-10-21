"""Port-side immutable benchmark inputs and A/A-first diagnostics; no PS2 claims."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def write(path, value):
    Path(path).write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


def input_paths(project, engine):
    project, engine = Path(project).resolve(), Path(engine).resolve()
    if not (project / 'bin/ps2_port.dll').is_file() or not engine.is_file():
        raise ValueError('A real extension DLL and engine executable are required')
    inputs = {'engine': str(engine)}
    for path in project.rglob('*'):
        rel = path.relative_to(project)
        if path.is_file() and '.godot' not in rel.parts and path.suffix in {
                '.gd', '.gdshader', '.gdshaderinc', '.glsl', '.godot', '.gdextension', '.dll', '.tscn'}:
            inputs['project/' + rel.as_posix()] = str(path)
    for path in (ROOT / 'src').rglob('*'):
        if path.is_file() and path.suffix in {'.cpp', '.hpp', '.h', '.glsl', '.gdshader'}:
            inputs['source/' + path.relative_to(ROOT).as_posix()] = str(path)
    for rel in ['CMakeLists.txt', 'CMakePresets.json', 'build.ps1',
                'tools/run_track_costs.ps1', 'tools/task0077_baseline.py']:
        inputs['source/' + rel] = str(ROOT / rel)
    for name in ['CMakeCache.txt', 'build.ninja', 'compile_commands.json']:
        path = ROOT / 'build/godot-debug' / name
        if path.is_file():
            inputs['configured_recipe/' + name] = str(path)
    return inputs


def identity(project, engine):
    inputs = input_paths(project, engine)
    pin = subprocess.check_output(['git', '-C', str(ROOT / 'third_party/godot-cpp'),
                                   'rev-parse', 'HEAD'], text=True).strip()
    return dict(schema=1, project=str(Path(project).resolve()), engine=str(Path(engine).resolve()),
                inputs=inputs, hashes={key: digest(path) for key, path in inputs.items()},
                godot_cpp_revision=pin,
                configured_recipe_files=[key for key in inputs if key.startswith('configured_recipe/')],
                compiler_provenance='Configured recipe snapshot, not an attestation that it produced the copied DLL; DLL SHA256 is authoritative.',
                engine_version=subprocess.check_output([str(engine), '--version'], text=True).strip())


def verify(manifest):
    changed = []
    if 'project' in manifest and 'engine' in manifest:
        current_paths = input_paths(manifest['project'], manifest['engine'])
        changed.extend(sorted(set(current_paths) ^ set(manifest['inputs'])))
    for key, path in manifest['inputs'].items():
        if not Path(path).is_file() or digest(path) != manifest['hashes'].get(key):
            changed.append(key)
    pin = subprocess.check_output(['git', '-C', str(ROOT / 'third_party/godot-cpp'),
                                   'rev-parse', 'HEAD'], text=True).strip()
    if pin != manifest['godot_cpp_revision']:
        changed.append('godot_cpp_revision')
    return dict(passed=not changed, changed=changed)


def first_difference(a, b, path='$'):
    """Return one stable structural difference, including command index if supplied."""
    if type(a) is not type(b):
        return dict(path=path, reference=a, candidate=b)
    if isinstance(a, dict):
        for key in sorted(a.keys() | b.keys()):
            if key not in a or key not in b:
                return dict(path=path + '.' + key, reference=a.get(key), candidate=b.get(key), missing=True)
            found = first_difference(a[key], b[key], path + '.' + key)
            if found:
                return found
    elif isinstance(a, list):
        for index, (x, y) in enumerate(zip(a, b)):
            found = first_difference(x, y, f'{path}[{index}]')
            if found:
                return found
        if len(a) != len(b):
            return dict(path=path + '.length', reference=len(a), candidate=len(b))
    elif a != b:
        return dict(path=path, reference=a, candidate=b)
    return None


CONFIG = ['engine', 'adapter', 'rendering_method', 'rendering_driver', 'viewport',
          'build', 'context', 'selected_car', 'route_kind', 'pose', 'heading_degrees',
          'warm_frames', 'sample_frames', 'repeats', 'candidate_flags', 'profiler_enabled',
          'profiler_ui', 'render_prepare_workers', 'render_thread_requested',
          'environment_capture_requested', 'port_ui_optimized_debug',
          'render_selection_optimized_debug', 'render_host_optimized_debug', 'visual_dt']


def validate(data, strict=True):
    errors = []
    if data.get('measurement_protocol') != 'task0077-export-separated-v1':
        errors.append('Missing export-separated protocol; historical captures are not silently reclassified')
    hashes = data.get('run_identity', {}).get('hashes', {})
    for key in ['engine', 'project/bin/ps2_port.dll', 'project/project.godot',
                'project/scripts/runtime/track_performance.gd', 'project/scripts/debug/profile_debug.gd']:
        if not hashes.get(key):
            errors.append('Missing identity: ' + key)
    if strict and (data.get('warm_frames'), data.get('sample_frames'), data.get('repeats'),
                   data.get('viewport')) != (120, 600, 3, [2880, 1695]):
        errors.append('Strict baseline requires 120 warm / 600 measured / 3 repeats at 2880x1695')
    if len(data.get('tracks', [])) != data.get('repeats'):
        errors.append('Missing or extra repeats (one route/track per control document)')
    for index, row in enumerate(data.get('tracks', [])):
        samples = row.get('samples', [])
        if row.get('repeat') != index or not row.get('timing_valid') or row.get('error'):
            errors.append(f'Repeat {index}: invalid route or duplicate repeat')
        if len(samples) != data.get('sample_frames') or row.get('completed_samples') != len(samples):
            errors.append(f'Repeat {index}: incomplete samples')
        if [s.get('frame') for s in samples] != list(range(data['warm_frames'], data['warm_frames'] + len(samples))):
            errors.append(f'Repeat {index}: missing/duplicate/reordered logical frames')
        if any(s.get('gpu_query_source_verified') is not False or s.get('viewport_gpu_ms') is not None for s in samples):
            errors.append(f'Repeat {index}: unverified direct GPU time promoted')
        if data.get('profiler_enabled'):
            if not row.get('export_outside_measurement') or not row.get('profile_exported') or not row.get('profile_export_was_held'):
                errors.append(f'Repeat {index}: export boundary/completion missing')
            if row.get('profile_samples') != min(data['sample_frames'], 600):
                errors.append(f'Repeat {index}: Profile sample count mismatch')
            if not (isinstance(row.get('measurement_end_usec'), int) and
                    isinstance(row.get('profile_export_started_usec'), int) and
                    row['profile_export_started_usec'] >= row['measurement_end_usec']):
                errors.append(f'Repeat {index}: export overlaps measurement')
        if row.get('configuration', {}).get('render_thread_actual') != data.get('render_thread_requested'):
            errors.append(f'Repeat {index}: actual thread mode differs')
        if row.get('configuration', {}).get('render_prepare_workers') != data.get('render_prepare_workers'):
            errors.append(f'Repeat {index}: actual preparation worker count differs')
        environments = [e for e in row.get('effects', []) if e.get('id') == 'environment']
        expected_capture = bool(data.get('environment_capture_requested')) and row.get('scenario') != 'environment_off'
        if len(environments) != 1 or environments[0].get('enabled') is not expected_capture:
            errors.append(f'Repeat {index}: requested/actual environment capture differs')
    return errors


def evidence(data):
    """Only deterministic state; timing, engine IDs and source-frame domains excluded."""
    rows = []
    for row in data.get('tracks', []):
        samples = []
        for sample in row.get('samples', []):
            state = {k: sample.get(k) for k in ['frame', 'commands', 'packet_digest', 'zone',
                     'selected_sections', 'considered_instances', 'selected_instances']}
            driving = sample.get('driving', {})
            state['views'] = sample.get('native_selection', {})
            state['driving'] = {k: driving.get(k) for k in ['step_before', 'step_after', 'step_delta',
                               'native_tick_calls', 'input', 'body_bits', 'camera_bits', 'native_dt']}
            samples.append(state)
        rows.append(dict(repeat=row.get('repeat'), samples=samples, final_frame={k: row.get('final_frame', {}).get(k) for k in
                             ['packet_digest', 'commands', 'sections', 'visible_instances']},
                         pose_bits=row.get('pose_bits'), effects=row.get('effects')))
    return rows


def compare(reference, control, candidate=None, strict=True):
    errors = [f'{label}: {error}' for label, data in [('reference', reference), ('control', control)]
              for error in validate(data, strict)]
    config = first_difference({k: reference.get(k) for k in CONFIG}, {k: control.get(k) for k in CONFIG}, '$.configuration')
    identities = first_difference(reference.get('run_identity', {}).get('hashes'), control.get('run_identity', {}).get('hashes'), '$.identity')
    divergence = first_difference(evidence(reference), evidence(control), '$.repeats')
    if config or identities or divergence:
        errors.append('A/A control differs; candidate evaluation blocked')
    result = dict(task='TASK0077', control_passed=not errors, errors=errors,
                  first_divergence=config or identities or divergence,
                  gpu_whole_frame_ms=None, command_detail='Command/resource detail only where exported; empty timed digests are unavailable, not parity proof.',
                  scope='Port-side input identity and deterministic state control; not image parity, PS2 verification or speedup acceptance')
    result['routes'] = []
    for label, data in [('reference', reference), ('control', control)]:
        for row in data.get('tracks', []):
            result['routes'].append(dict(side=label, repeat=row.get('repeat'), wall_ms=row.get('summary', {}).get('wall_ms'),
                export_dispatch_ms=row.get('profile_export_dispatch_ms'), export_wait_ms=row.get('profile_export_wait_ms'),
                export_total_ms=row.get('profile_export_total_ms'), costs=row.get('costs'), host_stats=row.get('host_stats')))
    medians = [r.get('summary', {}).get('wall_ms', {}).get('median') for d in [reference, control] for r in d.get('tracks', [])]
    medians = [m for m in medians if isinstance(m, (int, float))]
    result['baseline_median_range_ms'] = max(medians) - min(medians) if medians else None
    result['candidate_evaluated'] = False
    if candidate is not None and result['control_passed']:
        result['candidate_evaluated'] = True
        result['candidate_validation_errors'] = validate(candidate, strict)
        result['candidate_configuration_difference'] = first_difference(
            {k: reference.get(k) for k in CONFIG}, {k: candidate.get(k) for k in CONFIG})
        result['candidate_first_divergence'] = first_difference(evidence(reference), evidence(candidate), '$.repeats')
        result['candidate_policy'] = 'Diagnostic only; use the existing optimization-specific comparator for allowed configuration changes and performance gates.'
    return result


def load_run(path):
    path = Path(path)
    data = json.loads(path.read_text(encoding='utf-8-sig'))
    receipt = json.loads((path.parent / 'identity-check.json').read_text(encoding='utf-8-sig'))
    manifest_path = path.parent / 'identity.json'
    if not receipt.get('passed') or receipt.get('manifest_sha256') != digest(manifest_path):
        raise ValueError('Missing or invalid post-run identity receipt: ' + str(path))
    if data.get('run_identity') != json.loads(manifest_path.read_text(encoding='utf-8-sig')):
        raise ValueError('Run identity does not match checked manifest: ' + str(path))
    error_log = path.parent / 'errors.log'
    if not error_log.is_file() or 'ERROR:' in error_log.read_text(encoding='utf-8', errors='replace'):
        raise ValueError('Missing or failing runtime error log: ' + str(path))
    return data


def main():
    parser = argparse.ArgumentParser(__doc__)
    subs = parser.add_subparsers(dest='command', required=True)
    command = subs.add_parser('identity')
    command.add_argument('--project', type=Path, required=True)
    command.add_argument('--engine', type=Path, required=True)
    command.add_argument('--output', type=Path, required=True)
    command = subs.add_parser('verify')
    command.add_argument('manifest', type=Path)
    command.add_argument('--output', type=Path, required=True)
    command = subs.add_parser('compare')
    command.add_argument('reference', type=Path)
    command.add_argument('control', type=Path)
    command.add_argument('--candidate', type=Path)
    command.add_argument('--diagnostic', action='store_true')
    command.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.command == 'identity':
        write(args.output, identity(args.project, args.engine))
        print('TASK0077_IDENTITY_SAVED')
        return 0
    if args.command == 'verify':
        result = verify(json.loads(args.manifest.read_text(encoding='utf-8-sig')))
        result['manifest_sha256'] = digest(args.manifest)
    else:
        result = compare(load_run(args.reference), load_run(args.control),
                         load_run(args.candidate) if args.candidate else None, strict=not args.diagnostic)
    write(args.output, result)
    print(json.dumps({k: v for k, v in result.items() if k not in {'routes'}}, indent=2))
    return 0 if result.get('passed', result.get('control_passed', False)) else 1


if __name__ == '__main__':
    raise SystemExit(main())
