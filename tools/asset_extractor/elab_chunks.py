"""Generic reader for the EA "eLab" nested chunk container format used across HP2's PS2
ZZDATA bundles (confirmed by the literal embedded path "Global\\Pipeline\\eLabFrontEnd.bin"
inside FRONTEND/FRONTB.BUN; the same 8-byte-header chunk shape appears in every .BUN/.BIN/.tpk
file inspected so far: FRONTEND/TSELECT.BIN, GLOBAL/HUDTEX.BIN, GLOBAL/GLOBALB.BUN,
LANGUAGES/FE_ENGLISH.BIN).

Hypothesis (TASK-0012, derived from manual byte inspection, NOT yet independently verified
against any original tool or documentation -- treat every field name below as a port-side label,
not a recovered original identifier):

    struct Chunk {
        uint32_le tag;   // bit 0x80000000 set  => container: `size` bytes that follow are
                          //                        themselves a sequence of nested Chunks.
                          // bit 0x80000000 clear => leaf: `size` bytes of opaque payload
                          //                        (ASCII name string, floats, texture bytes, ...).
                          // (tag & 0x7fffffff) is observed to increment by exactly 1 for each
                          // chunk in a document in pre-order (container flag aside), i.e. it
                          // reads as a per-file chunk index rather than a semantic type id.
        uint32_le size;   // byte length of the payload/children that follow this header.
        uint8 data[size]; // present for both containers (nested Chunks) and leaves (opaque).
    };

This module only walks the tree and classifies chunks; it does not interpret leaf payloads
(texture pixel formats, layout/widget semantics, etc. are separate, later work).
"""
from __future__ import annotations
import argparse
import json
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import ps2_assets  # noqa: E402

CONTAINER_FLAG = 0x80000000
HEADER_SIZE = 8


@dataclass
class ChunkNode:
    tag: int
    index: int
    is_container: bool
    offset: int        # file offset of this chunk's 8-byte header
    size: int           # payload size in bytes (children, for a container)
    children: list = field(default_factory=list)
    ascii_guess: str | None = None  # best-effort printable-ASCII decode for small leaves

    def to_dict(self, max_depth: int | None = None, depth: int = 0) -> dict:
        row = {
            "tag": f"0x{self.tag:08x}",
            "index": self.index,
            "container": self.is_container,
            "offset": self.offset,
            "size": self.size,
        }
        if self.ascii_guess is not None:
            row["ascii_guess"] = self.ascii_guess
        if self.children and (max_depth is None or depth < max_depth):
            row["children"] = [c.to_dict(max_depth, depth + 1) for c in self.children]
        elif self.children:
            row["child_count"] = len(self.children)
        return row


def _printable_ascii(data: bytes, min_run: int = 4) -> str | None:
    """Best-effort: if `data` looks like a NUL-padded ASCII string, return the leading run."""
    end = data.find(b"\x00")
    candidate = data if end == -1 else data[:end]
    if len(candidate) < min_run:
        return None
    if all(32 <= b < 127 for b in candidate):
        return candidate.decode("ascii")
    return None


def parse_chunks(buffer: bytes, start: int, end: int) -> list[ChunkNode]:
    """Iteratively parses a flat sequence of sibling chunks in buffer[start:end]."""
    nodes: list[ChunkNode] = []
    pos = start
    while pos + HEADER_SIZE <= end:
        tag, size = struct.unpack_from("<II", buffer, pos)
        is_container = bool(tag & CONTAINER_FLAG)
        index = tag & ~CONTAINER_FLAG
        payload_start = pos + HEADER_SIZE
        payload_end = payload_start + size
        if payload_end > end:
            # Hypothesis breaks down here (payload runs past the parent's bound); stop and
            # report what we have rather than guessing/skipping silently.
            break
        node = ChunkNode(tag=tag, index=index, is_container=is_container, offset=pos, size=size)
        if is_container:
            node.children, _ = parse_chunks(buffer, payload_start, payload_end)
        else:
            node.ascii_guess = _printable_ascii(buffer[payload_start:payload_end])
        nodes.append(node)
        pos = payload_end
    return nodes, pos


def parse_chunks_top(buffer: bytes) -> tuple[list[ChunkNode], int]:
    nodes, end_pos = parse_chunks(buffer, 0, len(buffer))
    return nodes, end_pos


def _iter_all(nodes: list[ChunkNode]):
    for node in nodes:
        yield node
        if node.children:
            yield from _iter_all(node.children)


def summarize(nodes: list[ChunkNode], consumed: int, total: int) -> dict:
    all_nodes = list(_iter_all(nodes))
    containers = [n for n in all_nodes if n.is_container]
    leaves = [n for n in all_nodes if not n.is_container]
    strings = [(n.offset, n.ascii_guess) for n in leaves if n.ascii_guess]
    max_depth = 0

    def depth(node: ChunkNode, d: int) -> int:
        if not node.children:
            return d
        return max(depth(c, d + 1) for c in node.children)

    if nodes:
        max_depth = max(depth(n, 1) for n in nodes)
    return {
        "total_bytes": total,
        "consumed_bytes": consumed,
        "fully_consumed": consumed == total,
        "top_level_chunks": len(nodes),
        "total_chunks": len(all_nodes),
        "containers": len(containers),
        "leaves": len(leaves),
        "max_depth": max_depth,
        "ascii_strings_found": len(strings),
        "sample_strings": strings[:40],
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("relative_path", nargs="+",
                         help="Path parts under the configured ZZDATA root, e.g. FRONTEND FRONTB.BUN")
    parser.add_argument("--max-tree-depth", type=int, default=3,
                         help="Depth limit for the JSON tree dump (summary counts are unlimited)")
    parser.add_argument("--out", type=Path, default=None,
                         help="Write the full JSON tree here (default: tools/asset_extractor/output/<name>.chunks.json)")
    args = parser.parse_args()

    source = ps2_assets.path(*args.relative_path)
    buffer = source.read_bytes()
    nodes, consumed = parse_chunks_top(buffer)
    summary = summarize(nodes, consumed, len(buffer))

    print(f"source: {source}")
    print(json.dumps(summary, indent=2))

    out = args.out
    if out is None:
        out_dir = Path(__file__).resolve().parent / "output"
        out_dir.mkdir(parents=True, exist_ok=True)
        out = out_dir / (source.name + ".chunks.json")
    out.write_text(json.dumps({
        "source": str(source),
        "summary": summary,
        "tree": [n.to_dict(args.max_tree_depth) for n in nodes],
    }, indent=2), encoding="utf-8")
    print(f"tree written to {out} (depth-limited to {args.max_tree_depth}; summary counts are not limited)")


if __name__ == "__main__":
    main()
