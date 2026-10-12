"""Preserve baseline; explicitly skip untrustworthy boundary records for diagnosis."""
import csv
import json
from pathlib import Path

root = Path(__file__).resolve().parents[2] / 'ps2_decomp'
rows = list(csv.DictReader((root / 'ghidra_functions.csv').open(encoding='utf-8')))
bad = [r for r in rows if int(r['Size']) % 4 or int(r['Size']) > 65536]
config = (root / 'hp2.toml').read_text(encoding='utf-8')
config = config.replace('/recompiled"', '/recompiled-triage"')
config = config.replace('skip = []', 'skip = ' + json.dumps([r['Start'] for r in bad]))
(root / 'hp2-triage.toml').write_text(config, encoding='utf-8')
(root / 'triage-exclusions.json').write_text(json.dumps({'status': 'unimplemented placeholders, not verified',
    'reason': 'non-instruction-aligned envelope or >64 KiB diagnostic guard; no inferred behavior', 'functions': bad}, indent=2), encoding='utf-8')
print(f'Explicit diagnostic skip selectors: {len(bad)}')
