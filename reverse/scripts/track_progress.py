"""TASK-0073: local progress refresh/watch and optional read-only MCP catalog checks."""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

from recovery_progress import ROOT, atomic_write, collect, encoded, read_csv, read_json

CACHE = ROOT / 'local/progress'


@contextmanager
def writer_lock(root=ROOT):
    """OS advisory lock releases even after a process crash; lock file is ignored."""
    path = root / 'local/progress/writer.lock'
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('a+b') as handle:
        if path.stat().st_size == 0:
            handle.write(b'0')
            handle.flush()
        handle.seek(0)
        try:
            if os.name == 'nt':
                import msvcrt
                msvcrt.locking(handle.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                import fcntl
                fcntl.flock(handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except OSError as exc:
            raise RuntimeError('Another progress writer is active; retry shortly.') from exc
        try:
            yield
        finally:
            handle.seek(0)
            if os.name == 'nt':
                msvcrt.locking(handle.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(handle, fcntl.LOCK_UN)


def refresh(check=False):
    return subprocess.run([sys.executable, str(ROOT / 'reverse/scripts/generate_progress.py')]
                          + (['--check'] if check else []), cwd=ROOT).returncode


def payload(result, client=None, deadline=None):
    if result.get('isError'):
        raise RuntimeError('MCP returned an error')
    if 'structuredContent' in result:
        data = result['structuredContent']
        if client is not None and data.get('output_id') and data.get('session_id'):
            chunks, offset = [], 0
            while True:
                if deadline is not None and time.monotonic() >= deadline:
                    raise TimeoutError('Stored-output read exceeded time budget')
                chunk = payload(client.call('tools/call', {'name': 'read_tool_output', 'arguments': dict(
                    action='read', session_id=data['session_id'], output_id=data['output_id'],
                    view='envelope_json', offset=offset, max_chars=16000)}))
                content = chunk.get('content')
                if not isinstance(content, str) or not content:
                    raise RuntimeError('Incomplete stored MCP output')
                chunks.append(content)
                next_offset = chunk.get('nextOffset')
                if next_offset is None:
                    return json.loads(''.join(chunks))
                if next_offset <= offset or next_offset > 4000000:
                    raise RuntimeError('Invalid stored-output offset')
                offset = next_offset
        return data
    for block in result.get('content', []):
        if block.get('type') == 'text':
            return json.loads(block['text'])
    raise RuntimeError('MCP returned no JSON payload')


def resource(client, uri):
    result = client.call('resources/read', {'uri': uri})
    return json.loads(result['contents'][0]['text'])


def identity_sha(info):
    """Accept only explicitly SHA-256-labelled fields, never infer identity from name."""
    found = set()
    def visit(value):
        if isinstance(value, dict):
            for key, item in value.items():
                if key.lower().replace('_', '').replace('-', '').replace(' ', '') in ('sha256', 'executablesha256'):
                    if isinstance(item, str) and len(item) == 64:
                        try:
                            int(item, 16)
                            found.add(item.lower())
                        except ValueError:
                            pass
                else:
                    visit(item)
        elif isinstance(value, list):
            for item in value:
                visit(item)
    visit(info)
    if len(found) != 1:
        raise RuntimeError('Program info has no unique executable SHA-256; catalog rejected')
    return found.pop()


def program_identity(info, expected, root=ROOT):
    try:
        actual = identity_sha(info)
        method = 'program_info_sha256'
    except RuntimeError:
        # Current MCP info omits hashes. Bind to the explicitly configured original
        # import path, never to another file discovered by searching the disk.
        config = 'ps2_debug_elf.local.json' if info.get('programName') == 'NFSHP2.ELF' else 'ps2_elf.local.json'
        configured = Path(read_json(root / 'config' / config)['elf']).resolve()
        imported = info.get('executablePath', '')
        if re_windows_path(imported):
            imported = imported[1:]
        if Path(imported).resolve() != configured:
            raise RuntimeError('Ghidra import path does not match configured original ELF')
        with configured.open('rb') as handle:
            actual = hashlib.file_digest(handle, 'sha256').hexdigest()
        method = 'configured_import_file_sha256; database memory not certified'
    if actual != expected:
        raise RuntimeError('Program identity mismatch')
    return method


def re_windows_path(path):
    return len(path) > 3 and path[0] == '/' and path[1].isalpha() and path[2] == ':'


def fetch_catalog(client, program, sha, deadline=None):
    info = resource(client, f'ghidra://program/{program}/info')
    method = program_identity(info, sha)
    tools = client.call('tools/list', {})['tools']
    name = next(t['name'] for t in tools if t['name'] == 'functions' or t['name'].endswith('_functions'))
    rows, cursor, seen = [], None, set()
    while True:
        if deadline is not None and time.monotonic() >= deadline:
            raise TimeoutError('Catalog refresh exceeded time budget')
        args = dict(action='list', file_name=program, page_size=500)
        if cursor:
            args['cursor'] = cursor
        data = payload(client.call('tools/call', {'name': name, 'arguments': args}), client, deadline)
        page = data.get('data')
        if not isinstance(page, list):
            raise RuntimeError('Unexpected function-list schema; catalog rejected')
        rows.extend(page)
        cursor = data.get('next_cursor')
        if not cursor:
            break
        if cursor in seen or not page:
            raise RuntimeError('Incomplete/repeated MCP pagination')
        seen.add(cursor)
    if not rows:
        raise RuntimeError('Empty catalog rejected')
    # Recheck identity after collection, before replacing any cache.
    final_info = resource(client, f'ghidra://program/{program}/info')
    program_identity(final_info, sha)
    if isinstance(info.get('functionCount'), int) and len(rows) != info['functionCount']:
        raise RuntimeError('Catalog count differs from program info; incomplete/changed catalog')
    if final_info.get('functionCount') != info.get('functionCount'):
        raise RuntimeError('Program function count changed during refresh')
    entries = [f['entry_point'] for f in rows]
    if len(set(entries)) != len(entries):
        raise RuntimeError('Repeated function entries; catalog rejected')
    return dict(schema_version=1, program=program, binary_id='sha256-' + sha,
                identity_method=method, complete=True, functions=rows)


def refresh_mcp(endpoint, cache=CACHE, client_factory=None):
    """Failures preserve each last complete, identity-checked catalog."""
    if client_factory is None:
        sys.path.insert(0, str(ROOT))
        from tools.ghidra import Client
        client_factory = lambda: Client(endpoint, timeout=8)
    status = dict(state='offline', programs={}, message='', attempted_at=time.time())
    try:
        client = client_factory()
        programs = resource(client, 'ghidra://programs')['programs']
        available = {p['name'] for p in programs}
        identities = {r['binary_id']: r for r in read_csv(ROOT / 'reverse/binaries.csv')}
        targets = {
            'SLUS_203.62': read_json(ROOT / 'reverse/classes/template-inventory.json')['binary_id'],
            'NFSHP2.ELF': read_json(ROOT / 'reverse/classes/task0044-debug-canonical-owners.json')['binary_id'],
        }
        report = collect()
        for program, binary in targets.items():
            try:
                row = identities[binary]
                if program not in available:
                    raise RuntimeError('Program unavailable')
                catalog = fetch_catalog(client, program, row['sha256'], time.monotonic() + 45)
                entries = {int(f['entry_point'], 16) for f in catalog['functions']}
                catalog['bound_addresses_not_in_catalog'] = sorted(f['address'] for f in report['functions']
                    if f['binary_id'] == row['binary_id'] and int(f['address'], 16) not in entries)
                atomic_write(cache / (program + '.json'), encoded(catalog))
                status['programs'][program] = 'fresh'
            except Exception as exc:
                status['programs'][program] = 'cached/unavailable: ' + str(exc)
        status['state'] = 'fresh' if all(v == 'fresh' for v in status['programs'].values()) else 'cached/unavailable'
    except Exception as exc:
        status['message'] = str(exc)
    atomic_write(cache / 'status.json', encoded(status))
    print('MCP ' + status['state'] + ': ' + (status['message'] or json.dumps(status['programs'])), flush=True)
    return status


def snapshot(root=ROOT):
    result = {}
    patterns = ('src/game_core/**/*.cpp', 'src/game_core/**/*.hpp', 'src/game_core/**/*.h',
                'src/port/**/*.cpp', 'src/port/**/*.hpp', 'src/platform/**/*.cpp', 'src/platform/**/*.hpp',
                'reverse/**/*.json', 'reverse/**/*.csv', 'reverse/functions/**/*.md',
                'reverse/structures/**/*.md', 'reverse/scripts/*.py', 'tasks/**/*.md',
                'docs/assessment/*.csv')
    paths = {p for pattern in patterns for p in root.glob(pattern)}
    paths.update(root / p for p in ('docs/PROGRESS_LOG.md', 'docs/PROGRESS.md', 'tools/ghidra.py'))
    for path in sorted(paths):
        if path == root / 'reverse/progress.json' or not path.is_file():
            continue
        # Hash tracked inputs to catch same-size/same-timestamp saves too.
        data = path.read_bytes()
        if path.name in ('PROGRESS.md', 'PROGRESS_LOG.md'):
            import re
            data = re.sub(rb'<!-- generated:([\w]+):begin -->.*?<!-- generated:\1:end -->', b'', data, flags=re.S)
        result[path.relative_to(root).as_posix()] = hashlib.sha256(data).hexdigest()
    return result


class SaveDebounce:
    def __init__(self, initial):
        self.last = initial
        self.pending = None
    def observe(self, current, now):
        if current != self.last:
            self.last, self.pending = current, now
            return False
        if self.pending is not None and now - self.pending >= 1:
            self.pending = None
            return True
        return False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--once', action='store_true')
    mode.add_argument('--watch', action='store_true')
    mode.add_argument('--check', action='store_true')
    parser.add_argument('--refresh-ghidra', action='store_true')
    parser.add_argument('--endpoint', default='http://127.0.0.1:8080/mcp')
    args = parser.parse_args()
    if args.check and args.refresh_ghidra:
        parser.error('--check is offline and cannot refresh Ghidra')
    code = refresh(args.check)
    if code or args.check:
        return code
    if not args.watch:
        if args.refresh_ghidra:
            status = refresh_mcp(args.endpoint)
            return 0 if status['state'] == 'fresh' else 2
        return 0
    # Network work stays off the local save loop; only one MCP job at a time.
    with ThreadPoolExecutor(max_workers=1) as executor:
        job = executor.submit(refresh_mcp, args.endpoint)
        next_mcp = time.monotonic() + 600
        debounce = SaveDebounce(snapshot())
        print('Watching recovery inputs every 2s; Ctrl+C stops. MCP status: local/progress/status.json', flush=True)
        try:
            while True:
                time.sleep(2)
                try:
                    if debounce.observe(snapshot(), time.monotonic()):
                        if refresh():
                            debounce.pending = time.monotonic()
                    if job.done() and time.monotonic() >= next_mcp:
                        job.result()
                        job = executor.submit(refresh_mcp, args.endpoint)
                        next_mcp = time.monotonic() + 600
                except (OSError, ValueError, RuntimeError) as exc:
                    print(f'Refresh deferred: {exc}', file=sys.stderr, flush=True)
                    debounce.pending = time.monotonic()
        except KeyboardInterrupt:
            print('Watcher stopped. An active bounded MCP request may finish before exit.', flush=True)
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (OSError, ValueError, RuntimeError) as exc:
        print(f'Progress refresh failed: {exc}', file=sys.stderr)
        raise SystemExit(1)
