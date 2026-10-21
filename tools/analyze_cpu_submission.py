"""Offline TASK0072 CPU attribution. Does not launch/import Godot or sum scopes."""
import argparse
import json
import math
import statistics
from pathlib import Path

CPU_FIELDS = ['submit_ms', 'native_ms', 'prepare_ms', 'cost_reflection_submit_ms',
              'cost_main_submit_ms', 'cost_post_submit_ms']
COUNTERS = ['uniform_reads', 'proxy_creations', 'discovery_visits', 'discovery_scans']

def summary(values):
    values = sorted(v for v in values if type(v) in (float, int) and math.isfinite(v))
    if not values:
        return None
    return dict(samples=len(values), median=statistics.median(values),
                p95=values[math.ceil(.95*len(values))-1],
                p99=values[math.ceil(.99*len(values))-1], maximum=values[-1])

def correlation(a, b):
    if len(a)<2:
        return None
    x,y=statistics.mean(a),statistics.mean(b)
    denominator=sum((v-x)**2 for v in a)*sum((v-y)**2 for v in b)
    return sum((u-x)*(v-y) for u,v in zip(a,b))/math.sqrt(denominator) if denominator else None

def analyze(data, extreme_ms=200):
    result={'task':'TASK0072','context':data.get('context'), 'route':data.get('route_kind'),
            'build':data.get('build'),'render_host_optimized_debug':data.get('render_host_optimized_debug'),
            'rows':[], 'limits':'CPU scopes overlap and are never added. Correlation is not causation. Raw percentiles include all frames. No new game performance capture.'}
    for row in data.get('tracks',[]):
        samples=row.get('samples',[])
        valid=[s for s in samples if type(s.get('wall_ms')) in (int,float) and math.isfinite(s['wall_ms'])]
        ordinary=[s for s in valid if s['wall_ms']<extreme_ms]
        cutoff=summary([s['wall_ms'] for s in ordinary])
        tail=[s for s in ordinary if s['wall_ms']>=cutoff['p95']] if cutoff else []
        out={'repeat':row.get('repeat'),'timing_valid':row.get('timing_valid'),
             'wall_ms':summary([s['wall_ms'] for s in valid]),'cpu':{},'counter_deltas':{},
             'extreme_frames_unattributed':[{'frame':s.get('frame'),'wall_ms':s['wall_ms']} for s in valid if s['wall_ms']>=extreme_ms],
             'correlation_exclusion_threshold_ms':extreme_ms}
        for field in CPU_FIELDS:
            paired=[s for s in ordinary if type(s.get(field)) in (int,float) and math.isfinite(s[field])]
            out['cpu'][field]={'all_frames':summary([s.get(field) for s in valid]),
                'wall_tail_median_ms':statistics.median(s[field] for s in tail if type(s.get(field)) in (int,float)) if any(type(s.get(field)) in (int,float) for s in tail) else None,
                'ordinary_wall_correlation':correlation([s['wall_ms'] for s in paired],[s[field] for s in paired])}
        for field in COUNTERS:
            deltas=[];resets=0
            for a,b in zip(samples,samples[1:]):
                a=a.get('host_stats',{}).get('postprocess',{}).get(field)
                b=b.get('host_stats',{}).get('postprocess',{}).get(field)
                if type(a) is int and type(b) is int:
                    if b<a:resets+=1
                    else:deltas.append(b-a)
            out['counter_deltas'][field]={'summary':summary(deltas),'total_observed_increment':sum(deltas),'reset_intervals_excluded':resets}
        result['rows'].append(out)
    return result

if __name__=='__main__':
    parser=argparse.ArgumentParser(__doc__)
    parser.add_argument('inputs',nargs='+',type=Path)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    result=[dict(path=str(path),analysis=analyze(json.loads(path.read_text(encoding='utf-8')))) for path in args.inputs]
    text=json.dumps(result,indent=2)
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(text+'\n',encoding='utf-8')
    else:
        print(text)
