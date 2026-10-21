"""Compare untimed production Sun transport frames; no PS2 verification claim."""
import argparse
import json
from pathlib import Path


def compare(left, right):
    errors = []
    for name, manifest in [('reference', left), ('candidate', right)]:
        if not manifest.get('complete') or len(manifest.get('frames', [])) != 30:
            errors.append(f'{name}: incomplete 30-frame fixture')
    for field in ['engine', 'rendering_method', 'rendering_driver', 'viewport', 'visual_timestep']:
        if left.get(field) != right.get(field):
            errors.append(f'environment mismatch: {field}')
    checks = 0
    for a, b in zip(left.get('frames', []), right.get('frames', [])):
        tag = str(a['index'])
        for key in ['index', 'phase', 'pose', 'sun_enabled']:
            checks += 1
            if a[key] != b[key]:
                errors.append(f'frame {tag}: {key}')
        for key in ['packet_digest', 'commands', 'visible_instances', 'sections', 'sun_quads']:
            checks += 1
            if a['submitted_native'][key] != b['submitted_native'][key]:
                errors.append(f'frame {tag}: native {key}')
        for key in ['colour', 'native_depth', 'final']:
            for field in ['sha256', 'width', 'height', 'format', 'raw_bytes']:
                checks += 1
                if a['captures'][key][field] != b['captures'][key][field]:
                    errors.append(f'frame {tag}: {key} {field}')
    return {'task': 'TASK0072', 'checks': checks, 'passed': not errors,
            'errors': errors, 'scope': '30 port-side production lifecycle frames; exact native digest and raw image parity; no original runtime comparison'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument('reference', type=Path)
    parser.add_argument('candidate', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = compare(json.loads(args.reference.read_text()), json.loads(args.candidate.read_text()))
    rendered = json.dumps(result, indent=2)
    if args.output:
        args.output.write_text(rendered + '\n')
    print(rendered)
    raise SystemExit(0 if result['passed'] else 1)
