"""TASK-0038 reproducible read-only Ghidra capture. Raw output stays under local/."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from tools.ghidra import Client
SHA="7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2"
ENTRIES={"00131410":484,"00131618":484,"00137100":844,"00137c88":176,"00137d90":3944,
         "00138e08":300,"00138f38":588,"00139188":776,"00152070":1252,"001a4e88":88,"0023a3d8":252}
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf",type=Path,required=True)
    parser.add_argument("--output",type=Path,default=ROOT/"local/task0038/ghidra")
    parser.add_argument("--endpoint",default="http://127.0.0.1:8080/mcp")
    args=parser.parse_args()
    if hashlib.sha256(args.elf.read_bytes()).hexdigest()!=SHA:raise ValueError("Wrong ELF SHA-256")
    output=args.output.resolve()
    if not output.is_relative_to(ROOT/"local"):raise ValueError("Raw output must remain under ignored local/")
    output.mkdir(parents=True,exist_ok=True);client=Client(args.endpoint);manifest=[]
    def collect(name,arguments,path):
        # Existing decompilations are reused; metadata/listings remain independent checks.
        if path.exists():result=json.loads(path.read_text(encoding="utf-8"))
        else:
            result=client.call("tools/call",{"name":name,"arguments":{"file_name":"SLUS_203.62",**arguments}})
            if result.get("isError"):raise RuntimeError(result)
            path.write_text(json.dumps(result,indent=2),encoding="utf-8")
        manifest.append({"tool":name,"arguments":arguments,"file":path.name,"sha256":hashlib.sha256(path.read_bytes()).hexdigest()})
    for address,size in ENTRIES.items():
        collect("inspect",{"action":"decompile","address":address},output/(address+".json"))
        collect("inspect",{"action":"listing","address":address,"end_address":hex(int(address,16)+size-1),"max_lines":1000},output/(address+".listing.json"))
        collect("functions",{"action":"get","address":address},output/(address+".function.json"))
    for address,size in {"0032dce0":4,"00330000":20,"002aeac8":40,"002be170":56}.items():
        collect("memory",{"action":"read","address":address,"length":size,"format":"hex"},output/(address+".memory.json"))
    (output/"manifest.json").write_text(json.dumps({"binary_sha256":SHA,"program":"SLUS_203.62","revision":"unknown","queries":manifest},indent=2),encoding="utf-8")
    print(json.dumps({"result":"CAPTURED","queries":len(manifest),"binary_sha256":SHA,"output":str(output)}))
if __name__=="__main__":main()
