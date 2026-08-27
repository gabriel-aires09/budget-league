#!/usr/bin/env python3
"""Asset Cooker - build event that prepares Game/Assets/ for a build's assets/ folder.

It cooks the Cars-Park FBX models into .evmodel, the engine's own model format
(see WriteModel for the layout), and the PNGs into .evtex, the engine's texture
format (see WriteTexture). Fonts arrive with the milestone that needs them.

Cooking is incremental: an asset is rebuilt only when its source, this script or
FbxReader.py is newer than the cooked file.
"""

import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from FbxReader import ReadMeshes, FbxError
from PakWriter import PackFolder

MODEL_MAGIC = b"EVMDMSH1"
TEXTURE_MAGIC = b"EVTXQOI1"
# CLAUDE.md 3.2. The logo is authored at 1024 and is drawn at about a third of
# the window height, so nothing in the game asks for more than this.
MAX_TEXTURE_SIZE = 512

# Materials whose baked colour must survive: tinting the glass, the tyres or
# the lights with a team colour would turn the whole car into a single blob.
FIXED_MATERIALS = ("black", "grey", "gray", "windows", "window", "glass",
                   "chrome", "tyre", "tire", "wheel", "rubber")


def IsPaint(name):
    """True when a material is car paint, so the team colour replaces it."""
    key = name.lower()
    return key not in FIXED_MATERIALS and not key.endswith("lights") and not key.endswith("light")


def LinearToSrgb(value):
    value = max(0.0, min(1.0, value))
    encoded = 12.92 * value if value <= 0.0031308 else 1.055 * value ** (1.0 / 2.4) - 0.055
    return int(round(encoded * 255.0))


def WriteModel(path, materials, meshes, bounds):
    """Writes the .evmodel binary. All values little endian, no padding.

        char[8]  "EVMDMSH1"
        uint32   materialCount
        uint32   meshCount
        float[3] boundsMin           model space, metres
        float[3] boundsMax
        materialCount x:
            uint32   nameLength
            char[]   name
            uint8[4] diffuse RGBA
            uint8    paint           1 = replace with the team colour
            uint8    shade           paint brightness relative to the brightest paint
            uint8[2] reserved
        meshCount x:                 one mesh per material, triangle soup
            uint32   materialIndex
            uint32   vertexCount     always a multiple of 3
            float[3] x vertexCount   positions
            float[3] x vertexCount   normals

    There is no index buffer on purpose. The models are flat shaded, so every
    triangle owns its normals and indexing would deduplicate almost nothing;
    it also keeps the format clear of raylib's 16 bit index limit.
    """
    with open(path, "wb") as out:
        out.write(MODEL_MAGIC)
        out.write(struct.pack("<II", len(materials), len(meshes)))
        out.write(struct.pack("<6f", *bounds[0], *bounds[1]))

        for material in materials:
            name = material["name"].encode("utf-8")
            out.write(struct.pack("<I", len(name)))
            out.write(name)
            out.write(bytes(LinearToSrgb(c) for c in material["diffuse"]))
            out.write(b"\xff")  # opaque
            out.write(struct.pack("<BBBB", 1 if material["paint"] else 0,
                                  material["shade"], 0, 0))

        for materialIndex, vertices in meshes:
            out.write(struct.pack("<II", materialIndex, len(vertices)))
            out.write(struct.pack("<%df" % (len(vertices) * 3),
                                  *[c for position, _normal in vertices for c in position]))
            out.write(struct.pack("<%df" % (len(vertices) * 3),
                                  *[c for _position, normal in vertices for c in normal]))


def EncodeQoi(pixels, pixelCount):
    """QOI chunk stream for RGBA pixels, without QOI's own 14 byte header.

    https://qoiformat.org/qoi-specification.pdf - the header is dropped because
    the .evtex header below already carries the size, and the end marker with
    it, because the decoder stops at pixelCount.
    """
    out = bytearray()
    table = [(0, 0, 0, 0)] * 64
    previous = (0, 0, 0, 255)
    run = 0

    for i in range(pixelCount):
        pixel = (pixels[i * 4], pixels[i * 4 + 1], pixels[i * 4 + 2], pixels[i * 4 + 3])

        if pixel == previous:
            run += 1
            if run == 62 or i == pixelCount - 1:
                out.append(0xc0 | (run - 1))
                run = 0
            continue

        if run > 0:
            out.append(0xc0 | (run - 1))
            run = 0

        slot = (pixel[0] * 3 + pixel[1] * 5 + pixel[2] * 7 + pixel[3] * 11) % 64
        if table[slot] == pixel:
            out.append(slot)
            previous = pixel
            continue
        table[slot] = pixel

        if pixel[3] != previous[3]:
            out.append(0xff)
            out.extend(pixel)
            previous = pixel
            continue

        dr = pixel[0] - previous[0]
        dg = pixel[1] - previous[1]
        db = pixel[2] - previous[2]
        drdg = dr - dg
        dbdg = db - dg

        if -2 <= dr <= 1 and -2 <= dg <= 1 and -2 <= db <= 1:
            out.append(0x40 | (dr + 2) << 4 | (dg + 2) << 2 | (db + 2))
        elif -32 <= dg <= 31 and -8 <= drdg <= 7 and -8 <= dbdg <= 7:
            out.append(0x80 | (dg + 32))
            out.append((drdg + 8) << 4 | (dbdg + 8))
        else:
            out.append(0xfe)
            out.extend(pixel[:3])

        previous = pixel

    return bytes(out)


def WriteTexture(path, width, height, pixels):
    """Writes the .evtex binary. All values little endian, no padding.

        char[8]  "EVTXQOI1"
        uint32   width
        uint32   height
        uint32   channels             always 4 (RGBA8)
        uint32   payloadBytes
        uint8[]  QOI chunk stream     see EncodeQoi

    The engine decodes this straight into an RGBA buffer and hands it to the
    GPU, so the format carries no mipmaps, no filtering hints and no palette.
    """
    payload = EncodeQoi(pixels, width * height)
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "wb") as out:
        out.write(TEXTURE_MAGIC)
        out.write(struct.pack("<IIII", width, height, 4, len(payload)))
        out.write(payload)
    return payload


def CookTexture(source, target):
    from PIL import Image

    image = Image.open(source).convert("RGBA")
    width, height = image.size
    if max(width, height) > MAX_TEXTURE_SIZE:
        scale = MAX_TEXTURE_SIZE / max(width, height)
        width = max(1, int(round(width * scale)))
        height = max(1, int(round(height * scale)))
        image = image.resize((width, height), Image.LANCZOS)

    payload = WriteTexture(target, width, height, image.tobytes())
    raw = width * height * 4
    print("[cook] %-24s %dx%d, %.0f KB, %.0f%% of raw" % (
        target.name, width, height, len(payload) / 1024.0, 100.0 * len(payload) / raw))


def CookModel(source, target):
    materials, triangles = ReadMeshes(source)

    # Group triangles by material, which drops materials the mesh never uses.
    # The packs carry a few of those and they are all default grey, so keeping
    # them would make the paint detection below pick the wrong one.
    grouped = {}
    for materialIndex, corners in triangles:
        grouped.setdefault(materialIndex, []).extend(corners)
    if not grouped:
        raise FbxError("no triangles found")

    used = sorted(grouped)
    cooked = []
    for index in used:
        material = materials[index]
        cooked.append({"name": material["name"], "diffuse": material["diffuse"],
                       "paint": IsPaint(material["name"]), "shade": 255})

    # Paint materials keep their relative brightness, so a car painted in two
    # shades (a body plus a darker skirt) still reads as two shades once tinted.
    def Luminance(rgb):
        return 0.2126 * rgb[0] + 0.7152 * rgb[1] + 0.0722 * rgb[2]

    brightest = max((Luminance(m["diffuse"]) for m in cooked if m["paint"]), default=0.0)
    if brightest > 0.0:
        for material in cooked:
            if material["paint"]:
                material["shade"] = int(round(255.0 * Luminance(material["diffuse"]) / brightest))

    meshes = [(slot, grouped[index]) for slot, index in enumerate(used)]

    low = [9e9] * 3
    high = [-9e9] * 3
    for _slot, vertices in meshes:
        for position, _normal in vertices:
            for axis in range(3):
                low[axis] = min(low[axis], position[axis])
                high[axis] = max(high[axis], position[axis])

    target.parent.mkdir(parents=True, exist_ok=True)
    WriteModel(target, cooked, meshes, (low, high))

    triangleCount = sum(len(vertices) for _slot, vertices in meshes) // 3
    paint = ", ".join(m["name"] for m in cooked if m["paint"]) or "none"
    print("[cook] %-24s %d tris, %d materials, paint: %s, size %.2f x %.2f x %.2f m" % (
        target.name, triangleCount, len(cooked), paint,
        high[0] - low[0], high[1] - low[1], high[2] - low[2]))


def main():
    parser = argparse.ArgumentParser(description="Cook game assets into a build folder.")
    parser.add_argument("--assets", required=True, type=Path, help="source assets folder")
    parser.add_argument("--output", required=True, type=Path, help="cooked assets folder")
    parser.add_argument("--force", action="store_true", help="ignore timestamps and cook everything")
    parser.add_argument("--pak", type=Path, default=None,
                        help="also write every cooked asset into this .pak archive")
    args = parser.parse_args()

    if not args.assets.is_dir():
        print("[cook] source assets folder not found: %s" % args.assets)
        return 1

    args.output.mkdir(parents=True, exist_ok=True)

    # Changing the cooker itself invalidates everything it produces.
    toolStamp = max(Path(__file__).stat().st_mtime,
                    (Path(__file__).parent / "FbxReader.py").stat().st_mtime)

    cooked = 0
    failed = 0
    modelsOut = args.output / "Models"
    for source in sorted(args.assets.glob("**/*.fbx")):
        target = modelsOut / (source.stem + ".evmodel")
        if not args.force and target.exists() and \
                target.stat().st_mtime >= max(source.stat().st_mtime, toolStamp):
            continue
        try:
            CookModel(source, target)
            cooked += 1
        except (FbxError, OSError, struct.error) as error:
            print("[cook] FAILED %s: %s" % (source.name, error))
            failed += 1

    texturesOut = args.output / "Textures"
    for source in sorted(args.assets.glob("**/*.png")):
        # The car pack ships a preview thumbnail of itself; the models are what
        # this cooker takes from that folder.
        if "Cars-Park" in source.parts:
            continue
        target = texturesOut / (source.stem + ".evtex")
        if not args.force and target.exists() and \
                target.stat().st_mtime >= max(source.stat().st_mtime, toolStamp):
            continue
        try:
            CookTexture(source, target)
            cooked += 1
        except (ImportError, OSError, struct.error) as error:
            print("[cook] FAILED %s: %s" % (source.name, error))
            failed += 1

    # Shaders are plain GLSL and the engine hands them straight to raylib, so
    # they are copied rather than converted.
    shadersOut = args.output / "Shaders"
    for source in sorted(args.assets.glob("Shaders/*.vs")) + sorted(args.assets.glob("Shaders/*.fs")):
        target = shadersOut / source.name
        if not args.force and target.exists() and target.stat().st_mtime >= source.stat().st_mtime:
            continue
        shadersOut.mkdir(parents=True, exist_ok=True)
        target.write_bytes(source.read_bytes())
        print("[cook] %s" % target.name)
        cooked += 1

    # The soundtrack is streamed from disk by raylib at runtime, not decoded
    # here, so the tracks are copied as they are. The engine finds them by
    # scanning this folder, which is why the file names are not listed anywhere.
    musicOut = args.output / "Music"
    for source in sorted(args.assets.glob("Sounds/*.mp3")):
        target = musicOut / source.name
        if not args.force and target.exists() and target.stat().st_mtime >= source.stat().st_mtime:
            continue
        musicOut.mkdir(parents=True, exist_ok=True)
        target.write_bytes(source.read_bytes())
        print("[cook] %-24s %.1f MB" % (target.name, target.stat().st_size / (1024.0 * 1024.0)))
        cooked += 1

    if failed:
        return 1
    print("[cook] %s -> %s" % ("%d asset(s) cooked" % cooked if cooked else "up to date", args.output))

    # The archive is written from the cooked folder rather than from the sources,
    # so it holds exactly what the loose build holds and the incremental cooking
    # above is untouched. Rewritten whenever anything was cooked, and when it is
    # missing, so a build can never leave a stale one beside the executable.
    if args.pak is not None and (cooked > 0 or not args.pak.exists()):
        entries, size = PackFolder(args.output, args.pak)
        if entries == 0:
            print("[pak] nothing to pack from %s" % args.output)
        else:
            loose = sum(f.stat().st_size for f in args.output.rglob("*") if f.is_file())
            print("[pak] %d asset(s), %.1f MB -> %s (%.1f MB)"
                  % (entries, loose / (1024.0 * 1024.0), args.pak.name, size / (1024.0 * 1024.0)))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
