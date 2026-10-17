"""Port-side offline ZDIR/ZZDATA extractor. See TASK-0047 for evidence/limits."""
import argparse
import hashlib
import json
from pathlib import Path
import struct


def name_hash(name):
    value = 0xffffffff
    for char in name.upper().replace('/', '\\'):
        value = (value * 33 + ord(char)) & 0xffffffff
    return value


def decompress(data):
    """Bounded COMP reader; Debug LZDecompress 001B1658, E-0047-002."""
    if data[:4] != b'COMP':
        return data
    if len(data) < 16:
        raise ValueError('Truncated COMP header')
    _, version, mode, expected, packed = struct.unpack_from('<4sHHII', data)
    if version != 0x1001 or mode not in (0, 1) or packed != len(data):
        raise ValueError('Unsupported COMP framing')
    if mode == 1:
        result = data[16:]
    else:
        result = bytearray()
        pos, flags = 16, 1
        while pos < packed:
            if flags == 1:
                flags = struct.unpack_from('<H', data, pos)[0] | 0x10000
                pos += 2
            if flags & 1:
                a, b = data[pos:pos+2]
                pos += 2
                distance, count = ((a & 0xf0) << 4) | b, (a & 15) + 3
                if not 0 < distance <= len(result):
                    raise ValueError('Invalid COMP backreference')
                start = len(result) - distance
                if distance >= count:
                    result.extend(result[start:start+count])
                else:
                    for _ in range(count):
                        result.append(result[-distance])
            else:
                result.append(data[pos])
                pos += 1
            flags >>= 1
            if len(result) > expected:
                raise ValueError('COMP output exceeds declared size')
    if len(result) != expected:
        raise ValueError('COMP output size mismatch')
    return bytes(result)


def digest_file(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def extract(source, output, names_root, resume=False):
    index = source / 'ZDIR.BIN'
    archive = source / 'ZZDATA.BIN'
    raw = index.read_bytes()
    if len(raw) % 12:
        raise ValueError('Partial index record')
    records = list(struct.iter_unpack('<III', raw))
    names = {}
    for path in names_root.rglob('*'):
        if path.is_file():
            name = path.relative_to(names_root).as_posix()
            names.setdefault(name_hash(name), set()).add(name)
    # Original-data filenames are candidates only; a unique 32-bit match is not
    # proof of Alpha path identity. Preserve that distinction in the manifest.
    output.mkdir(parents=True, exist_ok=True)
    manifest_path = output / 'extraction_manifest.json'
    if manifest_path.exists():
        raise FileExistsError(manifest_path)
    plan = []
    targets = set()
    for ordinal, (key, sector, size) in enumerate(records):
        if sector * 2048 + size > archive.stat().st_size:
            raise ValueError('Index range outside archive')
        candidates = sorted(names.get(key, []))
        name = candidates[0] if len(candidates) == 1 else f'_UNRESOLVED/{ordinal:04d}_{key:08X}.bin'
        if name.upper() in targets:
            name = f'_DUPLICATES/{ordinal:04d}/{name}'
        targets.add(name.upper())
        target = output / name
        if (target.exists() and not resume) or not target.resolve().is_relative_to(output.resolve()):
            raise FileExistsError(target)
        plan.append((ordinal, key, sector, size, name, candidates))
    entries = []
    with archive.open('rb') as stream:
        for ordinal, key, sector, size, name, candidates in plan:
            stream.seek(sector * 2048)
            data = stream.read(size)
            decoded = decompress(data)
            target = output / name
            target.parent.mkdir(parents=True, exist_ok=True)
            sha = hashlib.sha256(decoded).hexdigest()
            if target.exists() and resume:
                if digest_file(target) != sha:
                    raise ValueError(f'Resume would overwrite different data: {target}')
            else:
                with target.open('xb') as dest:
                    dest.write(decoded)
            if digest_file(target) != sha:
                raise ValueError('Written-file digest mismatch')
            entries.append(dict(ordinal=ordinal, hash=f'{key:08X}', offset=sector*2048,
                stored_size=size, output_size=len(decoded), path=name,
                path_basis='unique reference-tree hash candidate' if len(candidates)==1 else 'unresolved',
                compressed=data[:4]==b'COMP', stored_sha256=hashlib.sha256(data).hexdigest(), sha256=sha))
            print(f'{ordinal+1}/{len(plan)} {name} {len(decoded)}', flush=True)
    manifest = dict(evidence='E-0047-001..003', source=str(source),
        index_sha256=digest_file(index), archive_sha256=digest_file(archive),
        names_root=str(names_root), entries=entries)
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print(json.dumps(dict(entries=len(entries), compressed=sum(e['compressed'] for e in entries),
        unresolved=sum(e['path_basis']=='unresolved' for e in entries),
        output_bytes=sum(e['output_size'] for e in entries))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--names-root', type=Path, required=True)
    parser.add_argument('--resume', action='store_true', help='Reuse only byte-identical existing outputs')
    args = parser.parse_args()
    extract(args.source, args.output, args.names_root, args.resume)
