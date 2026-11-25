"""PPM parsing and screenshot comparison for NeoST scenarios."""

from __future__ import annotations

from pathlib import Path

Box = tuple[int, int, int, int]


def read_ppm(path: Path) -> tuple[int, int, bytes]:
    """Read a binary P6 PPM and return width, height and RGB bytes."""
    data = Path(path).read_bytes()
    position = 0

    def token() -> bytes:
        nonlocal position
        while position < len(data) and data[position] in b" \t\r\n":
            position += 1
        if position < len(data) and data[position] == ord("#"):
            while position < len(data) and data[position] not in b"\r\n":
                position += 1
            return token()
        start = position
        while position < len(data) and data[position] not in b" \t\r\n#":
            position += 1
        return data[start:position]

    magic, width, height, maximum = token(), int(token()), int(token()), int(token())
    if magic != b"P6" or maximum != 255:
        raise RuntimeError(f"unsupported NeoST screenshot format: {path}")
    while position < len(data) and data[position] in b" \t\r\n":
        position += 1
    pixels = data[position:]
    if len(pixels) != width * height * 3:
        raise RuntimeError(f"truncated NeoST screenshot: {path}")
    return width, height, pixels


def diff_stats(first: Path, second: Path, *,
               region: Box | None = None) -> tuple[int, Box | None]:
    """Count differing pixels and return their inclusive bounding box."""
    first_width, first_height, first_pixels = read_ppm(first)
    second_width, second_height, second_pixels = read_ppm(second)
    if (first_width, first_height) != (second_width, second_height):
        raise RuntimeError(f"screenshot sizes differ: {first} and {second}")

    left, top, right, bottom = region or (0, 0, first_width, first_height)
    left = max(0, left)
    top = max(0, top)
    right = min(first_width, right)
    bottom = min(first_height, bottom)

    count = 0
    minimum_x = first_width
    minimum_y = first_height
    maximum_x = -1
    maximum_y = -1
    for y in range(top, bottom):
        row = y * first_width
        for x in range(left, right):
            offset = (row + x) * 3
            if first_pixels[offset:offset + 3] != second_pixels[offset:offset + 3]:
                count += 1
                minimum_x = min(minimum_x, x)
                minimum_y = min(minimum_y, y)
                maximum_x = max(maximum_x, x)
                maximum_y = max(maximum_y, y)
    if not count:
        return 0, None
    return count, (minimum_x, minimum_y, maximum_x, maximum_y)


def bbox_inside(bbox: Box, region: Box) -> bool:
    """Return whether an inclusive box lies within a half-open region."""
    left, top, right, bottom = region
    return (left <= bbox[0] <= bbox[2] < right and
            top <= bbox[1] <= bbox[3] < bottom)
