"""TASK-0006: read-only Ghidra boundary export; private outputs only."""
import csv
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from ghidra import Client

EXPECTED = '7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2'
elf = Path('D:/ps2_game/SLUS_203.62')
assert hashlib.sha256(elf.read_bytes()).hexdigest() == EXPECTED
out = ROOT / 'ps2_decomp'
out.mkdir(exist_ok=True)
client = Client()
info = client.call('resources/read', {'uri': 'ghidra://program/SLUS_203.62/info'})
rows, cursor = [], None
while True:
    args = {'file_name': 'SLUS_203.62', 'action': 'list', 'page_size': 50}
    if cursor:
        args['cursor'] = cursor
    result = client.call('tools/call', {'name': 'functions', 'arguments': args})
    if result.get('isError'):
        raise RuntimeError(result)
    data = result.get('structuredContent')
    if data is None:
        data = json.loads(result['content'][0]['text'])
    rows.extend(data['data'])
    cursor = data.get('next_cursor')
    if not cursor:
        break
assert len(rows) == len({r['entry_point'] for r in rows})
with (out / 'ghidra_functions.csv').open('w', newline='', encoding='utf-8') as f:
    writer = csv.writer(f)
    writer.writerow(['Name', 'Start', 'End', 'Size'])
    for r in rows:
        start = int(r['entry_point'], 16)
        end = int(r['end_address'], 16) + 1
        if end <= start:
            raise ValueError(r)
        writer.writerow([r['name'], f'0x{start:08X}', f'0x{end:08X}', end-start])
(out / 'ghidra_snapshot.json').write_text(json.dumps({'info': info, 'functions': rows}, indent=2), encoding='utf-8')
config = '\n'.join(['[general]', f'input = "{elf.as_posix()}"',
    f'ghidra_output = "{(out / "ghidra_functions.csv").as_posix()}"',
    f'output = "{(out / "recompiled").as_posix()}"',
    'single_file_output = false', 'patch_syscalls = false',
    'patch_cop0 = false', 'patch_cache = false', 'stubs = []', 'skip = []',
    '[patches]', 'instructions = []', ''])
(out / 'hp2.toml').write_text(config, encoding='utf-8')
manifest = {'task': 'TASK-0006', 'elf_sha256': EXPECTED, 'version': 'unknown executable revision; boot VER 1.00',
    'function_count': len(rows), 'map_semantics': 'entry to max body address + 1, contiguous envelope; discontiguous body gaps unresolved',
    'synthetic_callable_labels': 'not exported by MCP function list', 'verification': 'unverified',
    'exporter': 'read-only MCP functions/list; no Ghidra mutations',
    'map_sha256': hashlib.sha256((out / 'ghidra_functions.csv').read_bytes()).hexdigest()}
(out / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
print(json.dumps(manifest))
