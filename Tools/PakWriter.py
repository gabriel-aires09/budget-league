#!/usr/bin/env python3
"""Writes the .pak archive: every cooked asset in one file.

The engine reads it in Game/Source/AssetPack.cpp, and the two must agree on the
layout below. Little endian throughout, no padding.

    char[8]   "EVPAK001"
    uint32    entryCount
    uint32    directoryOffset       from the start of the file
    <blobs>                         in the order the directory lists them
    directory at directoryOffset, entryCount x:
        uint16    nameLength
        char[]    name              relative, '/' separated, e.g. Textures/x.evtex
        uint32    offset            from the start of the file
        uint32    storedBytes       what is in the file
        uint32    originalBytes     what comes out
        uint8     compression       0 = stored, 1 = raw DEFLATE

Compression is raw DEFLATE (no zlib header), which is exactly what raylib's
DecompressData reads — verified against a real cooked model before this was
written. It is per entry and only kept when it actually pays: the music is
already MP3 and the textures are already QOI, so both are stored as they are and
only the models and shaders shrink.
"""

import struct
import zlib
from pathlib import Path

PAK_MAGIC = b"EVPAK001"
STORED = 0
DEFLATE = 1
# Below this saving, the decompression is not worth the space it buys back.
MIN_SAVING = 0.05


def PackFolder(folder, output):
    """Packs every file under `folder` into `output`. Returns (entries, bytes)."""
    folder = Path(folder)
    output = Path(output)
    if not folder.is_dir():
        return 0, 0

    # Sorted, so the same assets always produce the same archive.
    files = sorted(p for p in folder.rglob("*") if p.is_file())
    if not files:
        return 0, 0

    blobs = []
    directory = []
    offset = 16  # the header is written first, and every blob follows it

    for path in files:
        raw = path.read_bytes()
        compressor = zlib.compressobj(9, zlib.DEFLATED, -15)
        packed = compressor.compress(raw) + compressor.flush()

        if len(packed) < len(raw) * (1.0 - MIN_SAVING):
            blob, method = packed, DEFLATE
        else:
            blob, method = raw, STORED

        name = path.relative_to(folder).as_posix().encode("utf-8")
        directory.append((name, offset, len(blob), len(raw), method))
        blobs.append(blob)
        offset += len(blob)

    with output.open("wb") as pak:
        pak.write(PAK_MAGIC)
        pak.write(struct.pack("<II", len(directory), offset))
        for blob in blobs:
            pak.write(blob)
        for name, blobOffset, storedBytes, originalBytes, method in directory:
            pak.write(struct.pack("<H", len(name)))
            pak.write(name)
            pak.write(struct.pack("<IIIB", blobOffset, storedBytes, originalBytes, method))

    return len(directory), output.stat().st_size
