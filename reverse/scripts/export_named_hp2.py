"""Export updated map and address-based diagnostic config; preserve all TASK-0006 files."""
from hp2_naming import *

check_elf()
client=Client()
rows=functions(client)
before=json.loads((PRIVATE/'before.json').read_text())['functions']
after=json.loads((PRIVATE/'after.json').read_text())['functions']
if bounds(rows)!=bounds(before) or rows!=after:
    raise ValueError('Live Ghidra differs from saved naming snapshot')
named=ROOT/'ps2_decomp/recompiled-named'
if named.exists():
    raise ValueError('Preserve existing named output; select a new run directory manually')
map_path=PRIVATE/'ghidra_functions_named.csv'
export_map(map_path,rows)
exclusions=json.loads((ROOT/'ps2_decomp/triage-exclusions.json').read_text())['functions']
addresses=[r['Start'] for r in exclusions]
if len(addresses)!=28 or len(set(addresses))!=28:
    raise ValueError('Diagnostic exclusions changed')
config='\n'.join(['[general]',f'input = "{ELF.as_posix()}"',f'ghidra_output = "{map_path.as_posix()}"',
    f'output = "{named.as_posix()}"','single_file_output = false','patch_syscalls = false',
    'patch_cop0 = false','patch_cache = false','stubs = []','skip = '+json.dumps(addresses),
    '[patches]','instructions = []',''])
(PRIVATE/'hp2-named.toml').write_text(config,encoding='utf-8')
dump(PRIVATE/'named-manifest.json',dict(task='TASK-0007',sha256=SHA,
    version='unknown executable revision; boot VER 1.00',map_sha256=hashlib.sha256(map_path.read_bytes()).hexdigest(),
    function_count=len(rows),excluded_addresses=addresses,verification='unverified',
    ghidra_language='r5900:LE:32:default',compiler_spec='default'))
print(f'Exported {len(rows)} names with {len(addresses)} unchanged address exclusions')
