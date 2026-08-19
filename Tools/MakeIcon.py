#!/usr/bin/env python3
"""Builds the Windows executable icon from the game's logo.

Windows is the only platform here whose executable can carry an icon at all: it
is a resource compiled into the PE by windres (see Game/Source/BudgetLeague.rc
and the Makefile). ELF has nothing of the kind, and macOS keeps it in a bundle,
so neither reaches this script.

Run by the Makefile as a build event before linking a Windows build, and only
when the .png or this script is newer than the .ico.
"""

import argparse
from pathlib import Path

from PIL import Image

# What Windows asks for, smallest first: the title bar and the tray take 16, the
# desktop takes 32 and 48, and the extra-large view in Explorer takes 256.
SIZES = [16, 24, 32, 48, 64, 128, 256]


def main():
    parser = argparse.ArgumentParser(description="Cook the game logo into a Windows .ico")
    parser.add_argument("--input", required=True, type=Path, help="source .png")
    parser.add_argument("--output", required=True, type=Path, help="destination .ico")
    args = parser.parse_args()

    image = Image.open(args.input).convert("RGBA")

    # The logo is square, but a future one might not be: pad rather than squash,
    # because an icon stretched to a square reads as a mistake at 16 px.
    if image.width != image.height:
        side = max(image.width, image.height)
        square = Image.new("RGBA", (side, side), (0, 0, 0, 0))
        square.paste(image, ((side - image.width) // 2, (side - image.height) // 2))
        image = square

    args.output.parent.mkdir(parents=True, exist_ok=True)
    # Pillow writes every size into the one file, each resampled from the full
    # resolution source rather than from the previous step.
    image.save(args.output, format="ICO", sizes=[(size, size) for size in SIZES])
    print(f"[icon] {args.input.name} -> {args.output.name} ({', '.join(str(s) for s in SIZES)} px)")


if __name__ == "__main__":
    main()
