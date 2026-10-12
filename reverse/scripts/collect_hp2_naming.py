"""Read-only current function snapshot, ELF symbol/debug audit and pilot probes."""
from hp2_naming import *

check_elf()
client = Client()
rows = functions(client)
if len(rows) != 7544:
    raise ValueError('Unexpected population; review before mutation')
info = client.call('resources/read', {'uri': f'ghidra://program/{PROGRAM}/info'})
snapshot_path = PRIVATE / 'before.json'
if snapshot_path.exists():
    previous = json.loads(snapshot_path.read_text())
    if previous['sha256'] != SHA or previous['functions'] != rows:
        raise ValueError('Preserve existing before snapshot; use a new task/run directory for changed Ghidra state')
else:
    dump(snapshot_path, dict(sha256=SHA, info=info, functions=rows))
elf_info = inspect_elf()
dump(PRIVATE / 'elf-audit.json', elf_info)
pilots = json.loads((ROOT / 'docs/assessment/PILOT_SELECTION.json').read_text())
addresses = {p['address'] for p in pilots}
# Adjacent small memory/string helpers are candidates, not auto-naming rules.
addresses.update(r['entry_point'] for r in rows if 0x00250000 <= int(r['entry_point'], 16) < 0x00250800)
for address in sorted(addresses):
    path = PRIVATE / 'probes' / (address+'.json')
    if path.exists():
        continue
    probes = {}
    row = next(r for r in rows if r['entry_point'] == address)
    end = min(int(row['end_address'], 16), int(address, 16)+636)
    # Damaged bodies are probed past their one-byte envelope without changing it.
    if end <= int(address, 16):
        end = int(address, 16)+636
    for action, extra in [('decompile', dict(timeout=30)), ('listing', dict(max_lines=160, end_address=f'{end:08x}')),
                          ('references_to', dict(page_size=20))]:
        probes[action] = rpc(client, 'inspect', action=action, address=address, **extra)
    dump(path, probes)
    print(f'Probed {address}', flush=True)
interesting = [s for s in elf_info['strings'] if any(k in s['text'] for k in
    ['SOUND_EndCommand', 'PhysicsCar', 'CarCollision', 'AIDriver', 'libpad', 'scePadPortOpen'])]
for string in interesting[:40]:
    string['references'] = rpc(client, 'inspect', action='references_to', address=string['address'], page_size=30)
dump(PRIVATE / 'string-evidence.json', interesting)
print(f'Snapshot {len(rows)} functions; ELF symbols {len(elf_info["symbols"])}; debug sections {elf_info["debug_sections"]}')
