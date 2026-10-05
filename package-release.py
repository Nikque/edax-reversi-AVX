"""Create the Edax runtime ZIP with the upstream 4.5.5 directory layout.

Usage: python package-release.py path/to/edax-4.5.5-nikque.2.zip [--without-macos]
--without-macos makes a ZIP to check before the macOS files (built by the
release-binaries workflow) are there: it leaves them out.
The 32-bit macOS executable is intentionally omitted: current Xcode SDKs
cannot link it, and shipping the upstream executable would misrepresent it
as containing this fork's fixes.
"""

from hashlib import sha256
from pathlib import Path
from sys import argv
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo


ROOT = Path(__file__).resolve().parent
# eval.dat version 3.3.0 of the eval2 series (46 patterns + mobility weights); the upstream v4.5.5 file is f8b22996...
EVAL_SHA256 = "1870f8fa5eecb6df972a4d83224832f9a620745cdc67c2d9b17bcd0ba09ea172"

BINARIES = (
    "bin/aEdax-arm64-v8a",
    "bin/aEdax-armeabi-v7a",
    "bin/lEdax-x86",
    "bin/lEdax-x86-64",
    "bin/lEdax-x86-64-v3",
    "bin/lEdax-x86-64-v4",
    "bin/mEdax-arm64",
    "bin/mEdax-x64-modern",
    "bin/wEdax-arm64.exe",
    "bin/wEdax-x86.exe",
    "bin/wEdax-x86-sse.exe",
    "bin/wEdax-x86-64.exe",
    "bin/wEdax-x86-64-v3.exe",
    "bin/wEdax-x86-64-v4.exe",
)

# libedax (Edax as a library)
LIBRARIES = (
    "bin/libedax.universal.dylib",
    "bin/libedax-arm64-v8a.so",
    "bin/libedax-armeabi-v7a.so",
    "bin/libedax-x86-64.so",
    "bin/libedax-x86-64-v3.so",
    "bin/libedax-x86-64-v4.so",
    "bin/libedax-x64.dll",
    "bin/libedax-x64-v3.dll",
    "bin/libedax-x64-v4.dll",
)

MACOS_FILES = ("bin/mEdax-arm64", "bin/mEdax-x64-modern", "bin/libedax.universal.dylib")

OTHER_FILES = (
    "LICENSE",
    "README-NIKQUE.en.md",
    "README-NIKQUE.ja.md",
    "RELEASE-NOTES.md",
    "RELEASE-NOTES.ja.md",
    "bin/README.MS-Windows.txt",
    "bin/config.ini",
    "bin/data/book.dat",
    "bin/data/eval.dat",
    "problem/README.md",
    "problem/fforum-1-19.obf",
    "problem/fforum-20-39.obf",
    "problem/fforum-40-59.obf",
    "problem/fforum-60-79.obf",
)


def main() -> None:
    without_macos = "--without-macos" in argv[2:]
    if len(argv) < 2 or (len(argv) > 2 and not without_macos):
        raise SystemExit(__doc__)

    output = Path(argv[1]).resolve()
    eval_data = (ROOT / "bin/data/eval.dat").read_bytes()
    if sha256(eval_data).hexdigest() != EVAL_SHA256:
        raise SystemExit("bin/data/eval.dat is not the eval.dat 3.3.0 of the eval2 series")

    output.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(output, "w") as archive:
        for name in (*OTHER_FILES, *BINARIES, *LIBRARIES):
            if without_macos and name in MACOS_FILES:
                print(f"left out: {name}")
                continue
            path = ROOT / name
            data = path.read_bytes()
            info = ZipInfo(name, date_time=(2026, 9, 27, 0, 0, 0))
            info.create_system = 3
            executable = name in (*BINARIES, *LIBRARIES) and not name.endswith((".exe", ".dll"))
            info.external_attr = ((0o100755 if executable else 0o100644) << 16)
            info.compress_type = ZIP_DEFLATED
            archive.writestr(info, data, compress_type=ZIP_DEFLATED, compresslevel=9)

    with ZipFile(output) as archive:
        bad = archive.testzip()
        if bad is not None:
            raise SystemExit(f"ZIP verification failed: {bad}")
    print(f"Created {output} ({output.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
