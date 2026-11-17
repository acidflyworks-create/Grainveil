#!/usr/bin/env python3
"""Verify required glyphs exist in the embedded TTF Unicode cmaps (no OS fallback)."""
import pathlib
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
FONTS = [ROOT / "assets/fonts/Lato-Regular.ttf", ROOT / "assets/fonts/Lato-SemiBold.ttf"]
REQUIRED = set(range(0x20, 0x7f)) | {0x00b1, 0x2026}  # printable ASCII, ±, ellipsis


def u16(data, offset):
    return struct.unpack_from(">H", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from(">I", data, offset)[0]


def mappings(data):
    tables = {data[12 + 16*i:16 + 16*i]: u32(data, 20 + 16*i)
              for i in range(u16(data, 4))}
    cmap = tables[b"cmap"]
    offsets = set()
    for i in range(u16(data, cmap + 2)):
        pos = cmap + 4 + 8*i
        platform, encoding = u16(data, pos), u16(data, pos + 2)
        if platform == 0 or (platform == 3 and encoding in (1, 10)):
            offsets.add(cmap + u32(data, pos + 4))
    found = set()
    for pos in offsets:
        fmt = u16(data, pos)
        if fmt == 4:
            n = u16(data, pos + 6) // 2
            ends = pos + 14
            starts = ends + 2*n + 2
            deltas = starts + 2*n
            ranges = deltas + 2*n
            for code in REQUIRED:
                for i in range(n):
                    if u16(data, starts + 2*i) <= code <= u16(data, ends + 2*i):
                        delta, offset = u16(data, deltas + 2*i), u16(data, ranges + 2*i)
                        glyph = (code + delta) & 0xffff if offset == 0 else u16(data, ranges + 2*i + offset + 2*(code - u16(data, starts + 2*i)))
                        if offset and glyph:
                            glyph = (glyph + delta) & 0xffff
                        if glyph:
                            found.add(code)
                        break
        elif fmt == 12:
            for i in range(u32(data, pos + 12)):
                group = pos + 16 + i*12
                start, end, first = u32(data, group), u32(data, group + 4), u32(data, group + 8)
                found.update(code for code in REQUIRED if start <= code <= end and first + code - start != 0)
    return found


try:
    for font in FONTS:
        missing = REQUIRED - mappings(font.read_bytes())
        if missing:
            raise ValueError(f"{font.name}: missing " + ", ".join(f"U+{c:04X}" for c in sorted(missing)))
        print(f"{font.name}: printable ASCII, ± and ellipsis present in Unicode cmap")
except (OSError, ValueError, KeyError, struct.error) as exc:
    print(f"font glyph check failed: {exc}", file=sys.stderr)
    sys.exit(1)
