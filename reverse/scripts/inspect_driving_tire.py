"""TASK-0038 read-only tire/trig capture; all raw output remains ignored."""
import argparse, hashlib, json
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from tools.ghidra import Client
SHA="7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2"
ENTRIES={"0025a540":160,"0025a5e0":44,"0025a748":324,"0025a890":324,
"001a4818":280,"001a4d28":352,"001a4930":1016,"001a4ee0":1216,
"0011b248":120,"0011b6e0":244,"0011b7d8":172,"0011b950":200,"0011ba18":72,
"001c0d78":376,"001a4170":472,"001a4580":40,"001a45a8":264,
"0025ab50":192,"0025aa78":212,"0025a300":108,"001a46b0":196,"001a63c0":16,
"0025afa8":40,"001a57e0":308,"001a3f00":12,"001c0cf8":80,"001c0d48":44}
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf",required=True,type=Path)
    parser.add_argument("--output",type=Path,default=ROOT/"local/task0038/tire")
    args=parser.parse_args()
    if hashlib.sha256(args.elf.read_bytes()).hexdigest()!=SHA:raise ValueError("Wrong authorized ELF")
    output=args.output.resolve()
    if not output.is_relative_to(ROOT/"local"):raise ValueError("Raw evidence must remain under local/")
    output.mkdir(parents=True,exist_ok=True);c=Client();manifest=[]
    def collect(tool,arguments,name):
        path=output/name
        if not path.exists():
            r=c.call("tools/call",{"name":tool,"arguments":{"file_name":"SLUS_203.62",**arguments}})
            if r.get("isError"):raise RuntimeError(r)
            path.write_text(json.dumps(r,indent=2),encoding="utf-8")
        manifest.append({"tool":tool,"arguments":arguments,"file":name,"sha256":hashlib.sha256(path.read_bytes()).hexdigest()})
    for address,size in ENTRIES.items():
        collect("inspect",{"action":"decompile","address":address},address+".json")
        collect("inspect",{"action":"listing","address":address,"end_address":hex(int(address,16)+size-1),"max_lines":1000},address+".listing.json")
        if address in {"001a46b0","001a63c0"}:
            collect("memory",{"action":"read","address":address,"length":size,"format":"hex"},address+".memory.json")
    for address,size in {"0032da20":96,"0032e640":120,"0032f1b4":112,"00330cb8":96}.items():
        collect("memory",{"action":"read","address":address,"length":size,"format":"hex"},address+".memory.json")
    (output/"manifest.json").write_text(json.dumps({"task":"TASK-0038","binary_sha256":SHA,"revision":"unknown","queries":manifest},indent=2),encoding="utf-8")
    print(json.dumps({"result":"CAPTURED","queries":len(manifest),"output":str(output)}))
if __name__=="__main__":main()
