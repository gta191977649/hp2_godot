"""Read-only client for the existing Ghidra streamable HTTP MCP connection."""
import argparse
import hashlib
import json
from pathlib import Path
import urllib.request

class Client:
    def __init__(self, endpoint='http://127.0.0.1:8080/mcp', timeout=90):
        self.endpoint, self.session, self.sequence = endpoint, None, 0
        self.timeout = timeout
        self.call('initialize', {'protocolVersion': '2024-11-05', 'capabilities': {}, 'clientInfo': {'name': 'hp2-infrastructure', 'version': '1'}})
        self.call('notifications/initialized', {}, notification=True)

    def call(self, method, params, notification=False):
        self.sequence += 1
        body = {'jsonrpc': '2.0', 'method': method, 'params': params}
        if not notification: body['id'] = self.sequence
        headers = {'Content-Type': 'application/json', 'Accept': 'application/json, text/event-stream', 'MCP-Protocol-Version': '2024-11-05'}
        if self.session: headers['Mcp-Session-Id'] = self.session
        request = urllib.request.Request(self.endpoint, data=json.dumps(body).encode(), headers=headers)
        with urllib.request.urlopen(request, timeout=self.timeout) as response:
            self.session = response.headers.get('Mcp-Session-Id', self.session)
            raw = response.read().decode()
        if not raw: return {}
        if any(line.startswith('data:') for line in raw.splitlines()):
            messages = [json.loads(line[5:].strip()) for line in raw.splitlines() if line.startswith('data:')]
            result = next(x for x in messages if x.get('id') == self.sequence)
        else: result = json.loads(raw)
        if 'error' in result: raise RuntimeError(result['error'])
        return result.get('result', {})

    def export(self, program, address, elf):
        digest = hashlib.sha256(Path(elf).read_bytes()).hexdigest()
        available = self.call('tools/list', {})['tools']
        def tool(suffix):
            return next(x['name'] for x in available if x['name'] == suffix or x['name'].endswith('_' + suffix))
        queries = [
            ('inspect', {'action': 'decompile'}),
            ('inspect', {'action': 'listing', 'max_lines': 64}),
            ('inspect', {'action': 'references_to', 'page_size': 100}),
            ('analyze', {'action': 'call_graph', 'direction': 'both', 'depth': 1, 'max_results': 500})]
        results = []
        for name, arguments in queries:
            arguments.update(file_name=program, address=address)
            result = self.call('tools/call', {'name': tool(name), 'arguments': arguments})
            if result.get('isError'): raise RuntimeError(result)
            results.append({'query': arguments, 'result': result})
        return {'schema_version': 1, 'binary_sha256': digest, 'version': 'unknown executable revision', 'program': program, 'address': address, 'endpoint': self.endpoint, 'verification_status': 'not_tested', 'results': results}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--endpoint', default='http://127.0.0.1:8080/mcp')
    parser.add_argument('--program', default='SLUS_203.62')
    parser.add_argument('--address', default='0x00250340')
    parser.add_argument('--elf', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=Path('local/pipeline/ghidra-export.json'))
    args = parser.parse_args()
    result = Client(args.endpoint).export(args.program, args.address, args.elf)
    # Raw code/listings are private, never emitted into public documentation.
    if not args.output.resolve().is_relative_to(Path('local').resolve()):
        raise ValueError('raw analysis output must be under ignored local/')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps({'result': 'PASS', 'address': args.address, 'binary_sha256': result['binary_sha256'], 'output': str(args.output)}))

if __name__ == '__main__': main()
