"""Read-only token-body preservation check against a Git baseline (TASK-0043).

Ignores comments/formatting and permits only an explicit identifier rename ledger.
Function-body multisets survive file moves and namespace/source reorganization.
No binary/game data is read or written. This checks source preservation, not PS2 equivalence.
"""
from __future__ import annotations

import argparse
import collections
import csv
import hashlib
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TOKEN = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|R"([^ ()\\\t\r\n]{0,16})\([\s\S]*?\)\1"|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_]\w*|(?:0[xX][0-9a-fA-F]+|[0-9]+)(?:[A-Za-z_0-9.]*)|::|->|&&|\|\||==|!=|<=|>=|<<|>>|\+\+|--|[^\s]')
CONTROL = {'if', 'for', 'while', 'switch', 'catch', 'sizeof', 'alignof', 'decltype', 'static_assert', 'requires'}
SUFFIXES = {'.cpp', '.hpp', '.h', '.cc', '.cxx', '.inl'}


def tokens(text: str) -> list[str]:
    return [m[0] for m in TOKEN.finditer(text) if not m[0].startswith(('//', '/*'))]


def extract(text: str, path: str = '') -> list[dict]:
    """Extract outer function bodies; nested lambdas remain protected in their parent.

    Deliberately lexical: suitable for this repo's ordinary function declarations.
    Counts and unmatched bodies are reported; compile/tests remain separate checks.
    """
    ts = tokens(text)
    pairs = {}
    stack = []
    for i, token in enumerate(ts):
        if token in ('(', '{', '['):
            stack.append((token, i))
        elif token in (')', '}', ']'):
            if stack and stack[-1][0] == {')': '(', '}': '{', ']': '['}[token]:
                _, opening = stack.pop()
                pairs[i] = opening
                pairs[opening] = i
    result = []
    covered = -1
    for i, token in enumerate(ts):
        if token != '{' or i <= covered or i not in pairs:
            continue
        # Search past const/noexcept/override/final/trailing return tokens.
        j = i - 1
        while j >= 0 and ts[j] not in (';', '{', '}', '='):
            if ts[j] == ')' and j in pairs:
                opening = pairs[j]
                before = opening - 1
                name = ts[before] if before >= 0 else ''
                if name == 'noexcept':
                    j = before - 1
                    continue
                if name not in CONTROL and re.fullmatch(r'[A-Za-z_]\w*', name):
                    first = before
                    while first >= 2 and ts[first - 1] == '::':
                        first -= 2
                    if first > 0 and ts[first - 1] == '~':
                        first -= 1
                    end = pairs[i]
                    body = ts[i + 1:end]
                    result.append({'path': path, 'name': ''.join(ts[first:before + 1]),
                                   'signature_tokens': ts[first:i], 'body_tokens': body,
                                   'stub': sole_throw(body)})
                    covered = end
                break
            j -= 1
    return result


def sole_throw(body: list[str]) -> bool:
    return bool(re.fullmatch(r'hp2::not_implemented\("0x[0-9A-Fa-f]{8}","(?:\\.|[^"\\])*"\);', ''.join(body)))


def load_renames(path: Path | None) -> dict[str, str]:
    if path is None:
        return {}
    if path.suffix == '.csv':
        rows = list(csv.DictReader(path.open(encoding='utf-8-sig', newline='')))
    else:
        data = json.loads(path.read_text(encoding='utf-8'))
        if isinstance(data, dict) and 'renamed_tokens' in data:
            return data['renamed_tokens']
        rows = data.get('renames', []) if isinstance(data, dict) else data
    result = {}
    for row in rows:
        old = row.get('old_identifier', row.get('old', ''))
        new = row.get('new_identifier', row.get('new', ''))
        if not old or not new:
            raise ValueError('Rename ledger requires old_identifier/new_identifier')
        if old in result and result[old] != new:
            raise ValueError('Ambiguous rename: ' + old)
        result[old] = new
    return result


def canonical(body: list[str], renames: dict[str, str]) -> tuple[str, ...]:
    # Replacement values may introduce a verified owner qualification. Match token
    # sequences, longest first, so both single identifiers and Class::method work.
    rules = sorted(((tokens(old), tokens(new)) for old, new in renames.items()),
                   key=lambda pair: len(pair[0]), reverse=True)
    result = []
    i = 0
    while i < len(body):
        for old, new in rules:
            if body[i:i + len(old)] == old:
                result.extend(new)
                i += len(old)
                break
        else:
            token = body[i]
            # Exact full diagnostic/binding literals only; no substring rewrites.
            if token.startswith('"') and token.endswith('"') and token[1:-1] in renames:
                token = '"' + renames[token[1:-1]] + '"'
            result.append(token)
            i += 1
    return tuple(result)


def compare(before: list[dict], after: list[dict], renames: dict[str, str], aliases: dict[str, str] | None = None, permitted_removals: list[dict] | None = None) -> dict:
    buckets = collections.defaultdict(list)
    aliases = aliases or {}
    for row in after:
        buckets[canonical(row['body_tokens'], aliases)].append(row)
    missing = []
    for row in before:
        key = canonical(list(canonical(row['body_tokens'], renames)), aliases)
        if buckets[key]:
            buckets[key].pop()
        else:
            missing.append({'path': row['path'], 'name': row['name'],
                            'body_sha256': hashlib.sha256(' '.join(key).encode()).hexdigest(),
                            'stub': row['stub'],
                            'baseline_body_sha256': hashlib.sha256(' '.join(row['body_tokens']).encode()).hexdigest(),
                            'stub_address': row['body_tokens'][4].strip('"') if row['stub'] else None})
    added = [row for group in buckets.values() for row in group]
    # A removed wrong-owner gap is permitted only by a pinned original body and
    # an explicit surviving canonical throwing entry at the same Retail address.
    approved_removed = []
    rejected_removals = []
    for permission in permitted_removals or []:
        candidates = [r for r in missing if r['path'] == permission['source']
                      and r['name'] == permission['name']]
        targets = [r for r in after if r['path'] == permission['canonical_source']
                   and r['name'] == permission['canonical_name'] and r['stub']
                   and ('"' + permission['elf_address'] + '"') in r['body_tokens']]
        if (len(candidates) == 1 and candidates[0]['stub'] and targets
                and candidates[0]['stub_address'] == permission['elf_address']) and (
                candidates[0]['baseline_body_sha256'] == permission['baseline_body_sha256']):
            approved_removed.append({**permission, 'status': 'pinned_sole_throw_removed_canonical_gap_preserved'})
            missing.remove(candidates[0])
        else:
            rejected_removals.append({'source': permission['source'], 'name': permission['name'],
                                      'reason': 'original sole-throw hash or surviving canonical throwing target did not match'})
    test_changes = [r for r in missing if r['path'].startswith('src/tests/')]
    missing = [r for r in missing if not r['path'].startswith('src/tests/')]
    invalid = [{'path': r['path'], 'name': r['name']} for r in added
               if not r['stub'] and not r['path'].startswith('src/tests/')]
    return {'baseline_functions': len(before), 'current_functions': len(after),
            'preserved_functions': len(before) - len(missing) - len(test_changes) - len(approved_removed), 'missing_or_changed': missing,
            'added_throwing_stubs': sum(r['stub'] for r in added), 'added_nonstub_bodies': invalid,
            'test_body_changes_outside_production_preservation': test_changes,
            'permitted_removed_shared_stubs': approved_removed, 'rejected_removal_permissions': rejected_removals,
            'passed': not missing and not invalid and not rejected_removals}


def git(*args: str) -> str:
    return subprocess.check_output(['git', *args], cwd=ROOT).decode('utf-8-sig')


def collect_revision(revision: str) -> list[dict]:
    paths = [path for path in git('ls-tree', '-r', '--name-only', revision, '--', 'src').splitlines()
             if Path(path).suffix in SUFFIXES]
    # One Git process avoids a subprocess for every source file on Windows.
    payload = ''.join(revision + ':' + path + '\n' for path in paths).encode()
    raw = subprocess.check_output(['git', 'cat-file', '--batch'], input=payload, cwd=ROOT)
    offset = 0
    result = []
    for path in paths:
        newline = raw.index(b'\n', offset)
        size = int(raw[offset:newline].split()[-1])
        offset = newline + 1
        source = raw[offset:offset + size].decode('utf-8-sig')
        offset += size + 1
        result.extend(extract(source, path))
    return result


def collect_current() -> list[dict]:
    return [row for path in sorted((ROOT / 'src').rglob('*'))
            if path.is_file() and path.suffix in SUFFIXES
            for row in extract(path.read_text(encoding='utf-8-sig'), path.relative_to(ROOT).as_posix())]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', default='19a8de9')
    parser.add_argument('--renames', type=Path)
    parser.add_argument('--report', type=Path, help='Optional derived JSON report; no write by default')
    parser.add_argument('--permitted-removals', type=Path, default=ROOT / 'reverse/naming/task0043-permitted-stub-removals.json')
    parser.add_argument('--baseline-inventory', type=Path, help='Optional derived CSV of original function token hashes')
    args = parser.parse_args()
    baseline = collect_revision(args.baseline)
    if args.baseline_inventory:
        with args.baseline_inventory.open('w', encoding='utf-8', newline='') as handle:
            writer = csv.DictWriter(handle, fieldnames=['baseline_commit', 'path', 'name', 'signature', 'body_token_sha256', 'sole_throw_stub'])
            writer.writeheader()
            for row in baseline:
                writer.writerow({'baseline_commit': args.baseline, 'path': row['path'], 'name': row['name'],
                                 'signature': ' '.join(row['signature_tokens']),
                                 'body_token_sha256': hashlib.sha256(' '.join(row['body_tokens']).encode()).hexdigest(),
                                 'sole_throw_stub': str(row['stub']).lower()})
    aliases = {}
    if args.renames and args.renames.suffix == '.json':
        ledger = json.loads(args.renames.read_text(encoding='utf-8'))
        if isinstance(ledger, dict):
            aliases = ledger.get('normalized_aliases', {})
    permissions = json.loads(args.permitted_removals.read_text()) if args.permitted_removals.exists() else []
    # TASK-0043 remains a historical preservation claim at its fixed final commit.
    # TASK-0044 intentionally changes algorithms against a different executable;
    # those edits require a separate, exact before/after review, not rename credit.
    migration_review = ROOT / 'reverse/task0044/source-migration-review.json'
    if migration_review.exists() and args.baseline == '19a8de9':
        historical_after = collect_revision('8c7efde')
        report = compare(baseline, historical_after, load_renames(args.renames), aliases, permissions)
        import audit_task0056_review
        migration = audit_task0056_review.check_current()
        report['task0044_source_migration_review'] = migration
        report['historical_preservation_target'] = git('rev-parse', '8c7efde').strip()
        report['passed'] = report['passed'] and migration['passed']
    else:
        report = compare(baseline, collect_current(), load_renames(args.renames), aliases, permissions)
    report['baseline_commit'] = git('rev-parse', args.baseline).strip()
    report['scope'] = 'All recognized outer C++ function bodies under src, including headers and Godot bindings; comments/whitespace ignored; test-body changes reported separately and do not fail production preservation.'
    text = json.dumps(report, indent=2) + '\n'
    if args.report:
        args.report.write_text(text, encoding='utf-8')
    print(text, end='')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
