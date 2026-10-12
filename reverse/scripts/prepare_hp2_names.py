"""Screen every function and materialize reviewed names/evidence; no Ghidra mutations."""
from hp2_naming import *

before = json.loads((PRIVATE / 'before.json').read_text())
rows = before['functions']
review = json.loads((ROOT / 'reverse/naming/TASK-0007-reviewed.json').read_text())
check_elf()
client = Client()
now = functions(client)
if bounds(now) != bounds(rows) or any(a['name'] != b['name'] for a,b in zip(now,rows)):
    raise ValueError('Snapshot is stale')
plan = []
extra = json.loads((PRIVATE / 'extra-probes.json').read_text())
for index, item in enumerate(review['renames'], 1):
    row = next(r for r in rows if r['entry_point'] == item['address'])
    item = dict(item, old_name=row['name'], original_identifier='unknown',
        evidence_ids=[f'E-0007-N{index:02}'], signature=row['signature'], calling_convention=row['calling_convention'])
    probe_path = PRIVATE / 'probes' / (item['address']+'.json')
    if not probe_path.exists():
        address = item['address']
        probe = dict(decompile=extra[address], listing=rpc(client, 'inspect', action='listing',
            address=address, end_address=row['end_address'], max_lines=160),
            references_to=rpc(client, 'inspect', action='references_to', address=address, page_size=20))
        dump(probe_path, probe)
    probe = json.loads(probe_path.read_text())
    if probe['decompile']['body_size'] != int(row['end_address'],16)+1-int(row['entry_point'],16):
        raise ValueError('Discontiguous/incorrect candidate body')
    item['probe_sha256'] = hashlib.sha256(probe_path.read_bytes()).hexdigest()
    item['old_symbol'] = rpc(client, 'symbols', action='get', address=item['address'])
    if item['old_symbol']['name'] != item['old_name'] or item['old_symbol']['source'] != 'DEFAULT':
        raise ValueError('Candidate no longer default')
    collision = rpc(client, 'symbols', action='list', name_pattern='^'+item['new_name']+'$', page_size=2)
    if collision.get('data'):
        raise ValueError('Non-function symbol conflict')
    plan.append(item)
validate_plan(rows, plan)
aliases = []
blob = ELF.read_bytes()
text = next(s for s in inspect_elf()['sections'] if s['name']=='.text')
for row in rows:
    for item in plan:
        if row['name'] == 'thunk_FUN_'+item['address']:
            address = int(row['entry_point'],16)
            offset = text['offset']+address-text['address']
            word, delay = struct.unpack_from('<II',blob,offset)
            if word >> 26 != 2 or ((word & 0x3ffffff)<<2) != int(item['address'],16) or delay != 0:
                raise ValueError('Unconfirmed thunk alias')
            aliases.append(dict(address=row['entry_point'], old_name=row['name'],
                new_name=item['new_name'], category='automatic_thunk_alias', confidence='high',
                evidence_ids=item['evidence_ids'], target=item['address'], evidence='ELF direct J target with NOP delay slot',
                old_symbol=rpc(client,'symbols',action='get',address=row['entry_point'])))
dump(PRIVATE / 'rename-plan.json', dict(sha256=SHA, renames=plan, automatic_aliases=aliases))
inventory = {r['elf_address'].removeprefix('0x').lower():r for r in csv.DictReader((ROOT/'docs/assessment/FUNCTION_INVENTORY.csv').open())}
deferred = {r['address']:r for r in review['deferred']}
selected = {r['address']:r for r in plan}
screen = []
for row in rows:
    address = row['entry_point']
    old = inventory[address]
    size = int(row['end_address'],16)+1-int(address,16)
    reason, status = 'No reviewed purpose evidence; keep unknown', 'unresolved'
    candidate, confidence = '', 'unknown'
    if address in selected:
        item = selected[address]
        candidate, confidence, status, reason = item['new_name'], 'high', 'approved_analysis', item['evidence']
    elif size % 4 or size > 65536:
        status, reason = 'boundary_blocked', 'Nonaligned or oversized envelope; no rename'
    elif address in deferred:
        item = deferred[address]
        candidate, confidence, status, reason = item['candidate'], item['confidence'], 'deferred_candidate', item['reason']
    elif row['name'].startswith('thunk_FUN_'):
        status, reason = 'existing_thunk', 'Retain alias unless target is explicitly reviewed'
    elif not default_name(row['name']):
        status, reason = 'preserved_existing_name', 'Existing name is not original-symbol proof; preserve'
    hints = []
    if int(old['syscall_sites']): hints.append('syscall')
    if int(old['cop2_related_sites']): hints.append('COP2/VU')
    if int(old['jalr_sites']): hints.append('indirect_calls')
    if int(old['callers']) >= 50: hints.append('many_callers')
    screen.append(dict(address='0x'+address.upper(), old_name=row['name'], proposed_name=candidate,
        original_identifier='unknown', confidence=confidence, disposition=status, reason=reason,
        hints=';'.join(hints), evidence_ids=';'.join(selected.get(address,{}).get('evidence_ids',[]))))
screen_path = ROOT/'reverse/naming/TASK-0007-screening.csv'
with screen_path.open('w',newline='',encoding='utf-8') as f:
    writer=csv.DictWriter(f,fieldnames=list(screen[0]))
    writer.writeheader(); writer.writerows(screen)
for item in review['deferred']:
    # Capture metadata for preserved custom names and retained candidates.
    item['symbol'] = rpc(client,'symbols',action='get',address=item['address'])
dump(PRIVATE/'deferred-evidence.json',review['deferred'])
summary = {s:sum(r['disposition']==s for r in screen) for s in sorted({r['disposition'] for r in screen})}
dump(PRIVATE/'screening-summary.json',summary)
print(json.dumps(dict(screened=len(screen),approved=len(plan),automatic_aliases=len(aliases),summary=summary),indent=2))
