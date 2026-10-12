"""Global entry point for locating an authorized local PS2 ZZDATA copy.

Never hardcode a ZZDATA path anywhere else in this repository. Scripts that need the
original disc assets must import this module and call `zzdata()` or `path(*parts)`.

Resolution order (first match wins):
  1. The HP2_PS2_ASSET_ROOT environment variable.
  2. config/ps2_assets.local.json ("root" key; gitignored, machine-specific).
See config/ps2_assets.example.json for the schema and setup instructions.
"""
import json
import os
from pathlib import Path

ENV_VAR = "HP2_PS2_ASSET_ROOT"
REPO_ROOT = Path(__file__).resolve().parents[1]
EXAMPLE_CONFIG = REPO_ROOT / "config/ps2_assets.example.json"
LOCAL_CONFIG = REPO_ROOT / "config/ps2_assets.local.json"


class Ps2AssetRootNotConfigured(RuntimeError):
    pass


def configured_root() -> Path:
    """Returns the configured root exactly as given, without validating its shape."""
    env = os.environ.get(ENV_VAR)
    if env:
        return Path(env)
    if LOCAL_CONFIG.exists():
        data = json.loads(LOCAL_CONFIG.read_text(encoding="utf-8"))
        root = data.get("root")
        if root:
            return Path(root)
    raise Ps2AssetRootNotConfigured(
        f"No PS2 asset root configured. Set the {ENV_VAR} environment variable, or copy "
        f"{EXAMPLE_CONFIG} to {LOCAL_CONFIG} and set its \"root\" to your own authorized "
        "ZZDATA copy.")


def zzdata() -> Path:
    """Returns the ZZDATA directory (containing FRONTEND/, GLOBAL/, CARS/, ...)."""
    root = configured_root()
    if (root / "FRONTEND").is_dir():
        return root
    if (root / "GAME" / "ZZDATA" / "FRONTEND").is_dir():
        return root / "GAME" / "ZZDATA"
    raise Ps2AssetRootNotConfigured(
        f"Configured root {root} does not contain a FRONTEND/ subfolder directly or under "
        "GAME/ZZDATA/; check config/ps2_assets.local.json or HP2_PS2_ASSET_ROOT.")


def path(*parts: str) -> Path:
    """Convenience: zzdata() joined with the given path parts, e.g. path('FRONTEND', 'FRONTB.BUN')."""
    return zzdata().joinpath(*parts)


if __name__ == "__main__":
    root = zzdata()
    print(f"ZZDATA root: {root}")
    for entry in sorted(root.iterdir()):
        print(f"  {entry.name}")
