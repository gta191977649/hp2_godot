"""Analyze port-side deterministic TrackViewer scenario runs without inferred GPU time.

Example: py -3 reverse/scripts/analyze_track_costs.py local/task0063/matrix.json
         --output local/task0063/costs.json --markdown local/task0063/costs.md
"""
import argparse
import itertools
import json
import math
import statistics
from collections import defaultdict
from pathlib import Path

METRICS = ('wall_ms', 'prepare_ms', 'native_ms', 'submit_ms',
           'viewport_cpu_ms', 'viewport_gpu_ms', 'draw_calls', 'triangles')


def finite(value):
    return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value)


def metric(row, name, quantile='median'):
    value = row.get('summary', {}).get(name, {}).get(quantile)
    return value if finite(value) else None


def median(values):
    values = [value for value in values if finite(value)]
    return statistics.median(values) if values else None


def analyze(documents):
    groups = defaultdict(list)
    errors = []
    invalid_groups = set()
    for origin, document in documents:
        key = (tuple(document.get('viewport', [])), document.get('rendering_method', 'unknown'),
               document.get('rendering_driver', 'unknown'), document.get('adapter', 'unknown'),
               document.get('profile', 'baseline'), document.get('mode', ''),
               json.dumps(document.get('overrides', {}), sort_keys=True), document.get('build', 'unknown'), document.get('pose', 'start'), json.dumps(document.get('candidate_flags', {}), sort_keys=True))
        groups.setdefault(key, [])
        if document.get('cancelled_by_user', False):
            errors.append(f'Cancelled by user: {origin}; timing is incomplete and cannot satisfy acceptance')
            invalid_groups.add(key)
        if 'scenarios' in document:
            scenarios = document['scenarios']
            selected_tracks = document.get('selected_track_ids', document.get('expected_track_ids'))
            if selected_tracks is None:
                selected_tracks = sorted({row['track'] for row in document.get('tracks', [])})
            repeats = document.get('repeats', 3)
            if not scenarios or not selected_tracks or not isinstance(repeats, int) or repeats < 1:
                errors.append(f'Invalid or empty declared route/scenario/repeat matrix in {origin}')
                invalid_groups.add(key)
            else:
                expected = set(itertools.product(selected_tracks, scenarios, range(repeats)))
                actual = {(row['track'], row.get('scenario', 'default'), row['repeat']) for row in document.get('tracks', [])}
                missing, unexpected = expected-actual, actual-expected
                if missing:
                    errors.append(f'Missing declared route/scenario/repeat entries in {origin}: {sorted(missing)}')
                    invalid_groups.add(key)
                if unexpected:
                    errors.append(f'Unexpected route/scenario/repeat entries in {origin}: {sorted(unexpected)}')
                    invalid_groups.add(key)
        for row in document.get('tracks', []):
            groups[key].append((origin, document, row))
    results = []
    for key, entries in sorted(groups.items()):
        metrics_names = METRICS+tuple(sorted({name for _, _, row in entries for name in row.get("summary", {}) if name.startswith("cost_")}))
        indexed = {}
        route_rows = defaultdict(list)
        complete = key not in invalid_groups
        for origin, document, row in entries:
            scenario = row.get('scenario', 'default')
            identity = (row['track'], scenario, row['repeat'])
            if identity in indexed:
                errors.append(f'Duplicate route/scenario/repeat {identity} in {origin}')
                complete = False
                continue
            indexed[identity] = row
            expected = document.get('sample_frames', 600)
            valid = not row.get('error') and row.get('timing_valid', True) and not row.get('window_minimized', False) and not document.get('window_minimized', False) and row.get('completed_samples') == expected
            if not valid:
                errors.append(f'Incomplete {identity} in {origin}: {row.get("error", "sample count mismatch")}')
            complete = complete and valid
            route_rows[(row['track'], scenario)].append(row)
        routes = []
        for (track, scenario), rows in sorted(route_rows.items()):
            metrics = {name: {quantile: median(metric(row, name, quantile) for row in rows)
                              for quantile in ('median', 'p95', 'p99', 'max')} for name in metrics_names}
            worst_wall = max((metric(row, 'wall_ms', 'p95') for row in rows if metric(row, 'wall_ms', 'p95') is not None), default=None)
            worst_draws = max((metric(row, 'draw_calls', 'max') for row in rows if metric(row, 'draw_calls', 'max') is not None), default=None)
            samples = [sample for row in rows for sample in row.get('samples', [])]
            routes.append(dict(track=track, scenario=scenario, repeats=len(rows), metrics=metrics,
                               worst_repeat_wall_p95_ms=worst_wall, max_draw_calls=worst_draws,
                               meets_frame_target=worst_wall is not None and worst_wall <= 16.67,
                               meets_draw_budget=worst_draws is not None and worst_draws < 1000,
                               gpu_timer_valid_samples=sum(sample.get('gpu_timer_valid') is True for sample in samples),
                               sampled_frames=len(samples),
                               particle_emitter_count=max((row['particle_emitter_count'] for row in rows if finite(row.get('particle_emitter_count'))), default=None),
                               particle_definition_count=max((row['particle_definition_count'] for row in rows if finite(row.get('particle_definition_count'))), default=None)))
        deltas = []
        interactions = []
        for track in sorted({identity[0] for identity in indexed}):
            repeats = sorted({identity[2] for identity in indexed if identity[0] == track})
            def difference(first, second):
                # Difference is enabled minus disabled; positive means added cost.
                pairs = [(indexed.get((track, first, repeat)), indexed.get((track, second, repeat))) for repeat in repeats]
                pairs = [(a, b) for a, b in pairs if a and b and not a.get('error') and not b.get('error')]
                return {name: median(metric(a, name)-metric(b, name) for a, b in pairs
                                     if metric(a, name) is not None and metric(b, name) is not None) for name in metrics_names}, len(pairs)
            for bits in itertools.product((0, 1), repeat=3):
                on = 'p%df%da%d' % bits
                for axis, feature in enumerate(('props', 'fog', 'particles')):
                    if not bits[axis]:
                        continue
                    off_bits = list(bits)
                    off_bits[axis] = 0
                    off = 'p%df%da%d' % tuple(off_bits)
                    costs, count = difference(on, off)
                    if count:
                        deltas.append(dict(track=track, feature=feature, enabled=on, disabled=off, paired_repeats=count, added_cost=costs))
            for feature in ('sun', 'environment', 'dof'):
                baseline = 'p1f1a1' if any((track, 'p1f1a1', repeat) in indexed for repeat in repeats) else 'default'
                costs, count = difference(baseline, feature+'_off')
                if count:
                    deltas.append(dict(track=track, feature=feature, enabled=baseline, disabled=feature+'_off', paired_repeats=count, added_cost=costs))
            for axes in itertools.combinations(range(3), 2):
                fixed_axis = ({0, 1, 2}-set(axes)).pop()
                for fixed in (0, 1):
                    names = []
                    for a, b in ((1, 1), (1, 0), (0, 1), (0, 0)):
                        bits = [0, 0, 0]
                        bits[axes[0]], bits[axes[1]], bits[fixed_axis] = a, b, fixed
                        names.append('p%df%da%d' % tuple(bits))
                    values = defaultdict(list)
                    count = 0
                    for repeat in repeats:
                        rows = [indexed.get((track, name, repeat)) for name in names]
                        if not all(row and not row.get('error') for row in rows):
                            continue
                        count += 1
                        for name in metrics_names:
                            numbers = [metric(row, name) for row in rows]
                            if all(value is not None for value in numbers):
                                values[name].append(numbers[0]-numbers[1]-numbers[2]+numbers[3])
                    if count:
                        interactions.append(dict(track=track, axes=[('props', 'fog', 'particles')[axis] for axis in axes], fixed_feature=('props', 'fog', 'particles')[fixed_axis], fixed_value=fixed, paired_repeats=count, extra_joint_cost={name: median(values[name]) for name in metrics_names}))
        full = complete and bool(entries) and all({row['repeat'] for row in rows} == {0, 1, 2} for rows in route_rows.values()) and all(document.get('warm_frames') == 120 and document.get('sample_frames') == 600 and document.get('repeats') == 3 for _, document, _ in entries)
        original = [row for row in routes if row['scenario'] in ('default', 'p1f1a1')]
        targets = complete and bool(original) and all(row['meets_frame_target'] and row['meets_draw_budget'] for row in original)
        results.append(dict(viewport=key[0], rendering_method=key[1], rendering_driver=key[2], adapter=key[3], profile=key[4], mode=key[5], overrides=json.loads(key[6]), build=key[7], pose=key[8], candidate_flags=json.loads(key[9]), complete=complete, standard_sampling_protocol=full, all_30_tracks=len({row['track'] for row in original}) == 30, original_effect_targets_met=targets, routes=routes, feature_costs=sorted(deltas, key=lambda row: row['added_cost'].get('wall_ms') or -math.inf, reverse=True), interactions=interactions))
    return dict(measurement='Port-side wall time and explicit GPU timers only; disabled effects diagnose cost and never satisfy original-effect acceptance. Positive deltas mean added cost. Repeat statistics are aggregated by median; target gate uses worst repeat P95 and maximum draws.', errors=errors, groups=results)


def markdown(report):
    def fmt(value):
        return 'unavailable' if value is None else f'{value:.3f}'
    lines = ['# TrackViewer cost analysis', '', report['measurement'], '']
    for group in report['groups']:
        lines += [f"## {group['rendering_method']} / {group['viewport']} / {group['build']} / {group['pose']}", '',
                  "Candidate flags: "+json.dumps(group['candidate_flags'], sort_keys=True), '',
                  '| Track | Scenario | Wall median ms | Wall P95 ms | Wall P99 ms | Max draws | Targets |',
                  '|---|---|---:|---:|---:|---:|---|']
        for row in group['routes']:
            wall = row['metrics']['wall_ms']
            passed = row['meets_frame_target'] and row['meets_draw_budget']
            lines.append(f"| {row['track']} | {row['scenario']} | {fmt(wall['median'])} | {fmt(wall['p95'])} | {fmt(wall['p99'])} | {fmt(row['max_draw_calls'])} | {'pass' if passed else 'miss/unavailable'} |")
        lines += ['', '| Track | Feature | Enabled − disabled | Added wall ms | Added submit ms | Added draws |', '|---|---|---|---:|---:|---:|']
        for row in group['feature_costs']:
            cost = row['added_cost']
            lines.append(f"| {row['track']} | {row['feature']} | {row['enabled']} − {row['disabled']} | {fmt(cost['wall_ms'])} | {fmt(cost['submit_ms'])} | {fmt(cost['draw_calls'])} |")
        lines += ['', 'Interaction terms and explicit GPU timer availability are retained in the JSON report.', '']
    if report['errors']:
        lines += ['## Incomplete inputs', '']+[f'- {error}' for error in report['errors']]
    return '\n'.join(lines)+'\n'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('inputs', type=Path, nargs='+')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--markdown', type=Path)
    args = parser.parse_args()
    report = analyze([(str(path), json.loads(path.read_text(encoding='utf-8-sig'))) for path in args.inputs])
    data = json.dumps(report, indent=2)+'\n'
    if args.output:
        args.output.write_text(data, encoding='utf-8')
    else:
        print(data, end='')
    if args.markdown:
        args.markdown.write_text(markdown(report), encoding='utf-8')
    raise SystemExit(1 if report['errors'] else 0)
