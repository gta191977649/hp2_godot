"""TASK-0041: conservative full-vtable reference correspondence, no renames.

Two distinct globally unique normalized function anchors are required. Every
slot/delta and header shape must agree; known contradictory targets reject a
pair. Table correspondence does not grant exclusive class ownership to shared
inherited member implementations. Input game bytes never enter output files.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import csv
import hashlib
import json
from pathlib import Path
import re
import struct
import unittest

from match_debug_reference import CACHE, DEBUG_SHA, MAP_SHA, RETAIL_SHA, ROOT, Elf, write_csv


def unique_anchors(rows):
    forward, reverse = defaultdict(set), defaultdict(set)
    for row in rows:
        if row['status'] in ('ambiguous', 'conflicting_call_targets'):
            continue
        d, r = int(row['debug_address'], 16), int(row['retail_address'], 16)
        forward[d].add(r)
        reverse[r].add(d)
    return {d: next(iter(rs)) for d, rs in forward.items()
            if len(rs) == 1 and len(reverse[next(iter(rs))]) == 1}


def pair_tables(debug_words, retail_words, anchors):
    """Return distinct anchored target count or None for incompatible pairs."""
    if len(debug_words) != len(retail_words) or len(debug_words) < 6:
        return None
    # Header second word can contain RTTI. Without a recovered correspondence,
    # do not silently normalize it. Header words and all this-deltas are exact.
    if debug_words[:2] != retail_words[:2]:
        return None
    reverse = {r: d for d, r in anchors.items()}
    evidence = set()
    for i in range(2, len(debug_words), 2):
        if debug_words[i] != retail_words[i]:
            return None
        d, r = debug_words[i + 1], retail_words[i + 1]
        if bool(d) != bool(r):
            return None
        if d in anchors and anchors[d] != r:
            return None
        if r in reverse and reverse[r] != d:
            return None
        if d in anchors:
            evidence.add((d, r))
    return len(evidence) if len(evidence) >= 2 else None


def verify_retail_cache(retail):
    manifest = json.loads((CACHE / 'retail-memory-manifest.json').read_text())
    if retail.sha != RETAIL_SHA or manifest['retail_sha256'] != RETAIL_SHA:
        raise ValueError('Retail identity mismatch')
    for row in manifest['sections']:
        data = (CACHE / f"retail{row['section']}.bin").read_bytes()
        if hashlib.sha256(data).hexdigest() != row['sha256']:
            raise ValueError('Ghidra cache digest mismatch')
        if retail.read(int(row['address'], 16), row['size']) != data:
            raise ValueError('Retail ELF differs from Ghidra cache')


def main(args):
    debug = Elf(args.debug_elf)
    retail = Elf(json.loads((ROOT / 'config/ps2_elf.local.json').read_text())['elf'])
    if debug.sha != DEBUG_SHA or hashlib.sha256(args.map.read_bytes()).hexdigest() != MAP_SHA:
        raise ValueError('Debug reference identity mismatch')
    verify_retail_cache(retail)
    map_names = defaultdict(list)
    for line in args.map.read_text().splitlines():
        m = re.match(r'^\s+0x([0-9a-fA-F]+)\s+(.+?)\s*$', line)
        if m:
            map_names[int(m[1], 16)].append(m[2])
    with args.matches.open(newline='') as handle:
        anchors = unique_anchors(list(csv.DictReader(handle)))
    debug_tables = [s for s in debug.symbols if s['name'].startswith('_vt$') and s['size'] >= 16 and s['size'] % 8 == 0]
    retail_tables = json.loads((ROOT / 'reverse/classes/vtables.json').read_text())['confirmed']
    labels = defaultdict(set)
    with (ROOT / 'reverse/classes.csv').open(newline='') as handle:
        for row in csv.DictReader(handle):
            for table in row['vtables'].split():
                labels[int(table, 16)].add(row['class_name'])
    functions = defaultdict(list)
    for s in debug.symbols:
        if s['kind'] == 2:
            functions[s['address']].append(s['name'])
    pairs = []
    for d in debug_tables:
        dw = struct.unpack('<' + 'I' * (d['size'] // 4), debug.read(d['address'], d['size']))
        for r in retail_tables:
            size = 8 + r['slots'] * 8
            if size != d['size']:
                continue
            address = int(r['vtable'], 16)
            rw = struct.unpack('<' + 'I' * (size // 4), retail.read(address, size))
            # Check historical ledger is describing the bytes now being read.
            for member in r['members']:
                i = int(member['offset'], 16) // 4
                if rw[i] != int(member['function'], 16) or (rw[i - 1] & 0xffff) != (member['this_delta'] & 0xffff):
                    raise ValueError('Retail vtable ledger differs from live-bound bytes')
            count = pair_tables(dw, rw, anchors)
            if count is not None:
                pairs.append((d, r, dw, rw, count))
    by_d, by_r = Counter(p[0]['address'] for p in pairs), Counter(int(p[1]['vtable'], 16) for p in pairs)
    ledger, slots = [], []
    for d, r, dw, rw, count in pairs:
        ra = int(r['vtable'], 16)
        unique = by_d[d['address']] == by_r[ra] == 1
        names = [n[:-len(' virtual table')] for n in map_names[d['address']] if n.endswith(' virtual table')]
        allocator_labels = sorted(labels[ra])
        ledger.append(dict(debug_vtable=f"0x{d['address']:08X}", retail_vtable=f'0x{ra:08X}',
            debug_symbol=d['name'], debug_table_designations=names,
            retail_allocator_labels=allocator_labels,
            label_text_comparison=('no_allocator_label' if not allocator_labels else
                'exact_text_present' if set(names) & set(allocator_labels) else 'different_text_not_equivalence'),
            slots=r['slots'], distinct_anchored_targets=count,
            debug_pair_candidates=by_d[d['address']], retail_pair_candidates=by_r[ra],
            status='supported_unique_vtable' if unique else 'ambiguous_vtable', evidence_id='E-0041-V01'))
        for i in range(2, len(dw), 2):
            da, target = dw[i + 1], rw[i + 1]
            known = functions.get(da, [])
            exact_names = map_names.get(da, [])
            slots.append(dict(debug_vtable=f"0x{d['address']:08X}", retail_vtable=f'0x{ra:08X}',
                pointer_offset=f'0x{(i + 1) * 4:X}', this_delta=struct.unpack('<h', struct.pack('<H', dw[i] & 0xffff))[0],
                slot_prefix_word=f'0x{dw[i]:08X}',
                debug_address=f'0x{da:08X}', retail_address=f'0x{target:08X}',
                mangled_names=' | '.join(sorted(set(known))), reference_names=' | '.join(exact_names),
                anchored_by_normalized_match=anchors.get(da) == target,
                status=('supported_vtable_slot' if unique and known else
                    'unknown_debug_function' if unique else 'ambiguous_vtable'),
                ownership='shared/inherited implementation possible; no exclusive class attribution',
                evidence_id='E-0041-V01'))
    target_names = defaultdict(set)
    for row in slots:
        if row['status'] == 'supported_vtable_slot':
            target_names[row['retail_address']].add(row['reference_names'])
    for row in slots:
        row['retail_target_distinct_reference_name_sets'] = len(target_names[row['retail_address']])
        row['name_identity'] = ('ambiguous_shared_or_folded_names' if len(target_names[row['retail_address']]) > 1
            else 'slot_correspondence_only_not_exclusive_ownership')
    summary = dict(evidence_id='E-0041-V01', debug_sha256=DEBUG_SHA, retail_sha256=RETAIL_SHA,
        map_sha256=MAP_SHA, normalized_unique_anchors=len(anchors), debug_tables=len(debug_tables),
        retail_confirmed_tables=len(retail_tables), candidate_pairs=len(pairs),
        unique_pairs=sum(r['status'] == 'supported_unique_vtable' for r in ledger),
        slot_status_counts=dict(Counter(r['status'] for r in slots)),
        shared_or_folded_name_conflicts={a: sorted(ns) for a, ns in target_names.items() if len(ns) > 1},
        unmatched_debug_symbols=[s['name'] for s in debug_tables if not by_d[s['address']]],
        unmatched_retail_tables=[r['vtable'] for r in retail_tables if not by_r[int(r['vtable'], 16)]],
        limitations=['Only existing 252 confirmed retail tables; no new table detection',
            'At least two distinct normalized unique function anchors; known contradictions rejected',
            'Full size/header/this-delta/zero-slot shape must agree; RTTI pointer headers not normalized',
            'Pair uniqueness measured over eligible debug symbols and existing confirmed retail tables',
            'Slot implementation name does not imply exclusive ownership by table class',
            'Allocator label spelling difference does not prove type equivalence',
            'Names are debug reference names; table correspondence does not verify behavior'])
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / 'task0041-debug-vtable-ledger.json').write_text(json.dumps(dict(summary=summary, pairs=ledger), indent=2) + '\n')
    slots.sort(key=lambda x: (x['retail_address'], x['debug_vtable'], x['pointer_offset']))
    write_csv(args.output / 'task0041-debug-vtable-slots.csv', slots, list(slots[0]) if slots else ['status'])
    print(json.dumps({k: v for k, v in summary.items() if not k.startswith('unmatched_') and k != 'limitations'}, indent=2))


class MatchingTests(unittest.TestCase):
    def test_two_distinct_anchors(self):
        self.assertEqual(pair_tables((0, 0, 0, 10, 0, 20), (0, 0, 0, 100, 0, 200), {10: 100, 20: 200}), 2)
        self.assertIsNone(pair_tables((0, 0, 0, 10, 0, 10), (0, 0, 0, 100, 0, 100), {10: 100}))

    def test_contradiction_delta_and_reverse_conflict(self):
        d, r = (0, 0, 0, 10, 0, 20, 0, 30), (0, 0, 0, 100, 0, 200, 0, 300)
        self.assertIsNone(pair_tables(d, r, {10: 100, 20: 200, 30: 400}))
        self.assertIsNone(pair_tables(d, r, {10: 100, 20: 200, 40: 300}))
        self.assertIsNone(pair_tables(d, (0, 0, 1, 100, 0, 200, 0, 300), {10: 100, 20: 200}))

    def test_ambiguous_anchor_excluded(self):
        rows = [dict(debug_address='0x10', retail_address=r, status='relocation_equivalent_candidate') for r in ('0x100', '0x200')]
        self.assertEqual(unique_anchors(rows), {})


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--debug-elf', type=Path)
    parser.add_argument('--map', type=Path)
    parser.add_argument('--matches', type=Path, default=ROOT / 'reverse/naming/task0041-debug-retail-matches.csv')
    parser.add_argument('--output', type=Path, default=ROOT / 'reverse/naming')
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    if args.self_test:
        unittest.main(argv=['match_debug_vtables'])
    elif args.debug_elf and args.map:
        main(args)
    else:
        parser.error('Supply --debug-elf and --map or --self-test')
