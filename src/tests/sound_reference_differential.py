"""Explicitly requested external-reference differential, original outputs under ignored local/."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import wave

parser = argparse.ArgumentParser()
parser.add_argument("--native-runner", type=Path, required=True)
parser.add_argument("--reference-source", type=Path, required=True)
args = parser.parse_args()
repo = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(repo))
from tools.ps2_assets import path
sys.path.insert(0, str(args.reference_source))
from map_tools_ps2.sound_extract import _write_vag_wav, HP2_JBC_INTERLEAVE

out = repo / "local/task0016/audio-reference"
out.mkdir(parents=True, exist_ok=True)
source = path("SOUND", "JUKEBOXM.JBC")
with source.open("rb") as reader:
    reader.seek(0x60 + 5 * 96)  # original category0 preferred tune5 index table
    offset, size, sync = struct.unpack("<III", reader.read(12))
    assert sync == 0
    reader.seek(offset)
    vag = reader.read(size)
rate, channels, frames, interleave = _write_vag_wav(vag, out / "reference.wav", HP2_JBC_INTERLEAVE)
subprocess.run([str(args.native_runner.resolve()), "--dump-reference-pcm", str(out / "native.pcm")],
               cwd=repo, check=True)
with wave.open(str(out / "reference.wav"), "rb") as reader:
    reference = reader.readframes(reader.getnframes())
native = (out / "native.pcm").read_bytes()
assert native == reference, "native PCM differs from authorized map_tools_ps2 reference"
data_bytes = struct.unpack_from(">I", vag, 12)[0]
assert frames == data_bytes // (16 * channels) * 28
assert len(native) == frames * channels * 2
report = dict(result="PASS", source_member="SOUND/JUKEBOXM.JBC", tune=5,
              rate=rate, channels=channels, interleave=interleave, frames=frames,
              duration_seconds=frames / rate, pcm_bytes=len(native),
              pcm_sha256=hashlib.sha256(native).hexdigest(),
              reference_source=str(args.reference_source),
              original_hardware_comparison=False)
(out / "result.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
print(json.dumps(report))
