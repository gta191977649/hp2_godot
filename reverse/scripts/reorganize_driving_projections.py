"""TASK-0042 one-time, audited partition of host projections by proven Debug TU.

This migrator is not a game implementation generator. Unknown owners stay in a
port directory; constructors keep address identifiers. Raw inputs stay local.
"""
import csv
import hashlib
import json
from pathlib import Path
import re
from collections import Counter, defaultdict
from generate_class_templates import METHOD, mask_comments_strings, closing_brace
from recover_debug_source_ownership import extract

ROOT = Path(__file__).resolve().parents[2]
CORE = ROOT / 'src/game_core'


def rename_code_tokens(text, rename):
    """Rename identifier tokens only; preserve comments, literals and evidence strings."""
    masked = mask_comments_strings(text)
    edits = [(m.start(), m.end(), rename[m[0]]) for m in re.finditer(r'\bSub_[0-9A-F]{8}\b', masked) if m[0] in rename]
    for start, end, name in reversed(edits):
        text = text[:start] + name + text[end:]
    return text


def cpp_tokens_sha256(text):
    """Fingerprint C++ tokens, retaining literal values while ignoring comments/space."""
    pattern = r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_]\w*|(?:\d[\w.]*|\.\d[\w.]*)|[^\s]'
    tokens = [match[0] for match in re.finditer(pattern, text) if not match[0].startswith(('//', '/*'))]
    return hashlib.sha256(json.dumps(tokens, separators=(',', ':')).encode()).hexdigest()


def normalize_include_line(line):
    return re.sub(r'(#include\s+")port/', r'\1game_core/port/', line)


def validate_unparsed_statements(text, parsed_functions, namespaces):
    """Anonymous chunks are copied whole; no other namespace-level statement may vanish."""
    masked = list(mask_comments_strings(text))
    ranges = [(f['start'], f['end']) for f in parsed_functions]
    ranges += [(n[0], n[2] + 1) for n in namespaces if not n[3]]
    ranges += [(n[0], n[1]) for n in namespaces if n[3]]
    ranges += [(n[2], n[2] + 1) for n in namespaces if n[3]]
    for start, end in ranges:
        masked[start:end] = ' ' * (end - start)
    remaining = re.sub(r'^\s*#include[^\n]*', '', ''.join(masked), flags=re.M)
    if remaining.strip():
        raise ValueError('Unparsed namespace-level statements require an explicit preserved partition: ' + remaining.strip()[:160])


def functions(text):
    masked = mask_comments_strings(text)
    result = []
    namespaces = []
    for n in re.finditer(r'\bnamespace\s*([\w:]*)\s*\{', masked):
        namespaces.append((n.start(), n.end(), closing_brace(masked, n.end()-1), n[1]))
    for m in METHOD.finditer(masked):
        if any(f['start'] <= m.start() < f['end'] for f in result):
            continue
        end = closing_brace(masked, m.end()-1)+1
        name = re.search(r'([\w:]+)\s*$', m[1].split('(', 1)[0])[1]
        parents = [n for n in namespaces if n[0] < m.start() < n[2]]
        result.append(dict(start=m.start(), end=end, opening=m.end()-1,
            name=name, scope='::'.join(n[3] for n in parents if n[3]),
            anonymous=any(not n[3] for n in parents), signature=text[m.start():m.end()-1].strip()))
    return result, namespaces


def main():
    paths = sorted(CORE.glob('Sub_*.cpp'))
    if len(paths) != 14:
        raise ValueError('One-time migration requires all 14 original projection CPPs')
    snapshots = {p: p.read_text(encoding='utf-8') for p in paths}
    local = ROOT / 'local/task0042/projection-migration'
    local.mkdir(parents=True, exist_ok=True)
    for p, text in snapshots.items():
        (local / p.name).write_text(text, encoding='utf-8')
    mappings = {r['retail_address']: r for r in csv.DictReader((ROOT / 'reverse/naming/task0041-adopted-retail-names.csv').open())}
    for filename in ('candidate', 'unresolved'):
        for r in json.loads((ROOT / f'reverse/naming/task0042-{filename}-bridges.json').read_text())['bridges']:
            if r['status'].startswith('supported'):
                mappings[r['retail_address']] = r
    original = {r['debug_address'] + '|' + r['mangled_name']: r for r in extract(
        'D:/ps2_game/GAME/DEBUG_VER/NFSHP2.ELF', 'D:/ps2_game/GAME/DEBUG_VER/NFSHP2.MAP')}
    ownership = {}
    for address, row in mappings.items():
        evidence = original.get(row['debug_address']+'|'+row['mangled_name'])
        if evidence:
            ownership[address] = dict(evidence, retail_address=address,
                reference_name=row['reference_name'], correspondence_evidence=row['evidence_id'])
    prior = {r['source_identifier']: r['elf_address'] for r in csv.DictReader(
        (ROOT/'reverse/naming/task0041-adopted-projection-names.csv').open())}
    parsed = {p: functions(t) for p, t in snapshots.items()}
    for path, (parsed_functions, namespaces) in parsed.items():
        validate_unparsed_statements(snapshots[path], parsed_functions, namespaces)
    entries = []
    for p, (fs, _) in parsed.items():
        for f in fs:
            match = re.search(r'Sub_([0-9A-F]{8})$', f['name'])
            f['address'] = '0x'+match[1] if match else prior.get(f['name'])
            if f['address']:
                entries.append(f)
    # A token replacement must be unambiguous across all host namespaces and
    # overload carriers. Refuse different original entries with one short name.
    names = defaultdict(set)
    proposals = {}
    for f in entries:
        row = mappings.get(f['address'])
        if not row:
            continue
        match = re.fullmatch(r'(?:(\w+)::)?([A-Za-z_]\w*)\(.*', row['reference_name'])
        if not match or match[1] == match[2]:
            continue
        proposals[f['address']] = match[2]
        names[match[2]].add(f['address'])
    # These nine independently supported global entries have different projected
    # parameter signatures. Keep their exact global overload spellings; do not
    # extend this exception to unrelated same-short-name class methods.
    overloaded_globals = {
        'bRandom': {'0x0025A3D0', '0x0025A410', '0x0025A4A8'},
        'bMulMatrix': {'0x0025AED0', '0x0025AE18'},
        'bSin': {'0x0025A540', '0x0025A5E0'},
        'bFixATan': {'0x0025AA78', '0x0025AB50'},
    }
    rename = {'Sub_'+a[2:]: n for a, n in proposals.items() if len(names[n]) == 1 or
              (a in overloaded_globals.get(n, set()) and mappings[a]['reference_name'].startswith(n+'('))}
    # Do not collide with unrelated explicit host utility identifiers.
    existing = {f['name'].split('::')[-1] for fs, _ in parsed.values() for f in fs if not f['address']}
    rename = {a: n for a, n in rename.items() if n not in existing}
    def rename_tokens(text):
        return rename_code_tokens(text, rename)
    headers = sorted(CORE.glob('Sub_*.hpp'))
    by_name = {p.name: p for p in headers}
    ordered, seen = [], set()
    def visit(p):
        if p.name in seen:
            return
        seen.add(p.name)
        for inc in re.findall(r'#include "([^"]+)"', p.read_text()):
            basename = Path(inc.replace('\\', '/')).name
            if basename in by_name:
                visit(by_name[basename])
        ordered.append(p)
    for p in headers:
        visit(p)
    byte_header = CORE / 'Sub_00137100.hpp'
    bt = byte_header.read_text()
    start = bt.index('struct ByteView {')
    end = closing_brace(mask_comments_strings(bt), bt.index('{', start))+2
    byte_struct = bt[start:end]
    out = CORE / 'port/driving'
    out.mkdir(parents=True, exist_ok=True)
    (out/'byte_view.hpp').write_text('#pragma once\n#include <array>\n#include <cstddef>\n#include <cstdint>\nnamespace port::driving::init {\n'+byte_struct+'\n}\n')
    memory = out/'memory.hpp'
    memory.write_text(memory.read_text().replace('game_core/Sub_00137100.hpp', 'game_core/port/driving/byte_view.hpp'))
    header = '#pragma once\n// TASK-0042: host projection interfaces; no original header/ABI claim.\n#include "game_core/port/driving/byte_view.hpp"\n#include "game_core/port/driving/memory.hpp"\nnamespace port::driving { struct ContactProvider; }\n'
    for p in ordered:
        text = p.read_text()
        if p == byte_header:
            text = text[:start]+text[end:]
        text = re.sub(r'^#pragma once\s*', '', text, flags=re.M)
        text = re.sub(r'^#include "(?:(?:game_core/)?Sub_[^"]+|(?:game_core/)?port/driving/(?:memory|projections).hpp)"\s*', '', text, flags=re.M)
        text = '\n'.join(normalize_include_line(line) for line in text.split('\n'))
        header += '\n// Host declarations formerly partitioned in '+p.name+'\n'+text
    support = '#pragma once\n// TASK-0042: shared host utilities; partitions prevent helper name collisions.\n#include "game_core/port/driving/projections.hpp"\n'
    includes = set()
    output = defaultdict(list)
    audit = []
    helper_audit = []
    for p, text in snapshots.items():
        fs, namespaces = parsed[p]
        for inc in re.findall(r'^#include[^\n]+', text, re.M):
            if not re.search(r'"(?:game_core/)?Sub_', inc):
                includes.add(normalize_include_line(inc))
        scope_helpers = defaultdict(list)
        for f in fs:
            if f['anonymous'] and not f['address']:
                scope_helpers[f['scope']].append(f)
        detail = 'detail_'+p.stem.lower().replace('sub_', 'group_')
        for ns in [n for n in namespaces if not n[3]]:
            scope = '::'.join(n[3] for n in namespaces if n[3] and n[0]<ns[0]<n[2])
            chunk = text[ns[1]:ns[2]]
            edits = []
            for f in fs:
                if ns[1] <= f['start'] < ns[2]:
                    if f['address']:
                        edits.append((f['start']-ns[1], f['end']-ns[1], ''))
                    elif not f['signature'].startswith('template'):
                        edits.append((f['start']-ns[1], f['start']-ns[1], 'inline '))
            for a,b,replacement in sorted(edits, reverse=True):
                chunk=chunk[:a]+replacement+chunk[b:]
            support += '\nnamespace '+scope+'::'+detail+' {\n'+chunk+'\n}\n'
            helper_audit.append(dict(old_source=p.relative_to(ROOT).as_posix(),
                scope=scope+'::'+detail, original_chunk_sha256=hashlib.sha256(text[ns[1]:ns[2]].encode()).hexdigest(),
                migrated_tokens_sha256=cpp_tokens_sha256(rename_tokens(chunk))))
        for f in fs:
            if f['anonymous'] and not f['address']:
                continue
            body = text[f['start']:f['end']]
            if scope_helpers[f['scope']]:
                offset = f['opening']-f['start']+1
                body=body[:offset]+'\n    using namespace '+detail+';'+body[offset:]
            if f['anonymous'] and f['address']:
                header += '\nnamespace '+f['scope']+' {\n'+f['signature']+';\n}\n'
            own = ownership.get(f['address'])
            target = 'port/driving/adapters.cpp'
            path_evidence = ''
            if f['address']:
                target = 'port/driving/unresolved_entries.cpp'
                if own:
                    source = own['definition_path']
                    if not source.lower().endswith('.cpp'):
                        source = own['translation_unit']
                    if source.lower().endswith('.cpp'):
                        path_evidence = source
                        target = source.replace('\\','/')
                        while target.startswith('../'):
                            target=target[3:]
                        if target.startswith('/') or ':' in target or '..' in Path(target).parts:
                            raise ValueError('Unsafe original source path')
            if f['name'].startswith('ByteView::'):
                target = 'port/driving/byte_view.cpp'
            prefix = '// Retail '+str(f['address'])+'; '+(own['correspondence_evidence']+'; E-0042-S001; exact Debug TU '+path_evidence if path_evidence else 'explicit host partition; original TU unconfirmed')+'\n'
            output[target].append(prefix+'namespace '+f['scope']+' {\n'+body+'\n}\n')
            # Verify the source body survives exactly apart from one using-directive.
            check=body.replace('\n    using namespace '+detail+';', '', 1)
            assert check == text[f['start']:f['end']]
            audit.append(dict(old_source=p.relative_to(ROOT).as_posix(), old_identifier=f['name'],
                elf_address=f['address'] or '', source='src/game_core/'+target,
                original_body_sha256=hashlib.sha256(check.encode()).hexdigest(),
                definition_path=path_evidence, evidence_id=own['correspondence_evidence'] if own else 'port-side',
                source_identifier=rename.get(f['name'].split('::')[-1], f['name'].split('::')[-1])))
            audit[-1].update(source_scope=f['scope'], source_signature=rename_tokens(f['signature']),
                migrated_tokens_sha256=cpp_tokens_sha256(rename_tokens(check)))
    support = support.replace('#include "game_core/port/driving/projections.hpp"', '\n'.join(sorted(includes))+'\n#include "game_core/port/driving/projections.hpp"')
    (out/'projections.hpp').write_text(rename_tokens(header), encoding='utf-8')
    (out/'projection_support.hpp').write_text(rename_tokens(support), encoding='utf-8')
    for target, blocks in output.items():
        path=CORE/target
        path.parent.mkdir(parents=True, exist_ok=True)
        old=path.read_text() if path.exists() else ''
        path.write_text(old+'\n// TASK-0042: bounded host projections; original member ABI remains unverified.\n#include "game_core/port/driving/projection_support.hpp"\n'+rename_tokens('\n'.join(blocks)), encoding='utf-8')
    # Rewrite source uses and include edges only, not historical prose or evidence.
    for path in (ROOT/'src').rglob('*'):
        if path.suffix not in ('.cpp','.hpp','.h') or path in paths or path in headers:
            continue
        text=path.read_text(encoding='utf-8')
        text=re.sub(r'#include "(?:game_core/)?Sub_[^"]+\.hpp"', '#include "game_core/port/driving/projections.hpp"', text)
        path.write_text(rename_tokens(text), encoding='utf-8')
    cmake=ROOT/'CMakeLists.txt'
    text=cmake.read_text()
    for p in paths:
        text=text.replace(p.relative_to(ROOT).as_posix(), '')
    # Generated class source manifest already lists several exact shared TUs.
    manifest=(ROOT/'reverse/classes/template_sources.cmake').read_text()
    additional=[t for t in sorted(output) if 'src/game_core/'+t not in manifest]
    text=text.replace('add_library(game_core STATIC', 'add_library(game_core STATIC\n'+'\n'.join('  src/game_core/'+t for t in additional))
    cmake.write_text('\n'.join(line.rstrip() for line in text.splitlines())+'\n')
    for path in paths+headers:
        path.unlink()
    (ROOT/'reverse/naming/task0042-projection-partitions.json').write_text(json.dumps(dict(task_id='TASK-0042',
        policy='Only adopted correspondences join exact Debug ECOFF paths. Host signatures and helper namespaces are explicitly port-side. Arithmetic bodies preserved; only recovered identifier substitutions and function-local using directives added.',
        renamed_tokens=rename, functions=audit, helper_chunks=helper_audit), indent=2)+'\n')
    with (ROOT/'reverse/naming/task0042-adopted-projection-names.csv').open('w', newline='') as handle:
        writer=csv.DictWriter(handle, ['old_identifier','source_identifier','elf_address','reference_name','evidence_id'])
        writer.writeheader()
        for old,new in sorted(rename.items()):
            r=mappings['0x'+old[4:]]
            writer.writerow(dict(old_identifier=old, source_identifier=new,elf_address=r['retail_address'],reference_name=r['reference_name'],evidence_id=r['evidence_id']))
    print(json.dumps(dict(functions=len(audit), partitions=len(output), renamed_tokens=len(rename), added_cmake_sources=additional)))


if __name__ == '__main__':
    main()
