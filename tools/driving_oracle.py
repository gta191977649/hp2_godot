"""TASK-0038 read-only PINE capture. Requires an already-paused matching HP2.

No frame stepping, input injection, breakpoint or behavior equivalence is claimed.
Raw runtime/asset bytes are restricted to ignored local/ output.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools.oracle import Pine

SHA = "7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2"
WORLD = 0x330288  # gp A998; E-0010-01 / E-0038-004.

def read_block(pine, address, size):
    if address % 4 or size % 4 or not 0 <= address <= 0x2000000-size or not 0 < size <= 0x10000:
        raise ValueError("Capture supports bounded aligned physical EE RAM only")
    result = pine.request(b"".join(b"\x02"+struct.pack("<I", address+i) for i in range(0,size,4)))
    if len(result) != size:
        raise ValueError("Unexpected PINE memory reply length")
    return result

def word(data, offset):
    return struct.unpack_from("<I",data,offset)[0]

def text(pine, opcode):
    data=pine.request(bytes([opcode]));size=word(data,0)
    if size != len(data)-4:
        raise ValueError("Invalid PINE string reply")
    return data[4:].rstrip(b"\0").decode("utf-8")

def capture(pine, index):
    if pine.status() != "paused":
        raise RuntimeError("Pause PCSX2 before capturing; running reads are not synchronized")
    if text(pine,0xC) != "SLUS-20362":
        raise RuntimeError("Wrong game ID")
    # Static entry words corroborate address compatibility, not a RAM-wide ELF hash.
    if pine.read32(0x137C88) != 0x3C013F80 or pine.read32(0x137D90) != 0x27BDFF00:
        raise RuntimeError("Runtime code does not match the investigated entries")
    world=pine.read32(WORLD)
    if not world:raise RuntimeError("No active World object")
    header=read_block(pine,world,0x2c)
    count=word(header,0x1c)
    if not 0 < count <= 64 or not 0 <= index < count:
        raise ValueError("No requested vehicle in bounded World table")
    car=pine.read32(world+0x2c+index*4)
    blocks={}
    def add(name,address,size):
        data=read_block(pine,address,size)
        blocks[name]={"address":hex(address),"size":size,"hex":data.hex(),"sha256":hashlib.sha256(data).hexdigest()}
        return data
    add("world",world,0x2c+count*4)
    car_data=add("car",car,0x8a0)
    config=add("config",word(car_data,0xc),0x560)
    mover=word(car_data,0x30)
    mover_data=add("mover",mover,0x78)
    if word(car_data,0x24)!=1 or word(mover_data,4)!=0x2aeac8:
        raise RuntimeError("Requested car is not the evidenced PhysicsCar mode/table")
    add("controller",word(car_data,0x28),0x54)
    add("controls",word(car_data,0x2c),0x280)
    add("body",word(mover_data,0x18),0x180)
    for i in range(4):
        add("wheel_%d"%i,word(mover_data,0x30+i*4),0x190)
        add("wheel_controller_%d"%i,word(mover_data,0x20+i*4),0xb0)
    add("engine",word(mover_data,0x40),0x58)
    add("transmission",word(mover_data,0x44),0xa0)
    add("helper",word(mover_data,0x48),0x18)
    return {"world":hex(world),"car":hex(car),"index":index,"code":config[:32].split(b"\0")[0].decode("ascii"),
            "replay_object":hex(pine.read32(0x330148)),"blocks":blocks}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port",type=int,default=28011)
    parser.add_argument("--car-index",type=int,default=0)
    parser.add_argument("--output",type=Path,default=Path("local/task0038/pcsx2/paused.json"))
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    output=args.output.resolve()
    if not output.is_relative_to(root/"local"):
        raise ValueError("Raw captured bytes must stay under ignored local/")
    pine=Pine(args.port)
    first=capture(pine,args.car_index);second=capture(pine,args.car_index)
    if pine.status()!="paused" or first!=second:
        raise RuntimeError("Paused capture changed; refusing a stable-snapshot claim")
    result={"schema_version":1,"evidence_id":"E-0038-004","emulator":pine.version(),
            "game_id":text(pine,0xC),"game_crc":text(pine,0xD),"reported_game_version":text(pine,0xE),
            "target_elf_sha256":SHA,"verification_status":"not_tested",
            "capture_scope":"two identical paused snapshots; no instruction or frame boundary established",**first}
    output.parent.mkdir(parents=True,exist_ok=True);output.write_text(json.dumps(result,indent=2),encoding="utf-8")
    print(json.dumps({"result":"CAPTURED","code":result["code"],"car":result["car"],"blocks":len(result["blocks"]),
                      "output":str(output),"scope":result["capture_scope"]}))
if __name__=="__main__":main()
