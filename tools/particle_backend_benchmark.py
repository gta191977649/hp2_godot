"""TASK0085 port-only GPU particle comparison; never attests PS2 equivalence."""
import argparse
import json
from pathlib import Path
import statistics
import subprocess

import task0077_baseline as baseline

ROOT = Path(__file__).resolve().parents[1]
CASES = [('native_a', 'native', 'default'), ('native_b', 'native', 'default'),
         ('gpu', 'godot_gpu', 'default'), ('off', 'native', 'particles_off')]
ROUTES = [('ordinary', 'driving', 'drive', 'start'),
          ('dense', 'viewer', 'dense', 'forest_fire')]


def percentile(values, fraction):
    values = sorted(values)
    return values[min(len(values) - 1, int((len(values) - 1) * fraction))] if values else None


def summarize(data):
    samples = [s for row in data['tracks'] for s in row['samples']]
    med = statistics.median(row['summary']['wall_ms']['median'] for row in data['tracks'])
    result = dict(median_ms=med, fps=1000 / med,
                  p95_ms=statistics.median(row['summary']['wall_ms']['p95'] for row in data['tracks']))
    for key in ['cost_particle_simulation_ms', 'cost_particle_packet_ms',
                'cost_particle_packing_ms', 'cost_particle_upload_ms', 'video_memory_bytes', 'particle_active']:
        values = [s[key] for s in samples if isinstance(s.get(key), (float, int))]
        result[key] = dict(median=statistics.median(values), p95=percentile(values, .95)) if values else None
    result['particle_counters'] = [row.get('host_stats', {}).get('particles', {}) for row in data['tracks']]
    return result


def report(directory):
    report = dict(task='TASK-0085', comparisons=[], default_backend='native',
                  visual_review_passed=False, limitations=[
                      'Wall frame time is authoritative; delayed viewport GPU queries are not promoted.',
                      'Particles-off disables presentation; original emission/lifetime bookkeeping still runs.',
                      'Visual approval and independent GPU curve checks are required in addition to timing.'])
    for size in ['2880x1695', '1920x1080']:
        for route, *_ in ROUTES:
            runs = {}
            for case, backend, _ in CASES:
                path = directory / size / route / case / 'run.json'
                if not path.exists():
                    raise ValueError('Missing run: ' + str(path))
                data = json.loads(path.read_text(encoding='utf-8-sig'))
                errors = baseline.validate(data, strict=size == '2880x1695')
                if (data.get('warm_frames'), data.get('sample_frames'), data.get('repeats')) != (120, 600, 3):
                    errors.append('Requires 120 warm / 600 measured / 3 repeats')
                if data.get('particle_backend') != backend:
                    errors.append('Backend mismatch')
                check = json.loads(path.with_name('identity-check.json').read_text(encoding='utf-8-sig'))
                if not check.get('passed'):
                    errors.append('Inputs changed during run')
                if errors:
                    raise ValueError(str(path) + ': ' + '; '.join(errors))
                runs[case] = data
            summaries = {k: summarize(v) for k, v in runs.items()}
            a, b, gpu = (summaries[k]['p95_ms'] for k in ['native_a', 'native_b', 'gpu'])
            reference = (a + b) / 2
            noise = abs(a - b) / reference * 100
            improvement = (reference - gpu) / reference * 100
            aa_difference = baseline.first_difference(baseline.evidence(runs['native_a']), baseline.evidence(runs['native_b']))
            # GPU draw count/order may differ; physical results must remain identical.
            physics = lambda d: [[s.get('driving', {}).get('body_bits') for s in r['samples']] for r in d['tracks']]
            physical_difference = baseline.first_difference(physics(runs['native_a']), physics(runs['gpu']))
            report['comparisons'].append(dict(resolution=size, route=route, summaries=summaries,
                aa_p95_noise_percent=noise, gpu_p95_improvement_percent=improvement,
                improvement_gate=improvement > 5 and improvement > 2 * noise,
                ordinary_regression_gate=route != 'ordinary' or improvement >= -3,
                aa_state_difference=aa_difference, physics_difference=physical_difference))
    report['timing_gate_passed'] = all(c['improvement_gate'] and c['ordinary_regression_gate']
        and c['aa_state_difference'] is None and c['physics_difference'] is None for c in report['comparisons'])
    baseline.write(directory / 'summary.json', report)
    print(json.dumps(report, indent=2))


def run(directory):
    for width, height in [(2880, 1695), (1920, 1080)]:
        for route, context, path, pose in ROUTES:
            for case, backend, scenario in CASES:
                output = directory / f'{width}x{height}' / route / case
                if (output / 'run.json').exists():
                    raise ValueError('Preserve existing run; choose a new directory: ' + str(output))
                command = ['powershell', '-NoProfile', '-File', str(ROOT / 'tools/run_track_costs.ps1'),
                    '-ProjectPath', str(ROOT / 'godot'), '-OutputDirectory', str(output),
                    '-Context', context, '-Car', '911TURBO', '-Route', path, '-Pose', pose,
                    '-Scenario', scenario, '-Width', str(width), '-Height', str(height),
                    '-WarmFrames', '120', '-Samples', '600', '-Repeats', '3',
                    '-Candidates', '--particle-backend=' + backend]
                subprocess.run(command, cwd=ROOT, check=True)
    report(directory)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument('action', choices=['run', 'report'])
    parser.add_argument('directory', type=Path)
    args = parser.parse_args()
    (run if args.action == 'run' else report)(args.directory.resolve())
