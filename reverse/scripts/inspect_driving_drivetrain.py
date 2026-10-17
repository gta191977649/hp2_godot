"""TASK-0038 drivetrain instruction capture; raw output stays under ignored local/."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.ghidra import Client
SHA = "7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2"
# Explicit listing bounds audited through each return delay slot; no inferred end sizes.
ENTRIES = {"0018abf0":"0018ac33", "0018bca0":"0018bcdb", "0018bce0":"0018c02b",
           "0018b360":"0018b49f", "0018b6c8":"0018b947", "0018b948":"0018ba9f",
           "0018baa0":"0018bc0b", "0018bc10":"0018bc17"}
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf",type=Path,required=True)
    parser.add_argument("--output",type=Path,default=ROOT/"local/task0038/drivetrain-ghidra")
    args=parser.parse_args()
    if hashlib.sha256(args.elf.read_bytes()).hexdigest()!=SHA: raise ValueError("Wrong ELF SHA-256")
    output=args.output.resolve()
    if not output.is_relative_to(ROOT/"local"): raise ValueError("Raw output must remain under ignored local/")
    output.mkdir(parents=True,exist_ok=True)
    client=Client(); manifest=[]
    for address,end in ENTRIES.items():
        for action in ["decompile","listing"]:
            arguments={"action":action,"address":address,"file_name":"SLUS_203.62"}
            if action=="listing": arguments.update(end_address=end,max_lines=1000)
            result=client.call("tools/call",{"name":"inspect","arguments":arguments})
            if result.get("isError"): raise RuntimeError(result)
            path=output/(address+"."+action+".json")
            path.write_text(json.dumps(result,indent=2),encoding="utf-8")
            manifest.append({"tool":"inspect","arguments":arguments,"file":path.name,
                             "sha256":hashlib.sha256(path.read_bytes()).hexdigest()})
    (output/"manifest.json").write_text(json.dumps({"binary_sha256":SHA,"program":"SLUS_203.62",
        "revision":"unknown","queries":manifest},indent=2),encoding="utf-8")
    print(json.dumps({"result":"CAPTURED","queries":len(manifest),"output":str(output)}))
if __name__=="__main__": main()
