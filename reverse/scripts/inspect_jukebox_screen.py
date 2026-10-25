"""TASK-0092: reproduce read-only Retail MCP evidence without changing Ghidra.

Raw responses remain ignored. Undefined handler bytes are a recorded limitation,
not a reason to create functions or disassemble into the project database.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.ghidra import Client

SHA = '7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2'


def elf_bytes(data, address, length):
    if data[:7] != b'\x7fELF\x01\x01\x01':
        raise ValueError('Expected original little-endian ELF32')
    phoff = struct.unpack_from('<I', data, 28)[0]
    stride, count = struct.unpack_from('<HH', data, 42)
    for i in range(count):
        kind, offset, virtual, _, size = struct.unpack_from('<5I', data, phoff+i*stride)
        if kind == 1 and virtual <= address and address+length <= virtual+size:
            return data[offset+address-virtual:offset+address-virtual+length]
    raise ValueError('Evidence address is not file-backed')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT/'local/task0091/capture')
    args = parser.parse_args()
    data = args.elf.read_bytes()
    if hashlib.sha256(data).hexdigest() != SHA:
        raise ValueError('Wrong authorized Retail ELF')
    output = args.output.resolve()
    if not output.is_relative_to(ROOT/'local'):
        raise ValueError('Raw evidence must remain under ignored local/')
    output.mkdir(parents=True, exist_ok=True)
    client = Client(timeout=30)
    queries = []
    for address in ('001f1220','001f02d8','001f0478','001f0cc8','001ef240',
                    '001eef60','001ef1c8','001aee90','001aeee0','001aef80','001aed90'):
        queries.append(dict(action='decompile',address=address))
    # Includes the known expected failure, so another session can distinguish
    # a missing function boundary from a disconnected/wrong-target MCP server.
    queries.append(dict(action='decompile',address='001f0d10'))
    queries.append(dict(action='listing',address='001f0d5c',end_address='001f121c',max_lines=1000))
    for address, length in (('001f02d8',3912),('002b9d30',104),('0031ae00',120)):
        queries.append(dict(action='read',address=address,length=length,format='hex'))
    manifest = []
    for index, query in enumerate(queries):
        tool = 'memory' if query['action'] == 'read' else 'inspect'
        arguments = dict(file_name='SLUS_203.62',**query)
        result = client.call('tools/call',dict(name=tool,arguments=arguments))
        target = output/f'{index:02d}-{query["address"]}-{query["action"]}.json'
        encoded = json.dumps(result,indent=2).encode()
        target.write_bytes(encoded)
        expected_gap = query['action']=='decompile' and query['address']=='001f0d10'
        if result.get('isError') and not expected_gap:
            raise RuntimeError(f'MCP query failed: {target}')
        bound = None
        if tool == 'memory':
            observed = bytes.fromhex(result['structuredContent']['hex_data'])
            bound = observed == elf_bytes(data,int(query['address'],16),query['length'])
            if not bound:
                raise ValueError(f'MCP bytes differ from authorized ELF: {query["address"]}')
        manifest.append(dict(query=arguments,file=target.name,sha256=hashlib.sha256(encoded).hexdigest(),
                             is_error=bool(result.get('isError')),expected_gap=expected_gap,elf_bytes_match=bound))
    report = dict(binary_sha256=SHA,version='unknown executable revision',
                  evidence_ids=['E-0092-J01','E-0092-J02','E-0092-J03'],
                  status='static evidence only; not original-runtime verification',queries=manifest)
    (output/'manifest.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(f'PASS {len(manifest)} read-only queries; 3 ELF memory ranges match; handler boundary gap recorded')


if __name__ == '__main__':
    main()
