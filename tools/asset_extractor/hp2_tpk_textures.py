"""Decodes P8/P4 (indexed) and RGBA textures from HP2's PS2 "eLab"/.tpk texture packages
(FRONTEND/*.BIN, GLOBAL/*TEX*.BIN, TRACKS/TEX*.BIN, etc.: any file whose top-level elab_chunks
container is tagged TPK_HP2_MAIN_CHUNKID).

TASK-0012. The chunk-container format (elab_chunks.py) was independently derived and validated
in this repo by exact full-file consumption. The *pixel* decode below is NOT independently
derived: it is ported, function-for-function, from an existing, already-validated local project
by the same user — D:\\dev\\eagl-godot\\map_tools_ps2\\src\\map_tools_ps2\\textures.py — because an
earlier from-scratch attempt in this repo got the chunk-header offsets right but missed a 0x80
alignment step on the pixel/palette data-pool base offset, producing a fine but visible garbled
artifact on every decoded texture. That project's own documentation
(D:\\dev\\eagl-godot\\docs\\hp2_ps2_texture_decoding.md) in turn credits Modelulator's legacy PS2
texture path (eTexturePack.lua / PSX2.lua) as the original source of the swizzle/RWBuffer
algorithm, and explicitly warns that a simplified linear/simple-swizzle decode produces visible
diagonal color bands on real HP2 textures -- which matches exactly what TASK-0012's first attempt
produced. Do not re-simplify this path; port changes from the reference, not from memory.

Verification performed in this repo after porting: GLOBAL/HUDTEX.BIN's "VIPERGTS_TACH" (a 128x128
P8 tachometer gauge) decodes to a clean, artifact-free image with legible "RPM X1000" text and
tick marks, where the prior unaligned attempt showed a fine speckle pattern over the same content.
"""
from __future__ import annotations
import argparse
import struct
import sys
from array import array
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import ps2_assets  # noqa: E402
import elab_chunks  # noqa: E402

TPK_HP2_MAIN_CHUNKID = 0xB0300001
TPK_HP2_CHUNK2_CHUNKID = 0x30300002  # name/source-path leaf
TPK_HP2_CHUNK3_CHUNKID = 0x30300003  # texture entry table, 0xA4 bytes each
TPK_HP2_CHUNK4_CHUNKID = 0x30300004  # shared pixel+palette data pool, indexed by the entries
ENTRY_SIZE = 0xA4


def align(value: int, boundary: int) -> int:
    """From map_tools_ps2/src/map_tools_ps2/binary.py (power-of-two alignment)."""
    return (value + boundary - 1) & ~(boundary - 1)


@dataclass
class TpkTextureHeader:
    name: str
    tex_hash: int
    width: int
    height: int
    bit_depth: int
    image_placement: int
    palette_placement: int
    image_size: int
    palette_size: int
    shift_width: int
    shift_height: int
    pixel_storage_mode: int
    clut_pixel_storage_mode: int
    texture_fx: int
    is_any_semitransparency: int
    is_swizzled: bool
    alpha_bits: int
    alpha_fix: int


def _decode_name(raw: bytes) -> str:
    return raw.split(b"\x00")[0].decode("ascii", errors="replace")


def parse_header(entry: bytes) -> TpkTextureHeader:
    """Field offsets from map_tools_ps2's read_ps2_tpk_bytes / docs/hp2_ps2_texture_decoding.md."""
    name = _decode_name(entry[0x08:0x20])
    tex_hash = struct.unpack_from("<I", entry, 0x20)[0]
    width, height = struct.unpack_from("<HH", entry, 0x24)
    bit_depth = entry[0x28]
    image_placement = struct.unpack_from("<I", entry, 0x30)[0]
    palette_placement = struct.unpack_from("<I", entry, 0x34)[0]
    image_size = struct.unpack_from("<I", entry, 0x38)[0]
    palette_size = struct.unpack_from("<I", entry, 0x3C)[0]
    shift_width = entry[0x48]
    shift_height = entry[0x49]
    pixel_storage_mode = entry[0x4A]
    clut_pixel_storage_mode = entry[0x4B]
    texture_fx = entry[0x4E]
    is_any_semitransparency = entry[0x4F]
    is_swizzled = entry[0x55] != 0
    alpha_bits = entry[0x76]
    alpha_fix = entry[0x77]
    return TpkTextureHeader(
        name=name, tex_hash=tex_hash, width=width, height=height, bit_depth=bit_depth,
        image_placement=image_placement, palette_placement=palette_placement,
        image_size=image_size, palette_size=palette_size,
        shift_width=shift_width, shift_height=shift_height,
        pixel_storage_mode=pixel_storage_mode, clut_pixel_storage_mode=clut_pixel_storage_mode,
        texture_fx=texture_fx, is_any_semitransparency=is_any_semitransparency,
        is_swizzled=is_swizzled, alpha_bits=alpha_bits, alpha_fix=alpha_fix)


def find_hp2_tpk_sections(buffer: bytes) -> tuple[bytes, bytes, int]:
    """Returns (entry_table_bytes, full_buffer, data_base) for the first TPK_HP2_MAIN container.

    data_base is the 0x80-aligned absolute offset of chunk4's payload in `buffer`; every
    image/palette placement in an entry is relative to data_base (map_tools_ps2 textures.py:
    `data_base = align(data_chunk.data_offset, 0x80)`). Returning the full buffer plus data_base,
    rather than a pre-sliced pool, avoids re-introducing the alignment bug by construction.
    """
    nodes, _consumed = elab_chunks.parse_chunks_top(buffer)
    for node in nodes:
        if node.tag != TPK_HP2_MAIN_CHUNKID or not node.is_container:
            continue
        entries = data_offset = None
        for child in node.children:
            if child.tag == TPK_HP2_CHUNK3_CHUNKID:
                entries = buffer[child.offset + 8: child.offset + 8 + child.size]
            elif child.tag == TPK_HP2_CHUNK4_CHUNKID:
                data_offset = child.offset + 8
        if entries is not None and data_offset is not None:
            return entries, buffer, align(data_offset, 0x80)
    raise ValueError("No TPK_HP2_MAIN_CHUNKID container with both an entry table (chunk3) "
                      "and a data pool (chunk4) was found in this file.")


def read_all_headers(entries_bytes: bytes) -> list[TpkTextureHeader]:
    if len(entries_bytes) % ENTRY_SIZE != 0:
        raise ValueError(f"Entry table size {len(entries_bytes)} is not a multiple of "
                          f"0x{ENTRY_SIZE:x}; the chunk3-is-an-entry-table hypothesis may not "
                          "hold for this file.")
    count = len(entries_bytes) // ENTRY_SIZE
    return [parse_header(entries_bytes[i * ENTRY_SIZE:(i + 1) * ENTRY_SIZE]) for i in range(count)]


# ---------------------------------------------------------------------------
# Pixel decode: ported verbatim (logic unchanged) from
# D:\dev\eagl-godot\map_tools_ps2\src\map_tools_ps2\textures.py. Keep this block's control flow
# identical to that source; port future fixes from there rather than editing the math here from
# first principles. Only renamed to avoid clashing with this file's own names.
# ---------------------------------------------------------------------------

def decode_indexed_texture(width, height, image, palette, *, bit_depth, shift_width,
                            shift_height, pixel_storage_mode, is_swizzled):
    return _decode_modelulator_indexed_texture(
        width, height, image, palette, bit_depth=bit_depth, shift_width=shift_width,
        shift_height=shift_height, pixel_storage_mode=pixel_storage_mode, is_swizzled=is_swizzled)


def _decode_modelulator_indexed_texture(width, height, image, palette, *, bit_depth,
                                         shift_width, shift_height, pixel_storage_mode,
                                         is_swizzled):
    depth = _indexed_bit_depth(bit_depth, pixel_storage_mode, len(palette))
    if depth not in (4, 8):
        raise ValueError(f"unsupported indexed PS2 bit depth {depth}")

    color_count = len(palette) // 4
    if color_count <= 0:
        raise ValueError("missing PS2 palette")
    colors = _decode_palette(palette, swizzle=_psm_type_index(pixel_storage_mode) == 3)

    buffer_width = 1 << shift_width
    buffer_height = 1 << shift_height
    if width <= 0 or height <= 0 or buffer_width <= 0 or buffer_height <= 0:
        raise ValueError("invalid PS2 texture dimensions")

    words = _u32_words(image)
    if is_swizzled:
        scale = 32 // depth
        scale_mask = scale - 1
        scale_x = (_adjust_with_mask(scale_mask, 1, 2, 1) | _adjust_with_mask(scale_mask, 1, 0, 0)) + 1
        scale_y = (_adjust_with_mask(scale_mask, 1, 3, 1) | _adjust_with_mask(scale_mask, 1, 1, 0)) + 1
        words = _legacy_ps2_rw_buffer(words, "write", 0, 32, width // scale_y, height // scale_x)
        words = _legacy_ps2_rw_buffer(words, "read", pixel_storage_mode, depth, width, height)

    indices: list[int] = []
    scale = 32 // depth
    index_mask = (1 << depth) - 1
    for word in words:
        for shift in range(0, 32, depth):
            indices.append((word >> shift) & index_mask)

    if is_swizzled:
        cropped_indices: list[int] = []
        for y in range(height):
            row = y * buffer_width
            cropped_indices.extend(indices[row: row + width])
        indices = cropped_indices

    final_indices: list[int] = []
    for y in range(height - 1, -1, -1):
        row = y * width
        final_indices.extend(indices[row: row + width])

    out = bytearray()
    for index in final_indices[: width * height]:
        out.extend(colors[index % color_count])
    return bytes(out)


def _indexed_bit_depth(bit_depth, pixel_storage_mode, palette_size):
    psm_index = _psm_type_index(pixel_storage_mode)
    if psm_index > 0:
        return 32 >> max(psm_index - 1, 0)
    if bit_depth in (4, 8):
        return bit_depth
    if palette_size == 0x40:
        return 4
    if palette_size in (0x80, 0x400):
        return 8
    raise ValueError(f"unsupported PS2 palette size 0x{palette_size:x}")


def _psm_type_index(pixel_storage_mode: int) -> int:
    return pixel_storage_mode & 0x07


def _u32_words(data: bytes) -> list[int]:
    padded = data + (b"\0" * ((4 - (len(data) % 4)) % 4))
    return [struct.unpack_from("<I", padded, offset)[0] for offset in range(0, len(padded), 4)]


@lru_cache(maxsize=64)
def _legacy_ps2_rw_plan(mode, pixel_storage_mode, bit_depth, width, height):
    scale = 32 // bit_depth
    scale_mask = scale - 1
    scale_x = (_adjust_with_mask(scale_mask, 1, 2, 1) | _adjust_with_mask(scale_mask, 1, 0, 0)) + 1
    scale_y = (_adjust_with_mask(scale_mask, 1, 3, 1) | _adjust_with_mask(scale_mask, 1, 1, 0)) + 1

    physical_width = width // scale_y
    physical_height = height // scale_x
    physical_buffer_width = _align_power_of_two_max(physical_width)
    physical_buffer_height = _align_power_of_two_max(physical_height)
    buffer_width = _align_power_of_two_max(width)
    buffer_height = _align_power_of_two_max(height)
    output_length = physical_buffer_width * physical_buffer_height

    type_index = _adjust_with_mask(pixel_storage_mode, 3, 0)
    type_mode = _adjust_with_mask(pixel_storage_mode, 2, 4)
    type_flag = _adjust_with_mask(pixel_storage_mode, 1, 3) != 0

    swap_xy = (((type_mode == 0) or (type_mode == 3)) and type_index == 2) or (type_mode == 1 and type_index == 4)
    z_buffer = type_mode == 3
    shifted = ((type_mode == 0) or (type_mode == 3)) and type_index == 2 and type_flag

    column_width = 8 * scale_x
    column_height = 2 * scale_y
    page_height = 1 << (1 if swap_xy else 0)
    page_width = (page_height ^ 0x03) << 2
    page_height <<= 2
    texture_buffer_width = width // (page_width * column_width)
    if texture_buffer_width <= 0:
        texture_buffer_width = 1

    if mode not in ("read", "write"):
        raise ValueError(f"unsupported PS2 buffer mode {mode}")

    swizzle_function = _LEGACY_SWIZZLE_FUNCTIONS[bit_depth]
    input_address = 0
    from_offset_w = 0
    targets, sources = array("i"), array("i")
    source_shifts, target_shifts = array("i"), array("i")

    for index in range(buffer_width * buffer_height):
        y = index // buffer_width
        x = index - y * buffer_width

        page_x = x // (page_width * column_width)
        page_y = y // (page_height * 4 * column_height)
        page = page_x + page_y * texture_buffer_width

        px = x - page_x * (page_width * column_width)
        py = y - page_y * (page_height * 4 * column_height)
        block_x = px // column_width
        block_y = py // (4 * column_height)
        block = _legacy_ps2_block_address(block_x + block_y * page_width, swap_xy, z_buffer, shifted)

        bx = px - block_x * column_width
        by = py - block_y * (4 * column_height)
        column_y = by // column_height
        column = column_y

        cx = bx
        cy = by - column_y * column_height
        pixel = swizzle_function(cx + cy * column_width, True)

        word = pixel // scale
        offset = pixel & scale_mask
        if bit_depth < 16:
            word ^= (column & 0x01) << 2
        word = (_rotate_bits(word >> 1, -1, 3) << 1) | (word & 0x01)

        output_address = (page << 11) | (block << 6) | (column << 4) | word
        if mode == "read":
            address_a, address_b = input_address, output_address
            source_shift = bit_depth * offset
            target_shift = bit_depth * from_offset_w
        else:
            address_a, address_b = output_address, input_address
            source_shift = bit_depth * from_offset_w
            target_shift = bit_depth * offset

        targets.append(address_a)
        sources.append(address_b)
        source_shifts.append(source_shift)
        target_shifts.append(target_shift)

        from_offset_w += 1
        if from_offset_w > 0 and (from_offset_w & scale_mask) == 0:
            input_address += 1
        from_offset_w &= scale_mask

    return targets, sources, source_shifts, target_shifts, output_length


def _legacy_ps2_rw_buffer(image_data, mode, pixel_storage_mode, bit_depth, width, height):
    targets, sources, source_shifts, target_shifts, output_length = _legacy_ps2_rw_plan(
        mode, pixel_storage_mode, bit_depth, width, height)
    data = [0] * output_length
    mask = (1 << bit_depth) - 1
    available = len(image_data)
    for address_a, address_b, source_shift, target_shift in zip(targets, sources, source_shifts, target_shifts):
        if not 0 <= address_a < output_length:
            continue
        input_value = image_data[address_b] if 0 <= address_b < available else 0
        data[address_a] |= ((input_value >> source_shift) & mask) << target_shift
    return data


def _adjust_with_mask(src, mask_width, mask_position=0, adjustment=0):
    return ((src >> mask_position) & ((1 << mask_width) - 1)) << adjustment


def _rotate_bits(value, shift, width):
    shift %= width
    mask = (1 << width) - 1
    value &= mask
    return ((value << shift) | (value >> (width - shift))) & mask


def _align_power_of_two_max(value):
    if value <= 1:
        return 1
    return 1 << (value - 1).bit_length()


def _legacy_ps2_swizzle_psmt4(pixel_index, mode_flag):
    ax, ay = (pixel_index >> 0) & 0x01, (pixel_index >> 1) & 0x01
    bx, by = (pixel_index >> 2) & 0x01, (pixel_index >> 3) & 0x01
    cx, cy = (pixel_index >> 4) & 0x01, (pixel_index >> 5) & 0x01
    dx = (pixel_index >> 6) & 0x01
    result = 0
    result ^= dx << 0
    result ^= by << 1
    result ^= cx << 2
    result ^= ax << 3
    result ^= ay << 4
    result ^= bx << 5
    result ^= ax << 7
    result ^= cy << (3 + 3 * int(mode_flag))
    result >>= 0 if mode_flag else 1
    result ^= dx << 5
    return result & 0x7F


def _legacy_ps2_swizzle_psmt8(pixel_index, mode_flag):
    ax, ay = (pixel_index >> 0) & 0x01, (pixel_index >> 1) & 0x01
    bx, by = (pixel_index >> 2) & 0x01, (pixel_index >> 3) & 0x01
    cx, cy = (pixel_index >> 4) & 0x01, (pixel_index >> 5) & 0x01
    result = 0
    result ^= ax << 0
    result ^= cy << 1
    result = _rotate_bits(result, 5 + int(mode_flag), 7)
    result ^= bx << (0 + 4 * int(mode_flag))
    result ^= cx << (2 + 3 * int(mode_flag))
    result ^= by << 1
    result ^= ax << 2
    result ^= ay << 3
    result ^= cy << 4
    return result & 0x3F


def _legacy_ps2_swizzle_psmt16(pixel_index, mode_flag):
    ax, ay = (pixel_index >> 0) & 0x01, (pixel_index >> 1) & 0x01
    bx, by = (pixel_index >> 2) & 0x01, (pixel_index >> 3) & 0x01
    cx = (pixel_index >> 4) & 0x01
    result = 0
    result ^= ay << 0
    result ^= bx << 1
    result ^= by << 2
    result ^= ax << 3
    result = _rotate_bits(result, 2 * int(mode_flag), 4)
    result ^= cx << 4
    return result & 0x1F


def _legacy_ps2_swizzle_psmt32(pixel_index, mode_flag):
    ax, ay = (pixel_index >> 0) & 0x01, (pixel_index >> 1) & 0x01
    bx, by = (pixel_index >> 2) & 0x01, (pixel_index >> 3) & 0x01
    result = 0
    result ^= ay << 0
    result ^= bx << 1
    result ^= by << 2
    result = _rotate_bits(result, 2 + int(mode_flag), 3)
    result = (result << 1) ^ ax
    return result & 0x0F


_LEGACY_SWIZZLE_FUNCTIONS = {
    4: _legacy_ps2_swizzle_psmt4,
    8: _legacy_ps2_swizzle_psmt8,
    16: _legacy_ps2_swizzle_psmt16,
    24: _legacy_ps2_swizzle_psmt32,
    32: _legacy_ps2_swizzle_psmt32,
}


def _legacy_ps2_block_address(block_index, swap_xy, flip_xy, shifted):
    swap = int(swap_xy)
    swap = (swap << 0) ^ (swap << 1)
    block_index = _rotate_bits(block_index, swap, 5)
    ax, ay = (block_index >> 0) & 0x01, (block_index >> 3) & 0x01
    bx, by = (block_index >> 1) & 0x01, (block_index >> 4) & 0x01
    cx = (block_index >> 2) & 0x01
    result = 0
    result ^= bx << 0
    result ^= by << 1
    result ^= cx << 2
    result = _rotate_bits(result, int(shifted), 3)
    result ^= (0x03 * int(flip_xy)) << 1
    result = (result << 2) ^ (ax << 0) ^ (ay << 1)
    return result & 0x1F


def _decode_palette(palette: bytes, swizzle: bool) -> list[tuple[int, int, int, int]]:
    colors: list[tuple[int, int, int, int]] = []
    for index in range(len(palette) // 4):
        source_index = _unswizzle_palette_index(index) if swizzle else index
        off = source_index * 4
        r, g, b, a = palette[off: off + 4]
        colors.append((r, g, b, _decode_ps2_alpha(a)))
    return colors


def _decode_ps2_alpha(value: int) -> int:
    expanded = max((value << 1) - ((value ^ 1) & 0x01), 0)
    if expanded <= 0xFF:
        return expanded
    return value


def _unswizzle_palette_index(index: int) -> int:
    block = index & ~0x1F
    pos = index & 0x1F
    if 8 <= pos < 16:
        pos += 8
    elif 16 <= pos < 24:
        pos -= 8
    return block + pos


# ---------------------------------------------------------------------------
# End of ported block.
# ---------------------------------------------------------------------------


def _flip_rgba_vertical(rgba: bytes, width: int, height: int) -> bytes:
    """From map_tools_ps2's textures.py; used there for the frontend-texture export path
    specifically (export_frontend_textures passes flip_vertical=True), because
    `_decode_modelulator_indexed_texture` already performs its own internal row reversal for the
    3D-mesh UV convention -- for standalone viewing, that reversal needs undoing again."""
    row_size = width * 4
    return b"".join(rgba[row * row_size:(row + 1) * row_size] for row in range(height - 1, -1, -1))


def decode_texture(header: TpkTextureHeader, buffer: bytes, data_base: int, *, flip_vertical: bool = True) -> Image.Image:
    image = buffer[data_base + header.image_placement: data_base + header.image_placement + header.image_size]
    palette = buffer[data_base + header.palette_placement: data_base + header.palette_placement + header.palette_size]
    if not image or not palette:
        raise ValueError(f"{header.name}: empty image or palette slice at the computed offsets")
    rgba = decode_indexed_texture(
        header.width, header.height, image, palette,
        bit_depth=header.bit_depth, shift_width=header.shift_width, shift_height=header.shift_height,
        pixel_storage_mode=header.pixel_storage_mode, is_swizzled=header.is_swizzled)
    if flip_vertical:
        rgba = _flip_rgba_vertical(rgba, header.width, header.height)
    img = Image.new("RGBA", (header.width, header.height))
    img.frombytes(rgba)
    return img


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("relative_path", nargs="+",
                         help="Path parts under the configured ZZDATA root, e.g. FRONTEND FRONTB.BUN")
    parser.add_argument("--out-dir", type=Path, default=None,
                         help="Where to write decoded PNGs (default: tools/asset_extractor/output/<name>_textures/)")
    parser.add_argument("--limit", type=int, default=None, help="Only decode the first N textures")
    args = parser.parse_args()

    source = ps2_assets.path(*args.relative_path)
    buffer = source.read_bytes()
    entries_bytes, buffer, data_base = find_hp2_tpk_sections(buffer)
    headers = read_all_headers(entries_bytes)
    print(f"source: {source}")
    print(f"{len(headers)} texture entries found; data_base=0x{data_base:x}")

    out_dir = args.out_dir
    if out_dir is None:
        out_dir = Path(__file__).resolve().parent / "output" / (source.stem + "_textures")
    out_dir.mkdir(parents=True, exist_ok=True)

    decoded, skipped = 0, 0
    for header in headers:
        if args.limit is not None and decoded >= args.limit:
            break
        if not header.name or not header.width or not header.height:
            skipped += 1
            continue
        try:
            image = decode_texture(header, buffer, data_base)
        except ValueError as exc:
            print(f"  FAILED {header.name!r}: {exc}")
            skipped += 1
            continue
        safe_name = "".join(c if c.isalnum() or c in "_-" else "_" for c in header.name) or f"hash_{header.tex_hash:08x}"
        out_path = out_dir / f"{safe_name}_{header.tex_hash:08x}.png"
        image.save(out_path)
        print(f"  decoded {header.name!r} ({header.width}x{header.height}) -> {out_path}")
        decoded += 1

    print(f"decoded {decoded}, skipped {skipped}")


if __name__ == "__main__":
    main()
