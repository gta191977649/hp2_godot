"""TASK-0006 artifact checks and tool provenance; emits private code only to ignored folder."""
import csv
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from ghidra import Client

out = ROOT / 'ps2_decomp'
generated = out / 'recompiled-triage'
rows = list(csv.DictReader((out / 'ghidra_functions.csv').open(encoding='utf-8')))
assert len(rows) == 7544 == len({r['Start'] for r in rows})
assert not any(',' in r['Name'] or '"' in r['Name'] for r in rows), 'upstream CSV parser cannot handle quoted names'
cpp = list(generated.glob('*.cpp'))
assert len(cpp) == 7545
assert (generated / 'register_functions.cpp').exists()
assert (generated / 'ps2_recompiled_stubs.h').exists()
excluded = json.loads((out / 'triage-exclusions.json').read_text())['functions']
todo = [f.name for f in cpp if 'TODO_NAMED(' in f.read_text(encoding='utf-8')]
assert len(todo) == len(excluded) == 28
log = (out / 'recompile-triage.log').read_text()
assert log.rstrip().endswith('Recompilation completed successfully')
assert 'Error generating' not in log
deps = {}
for name in ['elfio', 'fmt', 'libdwarf', 'rabbitizer', 'toml11']:
    deps[name] = subprocess.check_output(['git', '-C', str(ROOT / 'local/ps2recomp-build/_deps' / (name+'-src')), 'rev-parse', 'HEAD'], text=True).strip()
source = Path('D:/dev/PS2Recomp-0.4')
fingerprints = {str(p.relative_to(source)).replace('\\', '/'): hashlib.sha256(p.read_bytes()).hexdigest()
    for p in source.rglob('*') if p.is_file() and p.suffix in {'.cpp', '.h', '.java', '.txt', '.md', '.toml'}}
(out / 'tool-source-sha256.json').write_text(json.dumps(fingerprints, indent=2), encoding='utf-8')
client = Client()
for addr in ['0x00100008', '0x00250340']:
    result = client.call('tools/call', {'name': 'inspect', 'arguments': {'file_name': 'SLUS_203.62', 'action': 'decompile', 'address': addr}})
    if result.get('isError'):
        raise RuntimeError(result)
    (out / ('ghidra-decompile-'+addr+'.json')).write_text(json.dumps(result, indent=2), encoding='utf-8')
    (out / ('ghidra-decompile-'+addr+'.c')).write_text(
        '// Unverified Ghidra pseudocode; ELF SHA-256 ' + hashlib.sha256(Path('D:/ps2_game/SLUS_203.62').read_bytes()).hexdigest() + '\n' +
        '\n'.join(c['text'] for c in result.get('content', []) if c.get('type') == 'text'), encoding='utf-8')
summary = {'task': 'TASK-0006', 'map_functions': len(rows), 'cpp_files': len(cpp),
    'function_cpp_files': len(cpp)-1, 'unimplemented_placeholder_files': len(todo),
    'translated_function_files': len(cpp)-1-len(todo), 'generated_bytes': sum(p.stat().st_size for p in generated.iterdir() if p.is_file()),
    'dependencies': deps, 'baseline': 'terminated near 6 GB working set; 1732 function files plus header, incomplete',
    'triage': 'exit 0; 28 explicit unresolved placeholders', 'behavior': 'not compared, not verified',
    'ghidra_pseudocode_samples': ['0x00100008', '0x00250340'],
    'syntax_probes_passed': ['entry_0x100008.cpp', 'sub_00250340_0x250340.cpp',
        'register_functions.cpp', 'HP2_DemoCameraCoordinator_Update_0x19a4b8.cpp', 'sub_0020D8C0_0x20d8c0.cpp'],
    'full_build_and_original_comparison': 'not performed'}
(out / 'validation.json').write_text(json.dumps(summary, indent=2), encoding='utf-8')
print(json.dumps(summary, indent=2))
