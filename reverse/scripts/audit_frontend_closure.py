"""TASK-0098: reconcile frontend registration evidence, runtime and original documents.

This is an inventory/freshness check, never a behavioral or original-runtime pass.
Only hashes, registration identifiers and port-side acceptance metadata are exported.
"""
import argparse
import csv
import hashlib
import io
import json
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.ps2_assets import path
from tools.asset_extractor.elab_chunks import parse_chunks_top

RETAIL_SHA = '7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2'
DEBUG_SHA = '815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b'
OUTPUT = ROOT / 'reverse/frontend/closure-routes.csv'


def walk(nodes):
    for node in nodes:
        yield node
        yield from walk(node.children)


def documents():
    result = []
    for bundle in ('FRONTEND/FRONTB.BUN', 'GLOBAL/INGAMEB.BUN'):
        data = path(*bundle.split('/')).read_bytes()
        nodes, consumed = parse_chunks_top(data)
        if consumed != len(data):
            raise ValueError('Incomplete bundle parsing: ' + bundle)
        # Same authored screen header field consumed by native fmenus.cpp.
        headers = [node for node in walk(nodes) if node.tag == 0x10203]
        for index, node in enumerate(headers):
            if node.size < 0x1c:
                raise ValueError('Truncated authored screen header')
            name_hash = struct.unpack_from('<I', data, node.offset + 8 + 8)[0]
            result.append((bundle, index, f'0x{name_hash:08X}'))
    return result


def task_for(name):
    if name == 'Select Car':
        return 'TASK-0098', 'event-select/cars/skins/confirm/cancel/reentry/stale-load'
    if name.startswith('Select Car2P'):
        return 'TASK-0091', 'two-pad-frontend/confirm/cancel; two-player-race-out-of-scope'
    if name == 'View Event Tree':
        return 'TASK-0098', 'locked/unlocked/details/reward/select/back/all32-events'
    if name == 'JukeBox Music':
        return 'TASK-0092', 'play/change/filter/finish/exit/silent-state/persist'
    if name == 'Audio Options':
        return 'TASK-0093', 'sliders/output/defaults/feedback/back/persist'
    if name in ('Display Options', 'Customize P1 HUD', 'Customize P2 HUD',
                'Calibrate Display', 'Calibrate display offset'):
        return 'TASK-0094', 'adjust/defaults/back/persist/help'
    if name == 'NameEntry':
        return 'TASK-0095', 'grid/edit/commit/reentry/help'
    if name.startswith('Pause') or name.startswith('Race Objective'):
        return 'TASK-0101', 'actual-race/open/adjust/resume/retry/abandon'
    if name in ('Points Rewards', 'Task Rewards', 'Events Unlocked', 'Reward menu',
                'Race Results', 'Tourney Standings', 'Best Lap Times', 'Best Total Times',
                'End Game Menu', 'Be The Cop Results 1P', 'Be The Cop Results 2P',
                'Lap Times 1P', 'Lap Times 2P'):
        return 'TASK-0102', 'actual-outcome/summary/return/reward-once/persist'
    return 'TASK-0091', 'production-entry/input/variants/back/reentry/media-and-settings'


def generated():
    with (ROOT / 'reverse/frontend/task0091-coverage.csv').open(encoding='utf-8', newline='') as f:
        registrations = list(csv.DictReader(f))
    source = (ROOT / 'src/game_core/port/frontend/frontendscreens/registry_0016adb0.cpp').read_text(encoding='utf-8')
    runtime = re.findall(r'\{"([^"]+)", (0x[0-9A-Fa-f]+)u, (0x[0-9A-Fa-f]+)u\}', source)
    expected = [(r['class_name'], r['document_hash'], r['handler_address']) for r in registrations]
    if runtime != expected:
        raise ValueError('Runtime registration operands differ from E-0091 inventory')
    if len(set((name, doc) for name, doc, _ in runtime)) != len(runtime):
        raise ValueError('Duplicate class/document identity')
    docs = documents()
    rows = []
    for i, item in enumerate(registrations):
        name = item['class_name']
        task, acceptance = task_for(name)
        matches = [(bundle, index) for bundle, index, name_hash in docs if name_hash == item['document_hash']]
        # A registration with a missing document remains visible as a blocker.
        rows.append(dict(route_id=f'REG-{i:03d}', class_name=name,
            document_hash=item['document_hash'], retail_handler=item['handler_address'],
            documents=';'.join(f'{bundle}:{index}' for bundle, index in matches) or 'missing',
            owner_task=task, evidence=item['registration_evidence'],
            production_evidence=item['production_route'], acceptance=acceptance,
            window_acceptance='pending', original_comparison='pending'))
    registered = {r['document_hash'] for r in registrations}
    for bundle, index, name_hash in docs:
        if name_hash in registered:
            continue
        rows.append(dict(route_id=f'DOC-{bundle.split("/")[0]}-{index:03d}',
            class_name='', document_hash=name_hash, retail_handler='',
            documents=f'{bundle}:{index}', owner_task='TASK-0091', evidence='authored-header-0x10203+0x08',
            production_evidence='class/caller/variant mapping unresolved',
            acceptance='recover-caller-and-state-variants/production-entry/input/back',
            window_acceptance='pending', original_comparison='pending'))
    output = io.StringIO(newline='')
    writer = csv.DictWriter(output, fieldnames=list(rows[0]))
    writer.writeheader()
    writer.writerows(rows)
    return output.getvalue().replace('\r\n', '\n'), len(runtime), len(docs), len(rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for config, digest in (('ps2_elf', RETAIL_SHA), ('ps2_debug_elf', DEBUG_SHA)):
        settings = json.loads((ROOT / f'config/{config}.local.json').read_text(encoding='utf-8'))
        if hashlib.sha256(Path(settings['elf']).read_bytes()).hexdigest() != digest:
            raise ValueError('Wrong authorized ELF: ' + config)
    text, registrations, docs, routes = generated()
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding='utf-8') != text:
            raise ValueError('Stale closure routes; run py -3 reverse/scripts/audit_frontend_closure.py')
    else:
        OUTPUT.write_text(text, encoding='utf-8')
    print(f'PASS inventory only: {registrations} registrations, {docs} authored documents, {routes} route/unknown rows')
    print('Production/window/original behavior acceptance remains pending; this is not exhaustive caller coverage.')


if __name__ == '__main__':
    main()
