"""Strict paired TASK0072 TrackViewer benchmark; raw captures stay in local/."""
import argparse
import json
import statistics
from pathlib import Path


def compare(reference, candidate, optimization='sun', require_profile=True):
    errors = []
    for label, data in [('reference', reference), ('candidate', candidate)]:
        protocol = data.get('driving_resolution_protocol' if data.get('context') == 'driving' else 'native_resolution_protocol')
        if not require_profile:
            protocol = (data.get('viewport') == [2880,1695] and data.get('warm_frames') == 120 and data.get('sample_frames') == 600 and data.get('repeats') == 3 and data.get('build') == 'debug' and data.get('profiler_enabled') is False and data.get('profiler_ui') is False and data.get('rendering_method') == 'forward_plus' and data.get('rendering_driver') == 'vulkan' and data.get('selected_track_ids') == [11] and data.get('route_kind') in ['drive_spin','short_spin'])
        if not protocol:
            errors.append(f'{label}: native Debug/Profile protocol is incomplete')
        if data.get('external_camera_input') is not False:
            errors.append(f'{label}: external camera input was not disabled')
        if len(data.get('tracks', [])) != 3:
            errors.append(f'{label}: exactly three repeats required')
    for key in ['engine', 'adapter', 'rendering_method', 'rendering_driver', 'viewport',
                'context', 'selected_car', 'pose', 'heading_degrees', 'route_kind',
                'warm_frames', 'sample_frames', 'repeats', 'vsync', 'profile', 'visual_dt']:
        if reference.get(key) != candidate.get(key):
            errors.append(f'configuration mismatch: {key}')
    def effective_flags(data):
        flags = dict(data.get('candidate_flags', {}))
        # Early residency captures omitted the CLI field; require independently
        # exported actual renderer state in all three repeats, never assume it.
        for flag, field in [('track-residency-cache','residency_cache'),('track-capture-world-batches','capture_world_batches')]:
            values = [r.get('configuration', {}).get(field) for r in data.get('tracks', [])]
            if len(values)==3 and all(type(v) is bool and v==values[0] for v in values):
                if flag in flags and flags[flag] is not values[0]:
                    errors.append('declared/effective flag mismatch: '+flag)
                flags[flag]=values[0]
        return flags
    a_flags, b_flags = effective_flags(reference), effective_flags(candidate)
    delta = {key for key in a_flags.keys() | b_flags.keys() if a_flags.get(key) != b_flags.get(key)}
    expected = {'sun': {'track-sun-gpu-resident'}, 'compact': {'profile-compact-history'},
                'bulk': {'profile-ui-bulk-upload'}, 'export': {'profile-async-export'},
                'depth': {'track-depth-discovery-cache'}, 'dynamic': {'track-dynamic-plan-cache'}, 'residency': {'track-residency-cache'}, 'capture': {'track-capture-world-batches'}, 'staging': {'track-vehicle-depth-staging'}, 'compiler': set(), 'selection_compiler': set(), 'ui_compiler': set(), 'submit_math': {'track-submit-math'}, 'cpu_submit_repair': {'track-submit-math','track-vehicle-depth-staging','track-depth-discovery-cache'}, 'workers': set(), 'render_thread': set(), 'worker_combined': set(), 'combined': {'track-sun-gpu-resident', 'profile-compact-history', 'profile-ui-bulk-upload'}}[optimization]
    if delta != expected or any(a_flags.get(key) is not False or b_flags.get(key) is not True for key in expected):
        errors.append('unexpected optimization flags or enable direction: ' + optimization)
    threading_modes = {'workers', 'render_thread', 'worker_combined', 'cpu_submit_repair'}
    def actual_threading(data):
        workers = data.get('render_prepare_workers')
        mode = data.get('render_thread_requested')
        if type(workers) is not int or workers not in [0,2,4] or mode not in ['safe','separate']:
            errors.append('missing/invalid requested threading configuration')
        for row in data.get('tracks', []):
            config = row.get('configuration', {})
            if config.get('render_prepare_workers') != workers or config.get('render_thread_actual') != mode:
                errors.append('requested/actual worker or render mode mismatch')
        return workers, mode
    if optimization in threading_modes:
        aw, am = actual_threading(reference); bw, bm = actual_threading(candidate)
        if optimization == 'cpu_submit_repair' and not (aw == 0 and bw in [2,4] and am == bm == 'safe'): errors.append('CPU submission repair requires0/Safe ->2/4/Safe')
        if optimization == 'workers' and not (aw == 0 and bw in [2,4] and am == bm): errors.append('worker comparison requires0->2/4 with unchanged render mode')
        if optimization == 'render_thread' and not (aw == bw and am == 'safe' and bm == 'separate'): errors.append('render comparison requires Safe->Separate with unchanged workers')
        if optimization == 'worker_combined' and not (aw == 0 and am == 'safe' and bw in [2,4] and bm == 'separate'): errors.append('combined comparison requires0/Safe ->2/4/Separate')
    elif (reference.get('render_prepare_workers',0),reference.get('render_thread_requested','safe')) != (candidate.get('render_prepare_workers',0),candidate.get('render_thread_requested','safe')):
        errors.append('unrelated threading configuration change')
    if optimization in {'submit_math','cpu_submit_repair'}:
        for data, expected_math in [(reference,False),(candidate,True)]:
            if any(row.get('configuration',{}).get('submit_math_enabled') is not expected_math for row in data.get('tracks',[])):
                errors.append('declared/effective submission math mismatch')
    compiler_fields={'compiler':'render_host_optimized_debug','selection_compiler':'render_selection_optimized_debug','ui_compiler':'port_ui_optimized_debug'}
    for mode, field in compiler_fields.items():
        if optimization == mode:
            if reference.get(field) is not False or candidate.get(field) is not True:
                errors.append('compiler candidate requires explicit false->true evidence: '+field)
        elif reference.get(field, False) != candidate.get(field, False):
            errors.append('unexpected compiler configuration change: '+field)
    rows, gains, baseline_medians, export_gains, export_maxima = [], [], [], [], []
    for a, b in zip(reference.get('tracks', []), candidate.get('tracks', [])):
        tag = a.get('repeat')
        for label, row in [('reference', a), ('candidate', b)]:
            if not row.get('timing_valid') or row.get('completed_samples') != 600 or len(row.get('samples', [])) != 600 or row.get('error'):
                errors.append(f'{label} repeat {tag}: invalid sampling')
            counters = [sample.get('viewer_process_updates') for sample in row.get('samples', [])]
            if any(not isinstance(value, int) for value in counters) or any(y != x + 1 for x, y in zip(counters, counters[1:])):
                errors.append(f'{label} repeat {tag}: duplicate/missing production viewer update')
        if reference.get('context') == 'driving':
            ta, tb = a.get('driving_timeline', []), b.get('driving_timeline', [])
            if len(ta) != 720 or len(tb) != 720:
                errors.append(f'repeat {tag}: complete warm+measurement driving timeline required')
            else:
                for index, (x, y) in enumerate(zip(ta, tb)):
                    for key in ['continuous_frame', 'step_before', 'step_after', 'step_delta', 'elapsed', 'native_dt', 'input', 'body_bits', 'camera_bits']:
                        if x.get(key) != y.get(key):
                            errors.append(f'repeat {tag} frame {index}: driving {key} mismatch')
                            break
                    if x.get('step_delta') not in (0,1) or y.get('step_delta') not in (0,1) or any(z.get('native_tick_calls',1)!=1 for z in (x,y)):
                        errors.append(f'repeat {tag} frame {index}: invalid physics update count')
        for key in ['repeat', 'pose_bits', 'selected_car', 'context', 'actual_viewport']:
            if a.get(key) != b.get(key):
                errors.append(f'repeat {tag}: {key} mismatch')
        for key in ['packet_digest', 'commands', 'sections', 'visible_instances']:
            if a.get('final_frame', {}).get(key) != b.get('final_frame', {}).get(key):
                errors.append(f'repeat {tag}: native {key} mismatch')
        sa, sb = a.get('summary', {}).get('wall_ms', {}), b.get('summary', {}).get('wall_ms', {})
        if not sa.get('available') or not sb.get('available'):
            errors.append(f'repeat {tag}: wall timing unavailable')
            continue
        baseline_medians.append(sa['median'])
        if optimization == 'export':
            if 'max' not in sa or 'max' not in sb:
                errors.append(f'repeat {tag}: export event maximum unavailable')
            else:
                export_maxima.append(sa['max'])
                export_gains.append(sa['max']-sb['max'])
        gains.append(sa['median'] - sb['median'])
        rows.append({'repeat': tag, 'baseline': {k: sa[k] for k in ['median', 'p95', 'p99']},
                     'candidate': {k: sb[k] for k in ['median', 'p95', 'p99']},
                     'candidate_fps': 1000 / sb['median'], 'candidate_p95_fps': 1000 / sb['p95'],
                     'frame_budget_met': sb['median'] <= 1000 / 60 and sb['p95'] <= 1000 / 60,
                     'tail_regression': sb['p95'] > sa['p95'] or sb['p99'] > sa['p99']})
    noise = max(baseline_medians) - min(baseline_medians) if baseline_medians else None
    gain = statistics.median(gains) if gains else None
    measured_gain = gain is not None and noise is not None and gain > noise and not any(row['tail_regression'] for row in rows)
    if optimization == 'export':
        noise = max(export_maxima)-min(export_maxima) if export_maxima else None
        gain = statistics.median(export_gains) if export_gains else None
        measured_gain = gain is not None and noise is not None and gain > noise and not any(row['tail_regression'] for row in rows)
    return {'task': 'TASK0072', 'optimization': optimization, 'profile_required': require_profile, 'route': candidate.get('route_kind'), 'protocol_valid': not errors,
            'errors': errors, 'repeats': rows, 'baseline_repeat_range_ms': noise, 'baseline_median_range_ms': max(baseline_medians)-min(baseline_medians) if baseline_medians else None,
            'measurement_basis': 'export_frame_maximum' if optimization == 'export' else 'whole_frame_median',
            'paired_gain_ms': gain, 'median_paired_gain_ms': statistics.median(gains) if gains else None, 'performance_enable_gate': not errors and measured_gain,
            'all_repeats_60fps_gate': not errors and len(rows) == 3 and all(row['frame_budget_met'] for row in rows),
            'limits': 'Performance gate only. Require separate native-resolution pixel/depth and lifecycle parity; no PS2 runtime verification. Individual viewport times are never added.'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument('reference', type=Path)
    parser.add_argument('candidate', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--without-profile', action='store_true', help='Validate the separate Profile-off diagnostic protocol')
    parser.add_argument('--optimization', choices=['sun', 'compact', 'bulk', 'export', 'depth', 'dynamic', 'residency', 'capture', 'staging', 'compiler', 'selection_compiler', 'ui_compiler', 'workers', 'render_thread', 'worker_combined', 'submit_math', 'cpu_submit_repair', 'combined'], default='sun')
    args = parser.parse_args()
    result = compare(json.loads(args.reference.read_text()), json.loads(args.candidate.read_text()), args.optimization, require_profile=not args.without_profile)
    text = json.dumps(result, indent=2)
    if args.output:
        args.output.write_text(text + '\n')
    print(text)
    raise SystemExit(0 if result['protocol_valid'] else 1)
