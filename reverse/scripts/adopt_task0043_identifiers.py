"""TASK-0043: evidence-backed identifier adoption; never infer names from offsets.

Replacement rules are explicit and entity-specific. Known original signatures
describe the reference, not the host interface. --check never writes source.
"""
import argparse
import csv
import json
from pathlib import Path
import re
from generate_class_templates import mask_comments_strings, class_range, closing_brace
from adopt_task0042_names import upsert_section

ROOT = Path(__file__).resolve().parents[2]
RENAMES = {
    'CurrentIndex_0016A130': 'GetCurrentItemNumber',
    'FindByHash_0016A5B0': 'FindItem',
    'Nth_002503F8': 'GetNode',
    'FindLeafByNumber_001FA840': 'FindLeafByEventNumber',
    'Requirement_002008B8': 'GetGoal',
    'StartNode_001F5D00': 'GetSelectedEvent',
    'SetStartNode_001F5D28': 'SetSelectedEvent',
    'StringHash_002534A8': 'bStringHash',
    'SetWidgetColour_00140CA0': 'SetColor',
    'SetWidgetAlpha_00140ED0': 'SetAlpha',
    'Remove_00249790': 'RemoveTextureHash',
    'SliceStatusByte_00200018': 'EventRankState::GetStatus',
    'SliceStatusWord_00200068': 'EventTimeState::GetStatus',
    'EventRecord_001FFF60': 'EventInfo',
    'Sub_002001A8': 'GetRaceIndex',
    'Sub_00200710': 'RacerRank',
    'Sub_002008F8': 'GetTrack',
    'Sub_00200878': 'GetCar',
    'Sub_00200938': 'GetReward',
    'Sub_00201350': 'GetRaceCount',
    'Sub_001F91D0': 'Show',
    'Sub_001F4E68': 'HandleHighlight',
    'Sub_002486E8': 'SetupToAnimate',
    'Sub_0026D258': 'SOUND_HzToSPU',
    'Sub_0010C158': 'FixStripSTs',
    'Sub_00107600': 'GetPixelSize',
    'Sub_0011B520': 'eCreateLookAtMatrix',
    'Sub_0019CC40': 'Calibrate',
    'Sub_001A5F88': 'Calibrate',
    'Sub_001A4348': 'Calibrate',
    'Sub_001375F0': 'Calibrate',
    'Sub_001A4158': 'SetState',
    'Sub_001A4170': 'SetState',
    'Sub_0019C808': 'PhysicsSystem::ComputeAuxiliaries',
    'Sub_00137788': 'PhysicsCar::ComputeAuxiliaries',
    'Sub_001A4358': 'Wheel::ComputeAuxiliaries',
    'Sub_0019C670': 'PhysicsSystem::GetDerivative',
    'Sub_0018A9D8': 'Engine::GetDerivative',
    'Sub_001A46B0': 'Wheel::GetDerivative',
    'Sub_0018F838': 'Table::GetValue',
    'Sub_0018F9F0': 'Graph::GetValue',
    'Sub_001ADD70': 'Aerodynamics::ResolveForces',
    'Sub_001A6330': 'Suspension::ResolveForces',
}


def token_replace(text, names):
    masked = mask_comments_strings(text)
    protected = [(m.end(),closing_brace(masked,m.end()-1)) for m in
        re.finditer(r'\bnamespace\s+hp2::recovered\s*\{', masked)]
    # Whole-entry shell spelling is selected per declaring class/overload by
    # the complete member registry. A global projected token is not that API.
    edits = [(m.start(), m.end(), names[m[0]]) for m in re.finditer(r'\b[A-Za-z_]\w*\b', masked)
             if m[0] in names and not (m[0].startswith('Sub_') and
                 any(a<=m.start()<b for a,b in protected))]
    for a,b,value in reversed(edits):
        text = text[:a]+value+text[b:]
    return text


def rewrite_fields(text, path, bridges):
    """Only audited receiver/scope bindings; identical offsets in other classes stay.

    record, tree, row-indexed records and the three getter methods are reviewed
    bindings in the four named source files. The rule never matches arbitrary
    .Field_<offset> receivers project-wide.
    """
    if path not in ('src/game_core/indep/src/eventtree.hpp',
                    'src/game_core/indep/src/eventtree.cpp',
                    'src/port/runtime/frontend_runtime.cpp',
                    'src/port/runtime/event_tree_view.cpp',
                    'src/tests/eventtree_tests.cpp'):
        return text
    for row in bridges:
        old, new = row['old_identifier'], row['recovered_name']
        masked = mask_comments_strings(text)
        edits = []
        for m in re.finditer(r'\b'+re.escape(old)+r'\b', masked):
            before = masked[max(0,m.start()-130):m.start()]
            owner = row['owner']
            accept = False
            if path.endswith('.hpp'):
                bounds = class_range(text, 'EventInfo' if owner.startswith('EventRecord') else owner)
                accept = bool(bounds and bounds[0] < m.start() < bounds[1])
            elif owner.startswith('EventRecord'):
                accept = bool(re.search(r'(?:\brecord\s*\.|\brecords\s*\[[^\]\n]+\]\s*\.)\s*$', before))
            else:
                accept = bool(re.search(r'\btree\s*\.\s*$', before))
                if path.endswith('eventtree.cpp') and not accept:
                    # Unqualified table members occur only in the three existing
                    # EventTree typed getters; do not touch other class methods.
                    previous = list(re.finditer(r'EventTree_001FFC40::Sub_00200(?:878|8F8|938)\(', masked[:m.start()]))
                    if previous:
                        start = previous[-1].start()
                        opening = masked.find('{', start)
                        accept = opening < m.start() < closing_brace(masked, opening)
            if accept:
                edits.append((m.start(),m.end(),new))
        for a,b,value in reversed(edits):
            text=text[:a]+value+text[b:]
    return text


def rewrite(text, path, bridges):
    # Different GetStatus projections retain separate declaring scopes, avoiding
    # accidental overload selection when a caller passes an integer literal.
    if path == 'src/game_core/memorycard/include/memorycard.h':
        text=text.replace('int SliceStatusByte_00200018(std::uint8_t slice_value, std::uint32_t goal_threshold);',
            '// E-0043-N01; typed host carrier, not a recovered native class layout.\n'
            'struct EventRankState {\n    static int GetStatus(std::uint8_t slice_value, std::uint32_t goal_threshold);\n};')
        text=text.replace('int SliceStatusWord_00200068(std::int32_t slice_value, std::int32_t goal_threshold);',
            '// E-0043-N01; typed host carrier, not a recovered native class layout.\n'
            'struct EventTimeState {\n    static int GetStatus(std::int32_t slice_value, std::int32_t goal_threshold);\n};')
    if path == 'src/game_core/port/driving/projections.hpp':
        # A host carrier keeps original declaring scopes distinct when several
        # projections share identical C++ parameter types. It is not a native
        # object declaration and contains no copied layout or lifetime semantics.
        groups = {}
        for old, new in RENAMES.items():
            if not old.startswith('Sub_') or '::' not in new:
                continue
            match = re.search(r'(?m)^([^\n;{}]+)\b'+old+r'(\([^;{}]*\));', text)
            if not match:
                continue
            owner, method = new.split('::')
            # Namespace is determined at the original declaration, not inferred
            # from original class ownership (native vs finite-value adapters).
            prefix = mask_comments_strings(text[:match.start()])
            scopes = list(re.finditer(r'namespace\s+([\w:]+)\s*\{', prefix))
            scope = next((m[1] for m in reversed(scopes) if
                prefix[m.end():].count('{') >= prefix[m.end():].count('}')), None)
            if scope is None:
                raise ValueError('Projection declaration has no explicit namespace')
            groups.setdefault((scope, owner), []).append((match[0], match[1]+' '+method+match[2]+';'))
        for (scope, owner), declarations in groups.items():
            for old, _ in declarations:
                text=text.replace(old, '// Declaration grouped below in the '+owner+' host carrier.', 1)
            text+='\nnamespace '+scope+' {\n// E-0043-N01: typed static host projection carrier; no native layout/ABI claim.\nstruct '+owner+' {\n'
            text+=''.join('    static '+new.strip()+'\n' for _, new in declarations)+'};\n}\n'
    return rewrite_fields(token_replace(text, RENAMES), path, bridges)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    mode=parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--write', action='store_true')
    mode.add_argument('--check', action='store_true')
    args=parser.parse_args()
    rows=list(csv.DictReader((ROOT/'reverse/naming/task0042-adopted-retail-names.csv').open()))
    mappings={r['retail_address']: r for r in rows}
    fields=json.loads((ROOT/'reverse/naming/task0043-type-field-bridges.json').read_text())
    plan=[]
    for old,new in RENAMES.items():
        if old.startswith('EventRecord_'):
            assert fields['type_findings'][0]['recovered_name'] == new
            plan.append(dict(old_identifier=old,new_identifier=new,entity_kind='struct',evidence_id='E-0043-F01',rewrite_scope='all_src'))
        else:
            row=mappings['0x'+old[-8:]]
            short=new.split('::')[-1]
            assert re.search(r'(?:^|::)'+re.escape(short)+r'\(', row['reference_name'])
            plan.append(dict(old_identifier=old,new_identifier=new,entity_kind='function',
                elf_address=row['retail_address'],debug_address=row['debug_address'],reference_name=row['reference_name'],
                evidence_id=row['evidence_id'],adoption_evidence='E-0043-N01',rewrite_scope='all_src',
                limitation='Host scope/storage/signature is preserved; original declaring scope is in reference_name.'))
    for row in fields['bridges']:
        plan.append(dict(old_identifier=row['old_identifier'],new_identifier=row['recovered_name'],
            entity_kind='field',owner=row['owner'],original_owner=row['original_owner'],
            evidence_id=row['evidence_id'],rewrite_scope='audited_event_receivers',
            retail_offset=row['retail_offset'],debug_offset=row['debug_offset']))
    changed=[]
    for path in (ROOT/'src').rglob('*'):
        if path.suffix not in ('.cpp','.hpp','.h'):
            continue
        relative=path.relative_to(ROOT).as_posix()
        if relative == 'src/tests/class_template_tests.cpp':
            # The generator selects each whole-entry overload's actual callable
            # placeholder; projected global tokens must not rewrite its calls.
            continue
        old=path.read_text(encoding='utf-8')
        new=rewrite(old, relative, fields['bridges'])
        if old!=new:
            changed.append(relative)
            if args.write:
                path.write_text(new,encoding='utf-8')
    if args.check and changed:
        raise ValueError('Unadopted confirmed identifiers: '+', '.join(changed))
    if args.write:
        (ROOT/'reverse/naming/task0043-identifier-renames.json').write_text(json.dumps(dict(task_id='TASK-0043',
            retail_sha256=fields['retail_sha256'],debug_sha256=fields['debug_sha256'],renames=plan,
            normalized_aliases={r['new_identifier']:r['old_identifier'] for r in plan if r['entity_kind']=='field'},
            policy='Only code tokens are replaced. Type/field names require independent bridges; original source comments and strings retain provenance.',
            retained_title_evidence='reverse/naming/task0043-title-projection.json'),indent=2)+'\n')
        index_path=ROOT/'reverse/symbols.csv'
        index=list(csv.DictReader(index_path.open(encoding='utf-8',newline='')))
        for item in plan:
            if item['entity_kind']!='function':
                continue
            for row in index:
                if row['elf_address']!=item['elf_address'] or row['binary_id']!='sha256-'+fields['retail_sha256']:
                    continue
                ids=row['evidence_ids'].split(';')
                if 'E-0043-N01' not in [x.strip() for x in ids]:
                    row['evidence_ids']+=';E-0043-N01'
                record=ROOT/row['record_path']
                if not record.is_file():
                    raise ValueError('Missing existing evidence record: '+str(record))
                text=upsert_section(record.read_text(encoding='utf-8'),'TASK-0043 source identifier adoption',
                    f'E-0043-N01: source token `{item["old_identifier"]}` now uses `{item["new_identifier"]}`. '
                    f'Original reference spelling is `{item["reference_name"]}`; Retail {item["elf_address"]}, '
                    f'Debug {item["debug_address"]}; correspondence evidence {item["evidence_id"]}.\n\n'
                    'See reverse/naming/task0043-identifier-renames.json. Existing host signatures/scopes, '
                    'behavior, limitations and verification status remain unchanged. Constructors and '
                    'unrepresentable whole-entry overload gaps are separately managed by the class registry.')
                if record.read_text(encoding='utf-8')!=text:
                    record.write_text(text,encoding='utf-8')
        import io
        stream=io.StringIO(newline='');writer=csv.DictWriter(stream,list(index[0]),lineterminator='\n');writer.writeheader();writer.writerows(index)
        if index_path.read_text()!=stream.getvalue():
            index_path.write_text(stream.getvalue())
    print(json.dumps(dict(approved_functions=sum(r['entity_kind']=='function' for r in plan),
        approved_types=1,approved_fields=len(fields['bridges']),changed_files=len(changed),write=args.write)))


if __name__ == '__main__':
    main()
