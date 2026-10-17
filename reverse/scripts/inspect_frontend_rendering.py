"""TASK-0039 read-only Ghidra MCP capture and original frontend asset census.

Raw decompilation, listings and asset observations stay under ignored local/.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
from collections import Counter

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.ghidra import Client
from tools.ps2_assets import path
from tools.asset_extractor.elab_chunks import parse_chunks

SHA = "7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2"
ENTRIES = {
    "0013ffa8": 240, "00140098": 96, "001401d8": 88,
    "00140930": 68, "00140a48": 36, "00140cd8": 88, "00140d30": 236,
    "001411d8": 124, "00141288": 36, "00141a98": 788,
    "00141808": 516,
    "00141db0": 40, "00141dd8": 240, "00141f70": 416,
    "001422f0": 1532, "001428f0": 1928, "00143078": 1016,
    "00144ed0": 780, "001f7978": 1392,
}

def walk(nodes):
    for node in nodes:
        yield node
        yield from walk(node.children)

def census():
    data = path("FRONTEND", "FRONTB.BUN").read_bytes()
    nodes, _ = parse_chunks(data, 0, len(data))
    result = dict(screens=0, quads=0, text_widgets=0, gradients=0, nondefault_uv=0,
                  nonzero_rotations=0, text_flags={}, fonts=[])
    flags = Counter()
    for node in walk(nodes):
        off = node.offset + 8
        if node.tag == 0x80010201:
            result["screens"] += 1
        elif node.tag == 0x10210:
            result["quads"] += 1
            result["gradients"] += len(set(struct.unpack_from("<4I", data, off+0x58))) > 1
            result["nondefault_uv"] += struct.unpack_from("<8f", data, off+0x70) != (0,0,1,0,1,1,0,1)
            angle, mode = struct.unpack_from("<iI", data, off+0x40)
            result["nonzero_rotations"] += mode == 2 and angle != 0
        elif node.tag == 0x10212:
            result["text_widgets"] += 1
            flags[struct.unpack_from("<I", data, off+0x58)[0]] += 1
        elif node.tag == 0x15010:
            texture, count, pairs, glyph_offset, pair_offset = struct.unpack_from("<5I", data, off)
            heights = [struct.unpack_from("<f", data, node.offset+glyph_offset+i*32+16)[0]
                       for i in range(count)]
            result["fonts"].append(dict(texture_hash=f"{texture:08X}", glyphs=count, pairs=pairs,
                                        glyph_offset=glyph_offset, pair_offset=pair_offset,
                                        maximum_glyph_height=max(heights)))
    result["text_flags"] = dict(flags)
    result["bundle_sha256"] = hashlib.sha256(data).hexdigest()
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=ROOT/"local/task0039/capture")
    parser.add_argument("--assets-only", action="store_true")
    args = parser.parse_args()
    if hashlib.sha256(args.elf.read_bytes()).hexdigest() != SHA:
        raise ValueError("Wrong authorized ELF")
    output = args.output.resolve()
    if not output.is_relative_to(ROOT/"local"):
        raise ValueError("Raw evidence must remain under ignored local/")
    output.mkdir(parents=True, exist_ok=True)
    manifest = []
    if not args.assets_only:
        client = Client()
        for address, size in ENTRIES.items():
            for action in ("decompile", "listing"):
                arguments = dict(file_name="SLUS_203.62", action=action, address=address)
                if action == "listing":
                    arguments.update(end_address=f"{int(address,16)+size-1:08x}", max_lines=1000)
                target = output/f"{address}.{action}.json"
                if not target.exists():
                    result = client.call("tools/call", dict(name="inspect", arguments=arguments))
                    if result.get("isError"):
                        raise RuntimeError(result)
                    target.write_text(json.dumps(result, indent=2), encoding="utf-8")
                manifest.append(dict(arguments=arguments, file=target.name,
                                     sha256=hashlib.sha256(target.read_bytes()).hexdigest()))
        for address, length in (("002f09c0",30),("002af1b0",16),("002af1a0",16)):
            arguments = dict(file_name="SLUS_203.62",action="read",address=address,length=length,format="hex")
            target = output/f"{address}.memory.json"
            if not target.exists():
                result = client.call("tools/call",dict(name="memory",arguments=arguments))
                if result.get("isError"):
                    raise RuntimeError(result)
                target.write_text(json.dumps(result,indent=2),encoding="utf-8")
            manifest.append(dict(arguments=arguments,file=target.name,
                                 sha256=hashlib.sha256(target.read_bytes()).hexdigest()))
    result = dict(task="TASK-0039", binary_sha256=SHA, revision="unknown", queries=manifest,
                  assets=census())
    (output/"manifest.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(dict(result="CAPTURED", queries=len(manifest), assets=result["assets"])))

if __name__ == "__main__":
    main()
