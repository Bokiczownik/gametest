"""Heightmap for a large landscape: gentle rolling hills, imported in UE Landscape mode.

Run with any Python that has numpy (no Blender needed):
    python Tools/Terrain/landscape_heightmap.py

Writes a 16-bit grayscale PNG to OUT_PATH. Import in UE: Landscape mode > Manage > New >
Import from File, Scale X/Y = CELL * 100, Scale Z = Z_SCALE. With those scales the PNG value
32768 is z = 0 and every 128 units are 1 m, so heights in this script are metres above the
landscape actor. Terrain = spectral (FFT) noise, band-limited to long wavelengths, then scaled
so the steepest slopes stay below MAX_SLOPE_DEG (walkable for the player).
"""

import os
import struct
import zlib

import numpy as np

# ---------------------------------------------------------------- parameters
RESOLUTION = 2017        # vertices per side (UE: 63 quads x 2x2 sections x 16 components + 1)
CELL = 1.0               # m per quad (UE Scale X/Y = 100)
Z_SCALE = 100.0          # UE Scale Z; +-256 m range, 0.78 cm per step
SEED = 2026

MIN_WAVELENGTH = 60.0    # m; finer detail is filtered out (keeps hills smooth)
MAX_WAVELENGTH = 900.0   # m; larger features are damped (no single giant slope)
SPECTRAL_BETA = 1.8      # amplitude ~ 1/f^beta; higher = smoother, broader hills
HEIGHT_RANGE = 45.0      # m between lowest and highest point (before the slope clamp)
MAX_SLOPE_DEG = 18.0     # 99.9th percentile of slope is scaled down to this

_HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.normpath(os.path.join(_HERE, "..", ".."))
OUT_PATH = os.path.join(PROJECT_ROOT, "SourceArt", "Ground", "Landscape", "HM_Landscape_2km.png")


def generate():
    n = 1 << int(np.ceil(np.log2(RESOLUTION)))
    rng = np.random.default_rng(SEED)
    fx = np.fft.fftfreq(n, d=CELL)
    k = np.sqrt(fx[None, :] ** 2 + fx[:, None] ** 2)
    k[0, 0] = 1.0
    amp = k ** -SPECTRAL_BETA
    amp *= np.exp(-(k * MIN_WAVELENGTH) ** 2)
    amp *= 1.0 - np.exp(-(k * MAX_WAVELENGTH) ** 2)
    amp[0, 0] = 0.0
    spectrum = (rng.normal(size=(n, n)) + 1j * rng.normal(size=(n, n))) * amp
    h = np.real(np.fft.ifft2(spectrum))[:RESOLUTION, :RESOLUTION]
    h = (h - h.min()) / (h.max() - h.min()) * HEIGHT_RANGE

    gy, gx = np.gradient(h, CELL)
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    p999 = np.percentile(slope, 99.9)
    if p999 > MAX_SLOPE_DEG:
        h *= np.tan(np.radians(MAX_SLOPE_DEG)) / np.tan(np.radians(p999))
    return h


def write_png16(path, values):
    height, width = values.shape
    raw = b"".join(b"\x00" + row.astype(">u2").tobytes() for row in values)

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 16, 0, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 6)))
        f.write(chunk(b"IEND", b""))


def main():
    h = generate()
    values = np.clip(np.round(32768 + h * 100.0 * 128.0 / Z_SCALE), 0, 65535).astype(np.uint16)
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    write_png16(OUT_PATH, values)

    gy, gx = np.gradient(h, CELL)
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    print(f"{OUT_PATH}: {RESOLUTION}x{RESOLUTION}, {(RESOLUTION - 1) * CELL:.0f} m per side")
    print(f"height {h.min():.1f}..{h.max():.1f} m, slope median {np.median(slope):.1f} deg, "
          f"p99 {np.percentile(slope, 99):.1f}, max {slope.max():.1f}")


if __name__ == "__main__":
    main()
