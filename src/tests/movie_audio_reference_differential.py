"""Movie codec differential using original bytes and the user-authorized audio reference."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import wave

parser=argparse.ArgumentParser()
parser.add_argument("--native-runner",type=Path,required=True)
parser.add_argument("--reference-source",type=Path,required=True)
args=parser.parse_args()
repo=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(repo))
from tools.ps2_assets import path
sys.path.insert(0,str(args.reference_source))
from map_tools_ps2.sound_extract import _write_vag_wav
source=path("MOVIES","INTRO","EALOGO_EN.AVI")
data=source.read_bytes()
parts=[]
def chunks(start,end):
    while start+8<=end:
        tag=data[start:start+4]
        size=struct.unpack_from("<I",data,start+4)[0]
        payload=start+8
        assert payload+size<=end
        if tag==b"LIST":chunks(payload+4,payload+size)
        elif tag==b"01wb":parts.append(data[payload:payload+size])
        start=payload+size+(size&1)
chunks(12,struct.unpack_from("<I",data,4)[0]+8)
raw=b"".join(parts)
assert len(raw)==327680
# Envelope adapts raw data to the reference API; rate/channel/pitch came from ELF,
# not the misleading AVI carrier. No new sample bytes are synthesized.
header=bytearray(48)
header[:4]=b"VAGp"
struct.pack_into(">I",header,4,2)
struct.pack_into(">I",header,12,len(raw))
struct.pack_into(">I",header,16,48000)
header[36:40]=b"VAGx"
struct.pack_into(">I",header,44,2)
out=repo/"local/task0016/movie-audio-reference"
out.mkdir(parents=True,exist_ok=True)
rate,channels,frames,interleave=_write_vag_wav(bytes(header)+raw,out/"reference.wav",32768)
subprocess.run([str(args.native_runner.resolve()),"--dump-reference-pcm",str(out/"native.pcm")],
               cwd=repo,check=True)
with wave.open(str(out/"reference.wav"),"rb") as reader:
    reference=reader.readframes(reader.getnframes())
native=(out/"native.pcm").read_bytes()
assert native==reference
assert frames==len(raw)//32*28
report=dict(result="PASS",source_member="MOVIES/INTRO/EALOGO_EN.AVI",
            carrier_bytes=len(raw),host_rate=rate,channels=channels,interleave=interleave,
            frames=frames,spu_pitch=3762,pitch_scale=3762/4096,
            effective_rate=48000*3762/4096,pcm_bytes=len(native),
            pcm_sha256=hashlib.sha256(native).hexdigest(),original_hardware_comparison=False)
(out/"result.json").write_text(json.dumps(report,indent=2),encoding="utf-8")
print(json.dumps(report))
