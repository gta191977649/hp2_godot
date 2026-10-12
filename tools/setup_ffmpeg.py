"""Install the pinned LGPL shared Windows media backend under ignored local/tools.

No game assets are touched. The upstream latest URL is mutable: bytes must match
the recorded archive SHA-256 or installation stops. Other OS packages are not pinned.
"""
import hashlib
from pathlib import Path
import urllib.request
import zipfile

URL = "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-n8.1-latest-win64-lgpl-shared-8.1.zip"
SHA256 = "ec30bde63b8e36ebdc7ba5b37140a43a2d2afe884650269fadeb033a57098cfc"
ROOT = Path(__file__).resolve().parents[1] / "local" / "tools" / "ffmpeg"


def main():
    ROOT.mkdir(parents=True, exist_ok=True)
    archive = ROOT / "ffmpeg-n8.1-win64-lgpl-shared.zip"
    if not archive.exists():
        urllib.request.urlretrieve(URL, archive)
    actual = hashlib.sha256(archive.read_bytes()).hexdigest()
    if actual != SHA256:
        raise RuntimeError(f"FFmpeg archive hash mismatch: {actual}; expected {SHA256}. Preserve current pin; do not load this archive.")
    with zipfile.ZipFile(archive) as package:
        for member in package.infolist():
            if not (ROOT / member.filename).resolve().is_relative_to(ROOT.resolve()):
                raise RuntimeError("FFmpeg package member escapes install directory")
        package.extractall(ROOT)
    print(ROOT / "ffmpeg-n8.1-latest-win64-lgpl-shared-8.1")


if __name__ == "__main__":
    main()
