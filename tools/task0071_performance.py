"""TASK0071 repeat-based host performance comparison; never PS2 equivalence.

Usage: py -3 tools/task0071_performance.py BASELINE.json CANDIDATE.json --output REPORT.json
The performance gate does not replace separate colour/depth/reflection image checks.
"""
import argparse
import json
import math
import statistics
from pathlib import Path

METRICS = ('wall_ms', 'prepare_ms', 'native_ms', 'submit_ms',
           'cost_previous_sun_readback_ms', 'viewport_cpu_ms', 'viewport_gpu_ms')
QUANTILES = ('median', 'p95', 'p99')


def _finite(value):
    return isinstance(value, (int, float)) and math.isfinite(value) and value >= 0


def _index(document, errors, name):
    result = {}
    for row in document.get('tracks', []):
        key = (row.get('track'), row.get('scenario', 'default'), row.get('repeat'))
        if key in result:
            errors.append(f'{name}: duplicate route/repeat {key}')
        result[key] = row
    return result


def compare(baseline, candidate):
    errors = []
    a, b = (_index(d, errors, name) for d, name in
            ((baseline, 'baseline'), (candidate, 'candidate')))
    if not a or set(a) != set(b):
        errors.append('Route/scenario/repeat sets differ or are empty')
    for field in ('viewport', 'warm_frames', 'sample_frames', 'repeats', 'fixed_camera',
                  'snapshot_sha256', 'pose', 'visual_dt', 'profiler_enabled', 'profiler_ui',
                  'engine', 'build', 'adapter', 'adapter_vendor', 'os', 'vsync'):
        if field not in baseline or field not in candidate or baseline[field] != candidate[field]:
            errors.append(f'Missing or different benchmark metadata: {field}')
    for name, document in (('baseline', baseline), ('candidate', candidate)):
        if not document.get('acceptance_protocol') or document.get('window_minimized'):
            errors.append(f'{name}: incomplete 1080p 120/600/3 unminimized protocol')
        if document.get('viewport') != [1920, 1080] or document.get('warm_frames') != 120 or document.get('sample_frames') != 600 or document.get('repeats') != 3 or document.get('vsync') is not False:
            errors.append(f'{name}: required benchmark protocol differs')
        if not document.get('rendering_method') or not document.get('rendering_driver'):
            errors.append(f'{name}: actual renderer identity missing')
    shared = sorted(a.keys() & b.keys())
    routes = []
    for track, scenario in sorted({key[:2] for key in shared}):
        keys = [key for key in shared if key[:2] == (track, scenario)]
        if {key[2] for key in keys} != {0, 1, 2}:
            errors.append(f'Expected repeats 0,1,2 for {(track, scenario)}')
        for key in keys:
            for name, row in (('baseline', a[key]), ('candidate', b[key])):
                if row.get('error') or row.get('completed_samples') != 600 or not row.get('timing_valid'):
                    errors.append(f'{name}: incomplete/invalid repeat {key}')
                if len(row.get('samples', [])) != 600:
                    errors.append(f'{name}: raw sample count differs for {key}')
                if baseline.get('profiler_enabled') and (not row.get('profile_exported') or row.get('profile_samples') != 600):
                    errors.append(f'{name}: aligned profiler capture incomplete for {key}')
            if not a[key].get('pose_bits') or a[key]['pose_bits'] != b[key].get('pose_bits'):
                errors.append(f'Camera/car pose differs or missing for {key}')
            first, second = a[key].get('final_frame', {}), b[key].get('final_frame', {})
            for field in ('packet_digest', 'commands', 'sections', 'zone', 'visible_instances'):
                if field not in first or field not in second or first[field] != second[field] or (field == 'packet_digest' and not first[field]):
                    errors.append(f'Native {field} differs or missing for {key}')
            if a[key].get('scenario_settings') != b[key].get('scenario_settings') or a[key].get('effects') != b[key].get('effects'):
                errors.append(f'Effect settings differ for {key}')
        metrics = {}
        for metric in METRICS:
            metrics[metric] = {}
            for quantile in QUANTILES:
                pairs = [(a[key].get('summary', {}).get(metric, {}).get(quantile),
                          b[key].get('summary', {}).get(metric, {}).get(quantile)) for key in keys]
                if len(pairs) != 3 or not all(_finite(x) and _finite(y) for x, y in pairs):
                    metrics[metric][quantile] = {'available': False}
                    continue
                av, bv = ([pair[index] for pair in pairs] for index in (0, 1))
                med_a, med_b = statistics.median(av), statistics.median(bv)
                noise = max(av) - min(av)
                deltas = [y-x for x, y in pairs]
                entry = {'available': True, 'baseline': med_a, 'candidate': med_b,
                         'baseline_repeat_range_ms': noise,
                         'baseline_repeat_range_percent': 100*noise/med_a if med_a else None,
                         'change_percent': 100*(med_b/med_a-1) if med_a else None,
                         'repeats': [{'repeat': key[2], 'baseline': x, 'candidate': y,
                                      'change_ms': y-x,
                                      'baseline_fps': 1000/x if metric == 'wall_ms' and x else None,
                                      'candidate_fps': 1000/y if metric == 'wall_ms' and y else None}
                                     for key, (x, y) in zip(keys, pairs)],
                         'gain_exceeds_baseline_noise': statistics.median(deltas) < -noise,
                         'persistent_regression': sum(delta > noise for delta in deltas) >= 2}
                if metric == 'wall_ms':
                    entry.update(baseline_fps=1000/med_a if med_a else None,
                                 candidate_fps=1000/med_b if med_b else None)
                metrics[metric][quantile] = entry
        wall = metrics['wall_ms']
        gate = all(wall[q].get('available') for q in QUANTILES)
        gate = gate and wall['median'].get('gain_exceeds_baseline_noise', False)
        gate = gate and not any(wall[q].get('persistent_regression') for q in QUANTILES)
        routes.append({'track': track, 'scenario': scenario, 'metrics': metrics,
                       'performance_gate_passed': bool(gate)})
    return {'task': 'TASK0071', 'performance_passed': not errors and bool(routes) and all(r['performance_gate_passed'] for r in routes),
            'errors': errors, 'routes': routes,
            'baseline_renderer': {key: baseline.get(key) for key in ('rendering_method', 'rendering_driver')},
            'candidate_renderer': {key: candidate.get(key) for key in ('rendering_method', 'rendering_driver')},
            'rule': 'Require 1080p120/600/3; paired wall median gain exceeds baseline repeat max-min range. Reject wall median/P95/P99 increases larger than that quantile baseline range in >=2/3 repeats. All selected routes must pass.',
            'limitations': 'FPS is 1000/frame-time quantile, not averaged instantaneous FPS. CPU scopes overlap. Viewport timers are delayed root-only values, never whole-frame GPU totals. Separate pixel/depth/reflection and original-runtime comparisons remain required.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('baseline', type=Path)
    parser.add_argument('candidate', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = compare(json.loads(args.baseline.read_text(encoding='utf-8')),
                     json.loads(args.candidate.read_text(encoding='utf-8')))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({key: value for key, value in report.items() if key != 'routes'}, indent=2))
    return 0 if report['performance_passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
