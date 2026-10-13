"""TASK-0010 metadata/evidence checks and optional Godot smoke ledger."""
from hp2_naming import *
import argparse
import subprocess

def catalog_check():
    with (ROOT/'reverse/main_loop/call_sites.csv').open(newline='',encoding='utf-8') as f:rows=list(csv.DictReader(f))
    pairs={(int(r['call_pc'],16),int(r['target'],16)) for r in rows}
    assert len(rows)==len(pairs)==66
    main=[r for r in rows if r['caller']=='0012f378']
    assert len(main)==61 and len({r['target'] for r in main})==39
    source=(ROOT/'src/game_core/port/engine/call_sites.inc').read_text(encoding='utf-8')
    generated=[]
    for m in re.finditer(r'\{0x([0-9a-f]+), 0x([0-9a-f]+), 0x([0-9a-f]+), Module::(\w+), Phase::(\w+), "([^"]+)", "([^"]+)", "([^"]+)"\}',source):
        generated.append(m.groups())
    assert generated==[(r['caller'],r['call_pc'],r['target'],r['module'],r['phase'],r['guard'],r['evidence'],r['confidence']) for r in rows]
    with (ROOT/'docs/assessment/CALL_EDGES.csv').open(newline='',encoding='utf-8') as f:
        edges=list(csv.reader(f))
    historical={f'{int(r[1],16):08x}' for r in edges if r and r[0].lower()=='0x0012f378'}
    assert historical=={r['target'] for r in main}
    with (ROOT/'reverse/main_loop/world_connections.csv').open(newline='',encoding='utf-8') as f:world=list(csv.DictReader(f))
    assert len(world)==19 and len({r['call_pc'] for r in world})==19
    world_source=(ROOT/'src/game_core/port/engine/world_connections.cpp').read_text(encoding='utf-8')
    world_pairs={(int(m[1],16),int(m[2],16),int(m[3],16)) for m in re.finditer(r'\{0x([0-9a-f]+),0x([0-9a-f]+),(0x[0-9a-f]+|0),',world_source)}
    assert world_pairs=={(int(r['caller'],16),int(r['call_pc'],16),int(r['target'],16)) for r in world}
    with (ROOT/'reverse/main_loop/branches.csv').open(newline='',encoding='utf-8') as f:branches=list(csv.DictReader(f))
    assert len(branches)==12
    return rows,world

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--metadata-only',action='store_true');args=parser.parse_args()
    rows,world=catalog_check()
    if args.metadata_only:
        print('MAIN_LOOP_CATALOG_OK: 66 descriptors (61 loop/5 startup), 39 loop targets, 19 nested links, 12 guards');return
    out=ROOT/'ps2_decomp/main-loop';metadata=json.loads((out/'metadata.json').read_text(encoding='utf-8'))
    assert check_elf()==metadata['elf_sha256']
    client=Client()
    for row in metadata['functions']:
        assert rpc(client,'functions',action='get',address=row['entry_point'])==row
    for filename,digest in metadata['artifacts'].items():
        assert hashlib.sha256((out/filename).read_bytes()).hexdigest()==digest
    calls=json.loads((out/'main-sites.json').read_text(encoding='utf-8'))
    assert [(r['call_pc'],r['target']) for r in rows if r['caller']=='0012f378']==[(r['call_pc'],r['target']) for r in calls]
    # Validate every direct/indirect/tail connection PC against live captured listing.
    for row in world:
        listing=json.loads((out/(row['caller']+'.json')).read_text(encoding='utf-8'))['listing']['data']
        line=next(line for line in listing.splitlines() if line.startswith(row['call_pc']+' '))
        if int(row['target'],16):
            assert re.search(r'\b(?:jal|j) 0x'+row['target'],line),line
        else:assert 'jalr ' in line,line
    # File-backed table words corroborate conditional class-specific targets.
    elf=inspect_elf();binary=ELF.read_bytes()
    for address,expected in [(0x2b59bc,0x1a2110),(0x2b5a1c,0x1a1030),(0x2b87ec,0x1d81f0)]:
        section=next(s for s in elf['sections'] if s['address']<=address<section_end(s))
        assert struct.unpack_from('<I',binary,section['offset']+address-section['address'])[0]==expected
    godot=ROOT/'local/tools/godot-4.3/Godot_v4.3-stable_win64_console.exe'
    commands=[['ctest','--preset','native-debug'],['ctest','--preset','godot-debug'],
        [str(godot),'--headless','--path',str(ROOT/'godot'),'res://scenes/main_loop_observer.tscn','--quit-after','120','--','--observer-smoke'],
        [str(godot),'--headless','--path',str(ROOT/'godot'),'--','--smoke-test']]
    results=[]
    for command in commands:
        run=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=60)
        result=dict(command=command,exit_code=run.returncode,stdout=run.stdout,stderr=run.stderr)
        results.append(result)
        combined=run.stdout+'\n'+run.stderr
        if run.returncode or 'ERROR:' in combined or 'SCRIPT ERROR' in combined:
            dump(out/'validation-failed.json',dict(results=results));raise ValueError(combined)
        if '--observer-smoke' in command:assert 'MAIN_LOOP_OBSERVER_SMOKE_OK' in combined
        if '--smoke-test' in command:assert 'PORT_SMOKE_OK: 42' in combined
    dump(out/'validation.json',dict(task='TASK-0010',date='2026-10-12 Australia/Sydney',elf_sha256=check_elf(),
        checked_live_functions=len(metadata['functions']),live_metadata_unchanged=True,
        main_call_sites=61,main_targets=39,startup_sites=5,nested_connections=19,guards=12,results=results,
        original_behavior_verification='not performed',rea_skill_version='37; skill workflow applied; no REA MCP server registered',
        godot_version=subprocess.run([str(godot),'--version'],capture_output=True,text=True).stdout.strip(),
        compiler=subprocess.run(['g++','--version'],capture_output=True,text=True).stdout.splitlines()[0],python=sys.version,
        evidence_sha256=hashlib.sha256((out/'metadata.json').read_bytes()).hexdigest(),
        public_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT/'reverse/main_loop').glob('*.csv')}))
    print('MAIN_LOOP_VALIDATION_OK: evidence/live metadata unchanged; native and bridge suites, observer and prior demo passed')

def section_end(section):return section['address']+section['size']

if __name__=='__main__':main()
