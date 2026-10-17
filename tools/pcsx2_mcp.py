"""Port-side bounded adapter to the supplied PCSX2 DebugServer (JSON/TCP).

No RAM/register writes, savestates, input injection or flawed server step command.
Capture output may contain proprietary bytes and must remain under ignored local/.
"""
import argparse
import json
from pathlib import Path
import socket
import time


class DebugServer:
    def __init__(self, port=21512, timeout=2):
        self.port, self.timeout = port, timeout

    def request(self, command, **fields):
        payload = json.dumps({'cmd': command, 'cpu': 'ee', **fields}).encode() + b'\n'
        with socket.create_connection(('127.0.0.1', self.port), self.timeout) as sock:
            sock.sendall(payload)
            data = bytearray()
            while b'\n' not in data:
                chunk = sock.recv(65536)
                if not chunk:
                    raise ConnectionError('Truncated DebugServer response')
                data.extend(chunk)
                if len(data) > 1048576:
                    raise ValueError('Oversized DebugServer response')
        result = json.loads(data.split(b'\n', 1)[0])
        if result.get('ok') is not True:
            raise RuntimeError(result.get('error', 'DebugServer rejected request'))
        return result

    def status(self):
        return self.request('status')['data']

    def require_paused(self):
        state = self.status()
        if not state.get('alive') or state.get('paused') is not True:
            raise RuntimeError('An alive paused EE is required')
        return state

    def read_memory(self, address, size):
        if not 0 < size <= 65536 or not 0 <= address <= 0x2000000-size:
            raise ValueError('Read requires bounded physical EE RAM')
        data = bytes.fromhex(self.request('read_memory', address=address, length=size)['hex'])
        if len(data) != size:
            raise ValueError('Unexpected DebugServer memory reply length')
        return data

    def snapshot(self, windows=()):
        before = self.require_paused()
        regs = self.request('read_registers')['data']
        blocks = [{'address': hex(a), 'size': n, 'hex': self.read_memory(a, n).hex()} for a, n in windows]
        after = self.require_paused()
        if before != after or int(regs['pc'], 16) != int(before['pc'], 16):
            raise RuntimeError('EE changed during snapshot')
        return {'status': before, 'registers': regs, 'blocks': blocks}

    def run_until(self, address, seconds=3):
        """Execute naturally to one owned BP; always pause and remove only that BP.

        This advances original game state and cannot restore it. Never calls step.
        Existing BPs at the address are preserved by refusing before any mutation.
        """
        if address % 4 or not 0x100000 <= address <= 0x1fffffc:
            raise ValueError('Breakpoint must be aligned physical EE code')
        if not 0 < seconds <= 10:
            raise ValueError('Execution deadline must be within 10 seconds')
        self.require_paused()
        existing = self.request('list_breakpoints')['breakpoints']
        if any(int(bp['address'], 16) == address for bp in existing):
            raise RuntimeError('Breakpoint address already belongs to another debugger user')
        owned = False
        try:
            self.request('set_breakpoint', address=address, temporary=False,
                         description='TASK-0038 bounded oracle')
            owned = True
            self.request('resume')
            deadline = time.monotonic() + seconds
            while time.monotonic() < deadline:
                state = self.status()
                if state.get('paused'):
                    if int(state['pc'], 16) != address:
                        raise RuntimeError('Execution stopped at another breakpoint')
                    return state
                time.sleep(0.01)
            raise TimeoutError('Owned breakpoint was not reached within the deadline')
        finally:
            # The server pause reply alone does not establish completion.
            try:
                self.request('pause')
                deadline = time.monotonic() + self.timeout
                while not self.status().get('paused'):
                    if time.monotonic() >= deadline:
                        raise TimeoutError('EE did not pause after execution')
                    time.sleep(0.01)
            finally:
                if owned:
                    self.request('remove_breakpoint', address=address)


def register(snapshot, category, name):
    return int(next(r['value'] for r in snapshot['registers'][category]['regs'] if r['name'] == name), 16)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=21512)
    parser.add_argument('--output', type=Path, default=Path('local/task0038/pcsx2-mcp/paused.json'))
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(Path(__file__).resolve().parents[1] / 'local'):
        raise ValueError('Raw captures must remain under ignored local/')
    result = DebugServer(args.port).snapshot()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps({'output': str(output), 'status': result['status'],
                      'fcr31': hex(register(result, 'FCR', 'fcr31'))}))


if __name__ == '__main__':
    main()
