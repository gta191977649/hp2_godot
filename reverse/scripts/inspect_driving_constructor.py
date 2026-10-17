"""TASK-0038 constructor/reset/static/schedule evidence; read-only, private output."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.ghidra import Client

SHA = "7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2"
RANGES = {"00137100":844,"001375f0":280,"00138cf8":272,"00137788":80,"00137938":844,
          "001377d8":352,"00133130":92,"00133070":108,"0019c238":84,"0019c808":124,
          "001adfb8":32,"001a4358":336,"001a5fc0":880,"001a63d0":52,"001a6408":216,
          "0019cf10":232,"0019dcc8":8,"001a5c28":8,"001a6660":8,"0018c368":8,"0018c1c8":8,
          "0019da30":140,"001395b0":88,"0018f9e0":16,"001a33a8":24,"001a33c0":48,
          "00136dd0":48,"001a33f0":268,"001a3500":148,"001a3598":72,"001a35e0":12,"001a35f0":204}
MEMORY = {"002b5430":72,"002b5c40":72,"002b5cd0":72,"002b4618":72,"002b45d0":72,
          "002aeaa8":32,"0032e628":28,"0032dd48":12,"00310dc0":132,"0032e5f4":52,
          "00330c50":80,"0032dbc0":4}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf",type=Path,required=True)
    parser.add_argument("--output",type=Path,default=ROOT/"local/task0038/constructor2")
    args=parser.parse_args()
    if hashlib.sha256(args.elf.read_bytes()).hexdigest()!=SHA:
        raise ValueError("Wrong ELF SHA-256")
    output=args.output.resolve()
    if not output.is_relative_to(ROOT/"local"):
        raise ValueError("Private raw results must remain under ignored local/")
    output.mkdir(parents=True,exist_ok=True)
    client=Client();manifest=[]
    def collect(tool,arguments,filename):
        result=client.call("tools/call",{"name":tool,"arguments":{"file_name":"SLUS_203.62",**arguments}})
        if result.get("isError"):raise RuntimeError(result)
        path=output/filename;path.write_text(json.dumps(result,indent=2),encoding="utf-8")
        manifest.append({"tool":tool,"arguments":arguments,"file":filename,
                         "sha256":hashlib.sha256(path.read_bytes()).hexdigest()})
        return result.get("structuredContent") or json.loads(result["content"][0]["text"])
    for address,size in RANGES.items():
        collect("inspect",{"action":"listing","address":address,
                           "end_address":hex(int(address,16)+size-1),"max_lines":1000},address+".listing.json")
    for address,size in MEMORY.items():
        collect("memory",{"action":"read","address":address,"length":size,"format":"hex"},address+".memory.json")
    search=collect("memory",{"action":"search","search_type":"hex","search_value":"98 b3",
                             "address_start":"00100000","address_end":"002d0000","page_size":500},"timing-search.json")
    # Retain a bounded exhaustive low-immediate search; then decode GP-addressed stores.
    matches=[]
    for match in search["data"]:
        address=int(match["address"],16)
        if address%4:continue
        data=collect("memory",{"action":"read","address":hex(address),"length":4,"format":"hex"},f"timing-{address:08x}.json")
        word=int.from_bytes(bytes.fromhex(data["hex_data"]),"little")
        matches.append({"address":hex(address),"word":hex(word),"gp":((word>>21)&31)==28,
                        "store":word>>26 in (0x28,0x29,0x2b,0x39,0x3f)})
    (output/"timing-decoded.json").write_text(json.dumps(matches,indent=2),encoding="utf-8")
    (output/"manifest.json").write_text(json.dumps({"binary_sha256":SHA,"program":"SLUS_203.62",
        "revision":"unknown","queries":manifest},indent=2),encoding="utf-8")
    print(json.dumps({"result":"CAPTURED","queries":len(manifest),"output":str(output),"binary_sha256":SHA}))

if __name__=="__main__":main()
