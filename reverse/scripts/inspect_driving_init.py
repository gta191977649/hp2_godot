"""TASK-0038 init dependency capture; read-only Ghidra, raw output stays ignored."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.ghidra import Client

SHA = "7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2"
ENTRIES = ["0025ac68", "0025ac98", "0019cc58", "0019c9b0", "001a3f10", "001a5ea0",
           "0019c170", "0019cc40", "001a5f88", "001a4348", "001a4158", "001a5fb0",
           "0019c238", "00137788", "00137938", "0018a7e8", "0018ae70", "00138cf8",
           "0019c808", "001adfb8", "001377d8", "001375f0"]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=ROOT / "local/task0038/init-ghidra")
    args = parser.parse_args()
    if hashlib.sha256(args.elf.read_bytes()).hexdigest() != SHA:
        raise ValueError("Wrong ELF SHA-256")
    output = args.output.resolve()
    if not output.is_relative_to(ROOT / "local"):
        raise ValueError("Raw output must remain under ignored local/")
    output.mkdir(parents=True, exist_ok=True)
    client = Client()
    manifest = []
    def collect(name, arguments, filename):
        result = client.call("tools/call", {"name": name, "arguments": {"file_name": "SLUS_203.62", **arguments}})
        if result.get("isError"):
            raise RuntimeError(result)
        path = output / filename
        path.write_text(json.dumps(result, indent=2), encoding="utf-8")
        manifest.append({"tool": name, "arguments": arguments, "file": filename,
                         "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
        return result.get("structuredContent") or json.loads(result["content"][0]["text"])
    for address in ENTRIES:
        info = collect("functions", {"action": "get", "address": address}, address + ".function.json")
        collect("inspect", {"action": "decompile", "address": address}, address + ".decompile.json")
        collect("inspect", {"action": "listing", "address": address, "end_address": info["end_address"],
                            "max_lines": 1000}, address + ".listing.json")
    (output / "manifest.json").write_text(json.dumps({"binary_sha256": SHA, "revision": "unknown",
                                                    "program": "SLUS_203.62", "queries": manifest}, indent=2), encoding="utf-8")
    print(json.dumps({"result": "CAPTURED", "queries": len(manifest), "output": str(output), "binary_sha256": SHA}))

if __name__ == "__main__":
    main()
