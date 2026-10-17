"""Catalogue address/offset identifiers and declared types without guessing identity.

The inventory is lexical, not a C++ ABI parser. A shared address is explicitly
insufficient evidence for naming a field, struct, constant or namespace.
"""
import argparse
from collections import defaultdict
import csv
import io
import json
from pathlib import Path
import re
from generate_class_templates import ROOT, mask_comments_strings


def derive():
    mapping={r['retail_address']:r for r in csv.DictReader((ROOT/'reverse/naming/task0042-adopted-retail-names.csv').open())}
    members=json.loads((ROOT/'reverse/classes/task0043-member-registry.json').read_text())['members']
    reasons=defaultdict(set)
    for row in members:
        if row.get('name_limit'):
            reasons[row['elf_address']].add(row['name_limit'])
    found=defaultdict(lambda:dict(locations=set(),kinds=set()))
    for base in ('src','godot'):
        for path in (ROOT/base).rglob('*'):
            if path.suffix not in ('.cpp','.hpp','.h','.gd') or 'addons' in path.parts:
                continue
            relative=path.relative_to(ROOT).as_posix()
            masked=mask_comments_strings(path.read_text(encoding='utf-8'))
            if path.suffix=='.gd':
                masked=re.sub(r'#[^\n]*','',masked)
            declarations={m[2]:m[1] for m in re.finditer(r'\b(class|struct|enum(?:\s+class)?)\s+(\w+)\b',masked)}
            declarations.update({m[1]:'type_alias' for m in re.finditer(r'\busing\s+(\w+)\s*=',masked)})
            for match in re.finditer(r'\b[A-Za-z_]\w*_(?:[0-9A-Fa-f]{8}|0x[0-9A-Fa-f]+)\b',masked):
                name=match[0]
                following=masked[match.end():].lstrip()
                before=masked[max(0,match.start()-60):match.start()]
                kind=declarations.get(name)
                if not kind:
                    if re.search(r'namespace\s*$',before):kind='port_namespace'
                    elif following.startswith('('):kind='callable_or_constructor'
                    elif re.search(r'_0x[0-9A-Fa-f]+$',name):kind='offset_field_or_constant'
                    elif name.startswith('k'):kind='constant'
                    else:kind='reference_requires_declaration_context'
                item=found[name]
                item['locations'].add(relative+':'+str(masked.count('\n',0,match.start())+1))
                item['kinds'].add(kind)
            for name,kind in declarations.items():
                item=found[name]
                item['kinds'].add(kind)
                item['locations'].add(relative)
    rows=[]
    for name,item in sorted(found.items()):
        address=re.search(r'_([0-9A-Fa-f]{8})$',name)
        address='0x'+address[1].upper() if address else ''
        mapped=mapping.get(address)
        if name=='TitleStringId_001F96B8':
            state='retained_extracted_projection';reason='E-0043-T01: containing function identity does not name an extracted branch'
        elif reasons[address] and name.startswith('Sub_'):
            state='retained_unrepresentable_callable';reason=' | '.join(sorted(reasons[address]))
        elif mapped:
            state='retained_entity_not_named_by_function_address'
            reason='Function address correspondence alone cannot name this distinct entity; see declaration context'
        elif address or re.search(r'_0x[0-9A-Fa-f]+$',name):
            state='retained_without_independent_name_bridge';reason='No reviewed entity-specific naming bridge; no guessed name adopted'
        else:
            state='existing_declared_type';reason='Existing spelling retained; matching text alone does not verify original identity or layout'
        rows.append(dict(identifier=name,entity_kinds=';'.join(sorted(item['kinds'])),
            address=address,mapped_function=mapped['reference_name'] if mapped else '',
            disposition=state,reason=reason,locations=';'.join(sorted(item['locations']))))
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--write',action='store_true');args=p.parse_args()
    rows=derive();stream=io.StringIO(newline='');writer=csv.DictWriter(stream,list(rows[0]),lineterminator='\n');writer.writeheader();writer.writerows(rows)
    path=ROOT/'reverse/naming/task0043-entity-inventory.csv'
    if args.write:path.write_text(stream.getvalue())
    elif not path.exists() or path.read_text()!=stream.getvalue():raise ValueError('Stale TASK-0043 entity inventory')
    print('ENTITY_INVENTORY_OK:',len(rows),'identifiers/types; kinds are lexical and never identity evidence')


if __name__=='__main__':main()
