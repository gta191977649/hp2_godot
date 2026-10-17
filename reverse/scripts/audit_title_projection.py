"""TASK-0043: bounded, read-only evidence for the extracted title selector."""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from ghidra import Client


def main():
    out = ROOT / "local/task0043/title"
    out.mkdir(parents=True, exist_ok=True)
    client = Client()
    queries = [
        ("debug-symbols", "symbols", "NFSHP2.ELF", dict(action="list", name_pattern=".*(EventTree|TitleString).*", page_size=500)),
        ("debug-constructor", "inspect", "NFSHP2.ELF", dict(action="decompile", address="0x001F5060")),
        ("debug-constructor-symbol", "symbols", "NFSHP2.ELF", dict(action="get", address="0x001F5060")),
        ("debug-callback", "inspect", "NFSHP2.ELF", dict(action="decompile", address="0x001F6898")),
        ("retail-decompile", "inspect", "SLUS_203.62", dict(action="decompile", address="0x001F96B8")),
        ("retail-references", "inspect", "SLUS_203.62", dict(action="references_to", address="0x001F96B8", page_size=100)),
        ("retail-callback", "inspect", "SLUS_203.62", dict(action="decompile", address="0x001FB218")),
    ]
    for label, tool, program, arguments in queries:
        arguments["file_name"] = program
        result = client.call("tools/call", dict(name=tool, arguments=arguments))
        if result.get("isError"):
            raise RuntimeError(result)
        (out / (label + ".json")).write_text(json.dumps(dict(query=arguments, result=result), indent=2), encoding="utf-8")
    # No source edits or automatic symbol promotion are performed.
    print(json.dumps(dict(evidence_id="E-0043-T01", raw_directory=str(out), conclusion="retain_address_anchored_projection", queries=len(queries))))


if __name__ == "__main__":
    main()
