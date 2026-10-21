"""Port-side, conservative recovery inventory. Never promotes evidence records."""
from __future__ import annotations

from collections import Counter
import csv
import html
import json
import os
from pathlib import Path
import re
import tempfile

ROOT = Path(__file__).resolve().parents[2]
REPORT = Path('reverse/progress.json')
STATES = ('missing', 'stub', 'unconfirmed_body', 'partial', 'evidenced_body')
# Mask comments and literals before finding definitions, retaining offsets/newlines.
LEX = re.compile(r'R"(?P<delimiter>[^ ()\\\t\r\n]{0,16})\(.*?\)(?P=delimiter)"|//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
DEFINITION = re.compile(r'(?P<name>(?:[A-Za-z_]\w*::)+~?[A-Za-z_]\w*)\s*\([^;{}]*\)\s*(?:const\s*)?(?:noexcept\s*)?(?:override\s*)?\{')
ADDRESS = re.compile(r'(?i)(?:0x|\bDebug\s*)([0-9a-f]{8})\b')
GAP = re.compile(r'\b(?:not_implemented|HP2_NOT_IMPLEMENTED)\s*\(')


def mask(text):
    return LEX.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), text)


def definitions(text, path):
    """Conservative lexical candidates, not a C++ semantic parser.

    Unsupported syntax/overloads are left unresolved, never credited by guessing.
    """
    clean = mask(text)
    result = []
    previous_end = 0
    for match in DEFINITION.finditer(clean):
        statement_start = max(clean.rfind(c, 0, match.start()) for c in ';{}') + 1
        preamble = clean[statement_start:match.start()]
        if re.search(r'[=()]|\b(?:return|if|while|for|switch)\b', preamble):
            continue
        start = match.end() - 1
        depth, end = 1, start + 1
        while end < len(clean) and depth:
            depth += (clean[end] == '{') - (clean[end] == '}')
            end += 1
        if depth:
            continue
        # Only immediately preceding comments/declaration, never another function's body.
        prefix = text[max(previous_end, text.rfind('}', 0, match.start()) + 1):match.start()]
        prefix = '\n'.join(prefix.splitlines()[-12:])
        prefix = '\n'.join(m[0] for m in LEX.finditer(prefix) if m[0].startswith(('//', '/*')))
        addresses = {f'0x{m[1].upper()}' for m in ADDRESS.finditer(prefix)}
        suffix = re.search(r'(?:Sub_|FUN_)([0-9A-Fa-f]{8})$', match['name'])
        if suffix:
            addresses.add('0x' + suffix[1].upper())
        body = clean[start + 1:end - 1]
        gaps = list(GAP.finditer(body))
        # Exact whole-entry throwing template; any surrounding logic is partial.
        stub = bool(re.fullmatch(r'\s*(?:hp2::)?not_implemented\s*\([^;]*\)\s*;\s*', body))
        result.append(dict(name=match['name'], path=path,
                           line=text.count('\n', 0, match.start()) + 1,
                           addresses=sorted(addresses), gap=bool(gaps), stub=stub,
                           anchor_kind='debug' if re.search(r'\bDebug\s*[0-9a-fA-F]{8}', prefix) else 'unspecified'))
        previous_end = end
    return result


def read_json(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def read_csv(path):
    with path.open(encoding='utf-8-sig', newline='') as handle:
        return list(csv.DictReader(handle))


def address(value):
    return f'0x{int(value, 16):08X}'


def valid_record(root, row, binary, addr):
    if not row.get('evidence_ids') or binary not in row.get('record_path', ''):
        return False
    path = (root / row['record_path']).resolve()
    if not path.is_relative_to(root.resolve() / 'reverse/functions') or not path.is_file():
        return False
    try:
        text = path.read_text(encoding='utf-8-sig').lower()
    except UnicodeError:
        return False
    evidence = re.findall(r'E-\d{4}-[A-Za-z0-9]+', row['evidence_ids'])
    return (addr[2:].lower() in text and binary.removeprefix('sha256-') in text
            and any(item.lower() in text for item in evidence))


def classify(candidates, record_ok, status):
    if not candidates:
        return 'missing', 'no uniquely address-bound definition'
    if len(candidates) != 1:
        return 'unconfirmed_body', 'ambiguous definitions/overloads'
    body = candidates[0]
    if body['stub']:
        return 'stub', ''
    if body['gap']:
        return 'partial', 'explicit untranslated path in body'
    if record_ok and status in ('Implemented', 'Tested', 'Verified'):
        return 'evidenced_body', ''
    return 'unconfirmed_body', 'body exists; implementation evidence/status not established'


def collect(root=ROOT):
    registry = root / 'reverse/classes'
    inventory = read_json(registry / 'template-inventory.json')
    primary = inventory['binary_id']
    classes, bindings = {}, {}

    def add_class(binary, name):
        return classes.setdefault((binary, name), set())

    def bind(binary, owner, addr, source='', method=''):
        addr = address(addr)
        add_class(binary, owner).add(addr)
        row = bindings.setdefault((binary, addr), dict(owners=set(), sources=set(), methods=set()))
        row['owners'].add(owner)
        if source:
            row['sources'].add(source)
        if method:
            row['methods'].add(owner + '::' + method)

    for cls in inventory['classes']:
        add_class(primary, cls['name'])
        for member in cls.get('members', []):
            bind(primary, cls['name'], member['elf_address'], member.get('source', cls.get('source', '')), member.get('method', ''))
    for cls in read_csv(root / 'reverse/classes.csv'):
        add_class(cls['binary_id'], cls['class_name'])
        for addr in cls['functions'].split():
            bind(cls['binary_id'], cls['class_name'], addr)
    members = read_json(registry / 'task0043-member-registry.json')
    for name in members['classes']:
        add_class(members['binary_id'], name)
    for member in members['members']:
        bind(members['binary_id'], member['owner'], member['elf_address'], member.get('source', ''), member.get('method', ''))
    for path in sorted(registry.glob('*-debug-canonical-owners.json')):
        owners = read_json(path)
        for name, cls in owners['classes'].items():
            add_class(owners['binary_id'], name)
            for addr in cls.get('debug_addresses', []):
                for source in cls.get('implementation_sources', []) or ['']:
                    bind(owners['binary_id'], name, addr, source)
                if cls.get('header'):
                    bind(owners['binary_id'], name, addr, cls['header'])

    symbols = {}
    for row in read_csv(root / 'reverse/symbols.csv'):
        symbols.setdefault((row['binary_id'], address(row['elf_address'])), []).append(row)
    bodies = []
    for pattern in ('*.cpp', '*.hpp', '*.h'):
        for path in sorted((root / 'src/game_core').rglob(pattern)):
            bodies += definitions(path.read_text(encoding='utf-8-sig'), path.relative_to(root).as_posix())
    by_address = {}
    for body in bodies:
        for addr in body['addresses']:
            by_address.setdefault(addr, []).append(body)
    functions = []
    for (binary, addr), binding in sorted(bindings.items()):
        records = symbols.get((binary, addr), [])
        statuses = sorted({r['status'] for r in records})
        valid = [r for r in records if valid_record(root, r, binary, addr)]
        sources = set(binding['sources'])
        for row in records:
            sources.update(re.findall(r'src/[\w/.-]+\.(?:cpp|hpp|h)\b', row.get('pc_implementation', '')))
        implementation_sources = {s for s in sources if s.endswith('.cpp')}
        candidates = [b for b in by_address.get(addr, []) if
                      (not implementation_sources or b['path'] in sources) and
                      any(b['name'].endswith(owner + '::' + b['name'].split('::')[-1]) for owner in binding['owners']) and
                      ((b['anchor_kind'] == 'debug') == (binary != primary))]
        state, reason = classify(candidates, bool(valid), statuses[0] if len(statuses) == 1 else '')
        issues = []
        if len(statuses) > 1:
            issues.append('conflicting record statuses')
        if any(s in ('Implemented', 'Tested', 'Verified') for s in statuses) and state != 'evidenced_body':
            issues.append('implementation claim requires review against current source/scope')
        if state == 'unconfirmed_body':
            issues.append(reason)
        functions.append(dict(binary_id=binary, address=addr, owners=sorted(binding['owners']),
                              state=state, record_statuses=statuses, record_valid=bool(valid),
                              host_test_claim='Tested' in statuses,
                              original_verification_claim='Verified' in statuses,
                              definitions=[{k: b[k] for k in ('path', 'line', 'name')} for b in candidates],
                              issues=issues))
    index = {(f['binary_id'], f['address']): f for f in functions}
    class_rows = []
    for (binary, name), addresses in sorted(classes.items()):
        counts = Counter(index[binary, a]['state'] for a in addresses)
        class_rows.append(dict(binary_id=binary, name=name, bound_methods=len(addresses),
                               counts={s: counts[s] for s in STATES},
                               method_total='unknown', addresses=sorted(addresses)))
    report = dict(schema_version=2, task='TASK-0078', primary_binary=primary,
                binaries=[{k: row[k] for k in ('binary_id', 'sha256', 'version')}
                          for row in read_csv(root / 'reverse/binaries.csv')],
                policy='Retail headline: evidenced bodies / inventoried Retail functions. Class coverage: evidenced / explicitly bound methods, not whole-class completion. Full catalogs remain SHA-separated. Bodies and recorded claims are not equivalence; unsupported syntax remains unresolved. No evidence status is changed.',
                classes=class_rows, functions=functions)
    # Preserve the class-bound report/API, adding the full catalog separately.
    # Class ownership comes only from the registries above, never from mangled
    # name parsing or guessed namespaces. Unowned entries remain unassigned.
    catalog = {}
    retail_inventory = root / 'docs/assessment/FUNCTION_INVENTORY.csv'
    if retail_inventory.is_file():
        for row in read_csv(retail_inventory):
            binary = 'sha256-' + row['binary_sha256']
            addr = address(row['elf_address'])
            original = row.get('original_identifier', '')
            catalog[binary, addr] = dict(name=original if original and original != 'unknown' else '',
                                         inventoried=True)
    debug_inventory = root / 'reverse/naming/task0041-debug-functions.csv'
    if debug_inventory.is_file():
        # The tracked TASK0041 export is explicitly bound to this Debug ELF.
        debug_binary = 'sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b'
        for row in read_csv(debug_inventory):
            catalog[debug_binary, address(row['debug_address'])] = dict(
                name=row.get('reference_name') or row.get('mangled_name') or '', inventoried=True)
    complete_debug = root / 'reverse/naming/task0078-debug-function-inventory.json'
    if complete_debug.is_file():
        provenance = read_json(complete_debug)
        rows = read_csv(complete_debug.with_suffix('.csv'))
        debug_binary = 'sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b'
        addresses = [address(row['elf_address']) for row in rows]
        if provenance['binary_id'] != debug_binary or len(set(addresses)) != len(addresses) or len(addresses) != provenance['count']:
            raise ValueError('Complete Debug inventory identity/count is inconsistent')
        for addr in addresses:
            catalog.setdefault((debug_binary, addr), dict(name='', inventoried=True))
    for key in set(bindings) | set(symbols):
        catalog.setdefault(key, dict(name='', inventoried=False))
    all_functions = []
    for (binary, addr), entry in sorted(catalog.items()):
        records = symbols.get((binary, addr), [])
        if (binary, addr) in index:
            function = dict(index[binary, addr])
        else:
            statuses = sorted({r['status'] for r in records})
            valid = any(valid_record(root, r, binary, addr) for r in records)
            sources = {source for row in records for source in re.findall(
                r'src/[\w/.-]+\.(?:cpp|hpp|h)\b', row.get('pc_implementation', ''))}
            candidates = [b for b in by_address.get(addr, []) if b['path'] in sources and
                          ((b['anchor_kind'] == 'debug') == (binary != primary))]
            state, reason = classify(candidates, valid, statuses[0] if len(statuses) == 1 else '')
            function = dict(binary_id=binary, address=addr, owners=[], state=state,
                            record_statuses=statuses, record_valid=valid,
                            host_test_claim='Tested' in statuses,
                            original_verification_claim='Verified' in statuses,
                            definitions=[{k: b[k] for k in ('path', 'line', 'name')} for b in candidates],
                            issues=[reason] if reason else [])
        original_names = [r['original_identifier'] for r in records
                          if r.get('original_identifier') not in ('', 'unknown', None)]
        bound_names = sorted(bindings.get((binary, addr), {}).get('methods', []))
        source_names = [b['name'] for b in function['definitions']]
        function.update(name=(original_names or source_names or bound_names or
                              [entry['name'] or 'Sub_' + addr[2:]])[0],
                        inventoried=entry['inventoried'],
                        record_paths=sorted({r['record_path'] for r in records if r.get('record_path')}))
        all_functions.append(function)
    primary_functions = [f for f in all_functions if f['binary_id'] == primary and f['inventoried']]
    report['inventory_functions'] = all_functions
    report['overall'] = dict(binary_id=primary, total=len(primary_functions),
                             evidenced=sum(f['state'] == 'evidenced_body' for f in primary_functions))
    return report


STATE_LABELS = {'missing': 'Missing / unresolved', 'stub': 'Throwing stub',
                'unconfirmed_body': 'Unconfirmed body', 'partial': 'Partial',
                'evidenced_body': 'Evidenced body'}


def cell(value):
    return html.escape(str(value), quote=False).replace('|', '&#124;').replace('\n', ' ')


def coverage(done, total):
    return f'{100 * done / total:.1f}% ({done}/{total})' if total else 'unknown (0/?)'


def render_compact(report):
    """Entire dashboard: one headline and exactly two collapsed sections."""
    overall = report.get('overall', {})
    total, done = overall.get('total', 0), overall.get('evidenced', 0)
    percent = f'{100 * done / total:.1f}%' if total else 'unknown'
    lines = ['<!-- generated:dashboard:begin -->',
             '<!-- Generated by reverse/scripts/generate_progress.py; do not edit. -->', '',
             f'# Progress {percent} ({done}/{total if total else "?"} functions)', '',
             '<details>', '<summary>CLASS</summary>', '',
             '| Binary | Class | Progress (evidenced / bound) |', '|---|---|---|']
    primary = report.get('primary_binary')
    def binary_label(binary):
        label = 'Retail' if binary == primary else 'Debug' if binary.startswith('sha256-815b60c7') else 'Binary'
        return label + ' ' + binary.removeprefix('sha256-')[:12]
    classes = sorted(report.get('classes', []), key=lambda c: (c['binary_id'] != primary, c['binary_id'], c['name']))
    functions = report.get('inventory_functions', report.get('functions', []))
    index = {(f['binary_id'], f['address']): f for f in functions}
    for cls in classes:
        lines.append(f"| {binary_label(cls['binary_id'])} | <code>{cell(cls['name'])}</code> | "
                     + coverage(cls['counts']['evidenced_body'], cls['bound_methods']) + ' |')
    lines += ['', '</details>', '', '<details>', '<summary>Functions</summary>', '']

    def group(binary, name, members):
        if not members:
            return
        done = sum(f['state'] == 'evidenced_body' for f in members)
        lines.extend(['<details>', f'<summary>{cell(name)} · {binary_label(binary)} · {coverage(done, len(members))}</summary>', '',
                      '| Address | Function | Implementation | Record status |', '|---|---|---|---|'])
        for function in sorted(members, key=lambda f: f['address']):
            name_cell = '<code>' + cell(function.get('name', 'Sub_' + function['address'][2:])) + '</code>'
            if function.get('record_paths'):
                name_cell = '[' + name_cell + '](../' + function['record_paths'][0] + ')'
            status = ', '.join(function['record_statuses']) or 'Not recorded'
            if not function.get('inventoried', True):
                status += ' (outside inventory)'
            lines.append(f"| <code>{function['address']}</code> | {name_cell} | "
                         + STATE_LABELS[function['state']] + ' | ' + cell(status) + ' |')
        lines.extend(['', '</details>', ''])

    for cls in classes:
        group(cls['binary_id'], cls['name'], [index[cls['binary_id'], addr] for addr in cls['addresses']])
    for binary in sorted({f['binary_id'] for f in functions}, key=lambda b: (b != primary, b)):
        group(binary, 'Unassigned / global', [f for f in functions if f['binary_id'] == binary and not f['owners']])
    lines += ['</details>', '', '<!-- generated:dashboard:end -->', '']
    return '\n'.join(lines)


def render(report):
    """Compatibility entry point for the compact dashboard."""
    return render_compact(report)


def encoded(report):
    return json.dumps(report, indent=2, ensure_ascii=True) + '\n'


def atomic_write(path, text, expected=None):
    """Compare before replace; never overwrite an independently edited document."""
    before = path.read_text(encoding='utf-8') if path.exists() else None
    if expected is not None and before != expected:
        raise RuntimeError(f'Concurrent edit: {path}')
    if before == text:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(dir=path.parent, prefix=path.name + '.', suffix='.tmp')
    try:
        with os.fdopen(fd, 'w', encoding='utf-8', newline='\n') as handle:
            handle.write(text)
        current = path.read_text(encoding='utf-8') if path.exists() else None
        if current != before:
            raise RuntimeError(f'Concurrent edit: {path}')
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
    return True
