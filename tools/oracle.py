"""Windows PINE adapter. No register/trace/input-replay capabilities claimed."""
import argparse
import hashlib
import json
from pathlib import Path
import socket
import struct
import subprocess
import sys

class Pine:
    def __init__(self, port=28011, timeout=2):
        self.port, self.timeout = port, timeout

    @staticmethod
    def receive(sock, length):
        data = b''
        while len(data) < length:
            chunk = sock.recv(length - len(data))
            if not chunk: raise ConnectionError('truncated PINE response')
            data += chunk
        return data

    def request(self, payload):
        with socket.create_connection(('127.0.0.1', self.port), self.timeout) as sock:
            sock.sendall(struct.pack('<I', len(payload) + 4) + payload)
            size = struct.unpack('<I', self.receive(sock, 4))[0]
            if not 5 <= size <= 450000: raise ValueError('invalid PINE response size')
            reply = self.receive(sock, size - 4)
            if reply[0] != 0: raise RuntimeError('PINE rejected request')
            return reply[1:]

    def status(self):
        status = struct.unpack('<I', self.request(b'\x0f'))[0]
        return {0: 'running', 1: 'paused', 2: 'shutdown'}.get(status, 'unknown')

    def version(self):
        reply = self.request(b'\x08')
        size = struct.unpack('<I', reply[:4])[0]
        if size != len(reply) - 4: raise ValueError('invalid version length')
        return reply[4:].rstrip(b'\x00').decode('utf-8')

    def read32(self, address):
        if not 0 <= address <= 0x1fffffc or address % 4:
            raise ValueError('capture supports aligned physical EE RAM only')
        return struct.unpack('<I', self.request(b'\x02' + struct.pack('<I', address)))[0]

    def load_slot(self, slot):
        if not 0 <= slot <= 9: raise ValueError('slot must be 0..9')
        self.request(b'\x0a' + bytes([slot]))
        return 'request accepted; asynchronous load completion not established'

    def save_slot(self, slot):
        if not 0 <= slot <= 9: raise ValueError('slot must be 0..9')
        self.request(b'\x09' + bytes([slot]))
        return 'request accepted; asynchronous save completion not established'

    def capture(self, addresses):
        before = self.status()
        values = {hex(address): self.read32(address) for address in addresses}
        return {'version': self.version(), 'status_before': before, 'status_after': self.status(), 'ee_memory32': values, 'atomic': False, 'verification_status': 'not_tested'}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('command', choices=['probe', 'capture', 'load-slot', 'save-slot', 'launch'])
    parser.add_argument('--port', type=int, default=28011)
    parser.add_argument('--address', type=lambda x: int(x, 0), action='append', default=[])
    parser.add_argument('--slot', type=int)
    parser.add_argument('--emulator', type=Path)
    parser.add_argument('--media', type=Path)
    parser.add_argument('--output', type=Path, default=Path('local/pipeline/pcsx2.json'))
    args = parser.parse_args()
    pine = Pine(args.port)
    try:
        if args.command == 'probe': result = {'version': pine.version(), 'status': pine.status()}
        elif args.command == 'capture': result = pine.capture(args.address)
        elif args.command == 'load-slot': result = {'load': pine.load_slot(args.slot)}
        elif args.command == 'save-slot': result = {'save': pine.save_slot(args.slot)}
        else:
            if not args.emulator or not args.emulator.is_file() or not args.media or not args.media.is_file():
                raise ValueError('existing emulator and private media required')
            # PCSX2 CLI positional media; validate installed --help before using.
            process = subprocess.Popen([str(args.emulator), str(args.media)], creationflags=subprocess.CREATE_NO_WINDOW)
            result = {'pid': process.pid, 'status': 'launched; connectivity not yet established'}
    except (OSError, ValueError, RuntimeError) as error:
        result = {'status': 'blocked', 'reason': str(error)}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result, indent=2))
    return int(result.get('status') == 'blocked')

if __name__ == '__main__': sys.exit(main())
