"""Compare deterministic host runs; no original-console equivalence claim."""
import argparse
import json
import statistics
from pathlib import Path


def compare(baseline, candidate):
    def index(document):
        return {(r['track'], r['repeat']): r for r in document['tracks']}
    a, b = index(baseline), index(candidate)
    errors = []
    if set(a) != set(b):
        errors.append('Route/repeat sets differ')
    rows = []
    for track in sorted({k[0] for k in a.keys() & b.keys()}):
        row = {'track': track, 'metrics': {}}
        keys = sorted(k for k in a.keys() & b.keys() if k[0] == track)
        if {k[1] for k in keys} != {0, 1, 2}:
            errors.append(f'Expected exactly repeats 0,1,2 for track {track}')
        for key in keys:
            for source in (a[key], b[key]):
                if source.get('error') or source.get('completed_samples') != 600:
                    errors.append(f'Incomplete route {key}: {source.get("error", "samples != 600")}')
            if not a[key].get('pose_bits') or a[key].get('pose_bits') != b[key].get('pose_bits'):
                errors.append(f'Camera/car pose differs or is missing: {key}')
            first, second = a[key].get('final_frame', {}), b[key].get('final_frame', {})
            if not first.get('packet_digest') or first.get('packet_digest') != second.get('packet_digest'):
                errors.append(f'Native packet differs or is missing: {key}')
            for field in ('commands', 'sections', 'zone', 'visible_instances'):
                if field not in first or first[field] != second.get(field):
                    errors.append(f'Native {field} differs or is missing: {key}')
        for metric in ('wall_ms', 'prepare_ms', 'submit_ms', 'native_ms', 'draw_calls', 'video_memory_bytes'):
            row['metrics'][metric] = {}
            for quantile in ('median', 'p95', 'p99'):
                pairs = [(a[k]['summary'][metric].get(quantile), b[k]['summary'][metric].get(quantile)) for k in keys if 'summary' in a[k] and 'summary' in b[k]]
                pairs = [(x, y) for x, y in pairs if x is not None and y is not None and x > 0]
                if len(pairs) != 3:
                    row['metrics'][metric][quantile] = {'available': False}
                    continue
                ratios = [y / x for x, y in pairs]
                row['metrics'][metric][quantile] = dict(available=True, baseline=statistics.median(x for x, y in pairs), candidate=statistics.median(y for x, y in pairs), change_percent=(statistics.median(ratios)-1)*100, repeat_changes_percent=[(r-1)*100 for r in ratios], persistent_regression=sum(r>1.05 for r in ratios)>=2)
        rows.append(row)
    regressions = [dict(track=r['track'], quantile=q, **r['metrics']['wall_ms'][q]) for r in rows for q in ('median','p95','p99') if r['metrics']['wall_ms'][q].get('persistent_regression')]
    gains = {}
    for metric in ('wall_ms','prepare_ms','submit_ms'):
        values = [r['metrics'][metric]['median']['change_percent'] for r in rows if r['metrics'][metric]['median'].get('available')]
        gains[metric] = -statistics.median(values) if values else None
    protocol = all(d.get('acceptance_protocol') for d in (baseline,candidate)) and len(rows)==30 and len(a)==90 and len(b)==90
    passed = protocol and not errors and not regressions and any(v is not None and v>=5 for v in gains.values())
    return dict(task='TASK0055', passed=passed, acceptance_protocol=protocol, errors=errors, median_route_improvement_percent=gains, persistent_wall_regressions=regressions, routes=rows, rule='Three repeats per route; persistent regression means >5% in at least two paired repeats. Require >=5% median-route improvement in wall/prepare/submit and no persistent wall median/P95/P99 regression. Full 30-route 120/600/3 protocol required.')


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('baseline',type=Path)
    parser.add_argument('candidate',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    result=compare(json.loads(args.baseline.read_text()),json.loads(args.candidate.read_text()))
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='routes'},indent=2))
    raise SystemExit(0 if result['passed'] else 1)
