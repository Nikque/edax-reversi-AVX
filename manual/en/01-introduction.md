# 1. Introduction

[Contents](README.md) | Next: [2. Starting and playing a first game](02-quick-start.md)

## 1.1 What Edax is

Edax is a program that plays Othello (Reversi). It was written by Richard Delorme; a version made faster with the instructions of recent CPUs (AVX2, AVX-512 and others) is published by Toshihiko Okuhara (Edax 4.5.x, `okuhara/edax-reversi-AVX`).

This **fixed version** (`Nikque/edax-reversi-AVX`; its versions are named `v4.5.5-nikque.N`) starts from that v4.5.5 and adds:

- bug fixes (book handling, multi-threaded search, reading and writing games, …);
- speed and memory improvements, so that large books (hundreds of millions of positions) can be handled;
- commands to grow and maintain a book (`book deviate2`, `deviate3`, `learn`, `leaf-recalculate`, …);
- a library (libedax) to call Edax from other programs.

The evaluation data (`eval.dat`) and the format of the book file are those of the original Edax 4.5.5. A book of the original Edax can be read as it is.

What changed and when is in the `README-NIKQUE.en.md` of the package (changes of each version) and in `RELEASE-NOTES.md` (list of fixes). This manual explains how to use the program.

## 1.2 What you can do with Edax

| What you want to do | Chapter |
|---|---|
| Play against Edax. See how good the moves of a game were | [2](02-quick-start.md), [3](03-playing.md) |
| Find the best move and the score of a position | [3](03-playing.md), [5](05-search.md) |
| Solve an endgame position to the end (exact solving) | [5](05-search.md), [11](11-solve-bench.md) |
| Build, grow and maintain opening data (a book) | [6](06-book-basics.md) to [9](09-large-books.md) |
| Convert and check game files | [10](10-files.md) |
| Use Edax from a graphical program (GUI) or from your own program | [12](12-integration.md) |

Edax itself has no window where you place discs with the mouse. **It is a program that you operate by typing** (the board is drawn with characters in a console window, and you type moves such as `f5`). To see a drawn board, connect Edax to a GUI program that supports it ([chapter 12](12-integration.md)).

## 1.3 What the package contains

The ZIP downloaded from the Releases page contains:

| Place | Content |
|---|---|
| `bin/` | The executables (table below), the libraries (libedax), the settings file `config.ini`, and `README.MS-Windows.txt`, the note for Windows that came with the original Edax |
| `bin/data/eval.dat` | The evaluation data. **Edax does not start without it** |
| `bin/data/book.dat` | A first book (nearly empty: it holds the initial position only) |
| `problem/` | Endgame problem sets (`fforum-1-19.obf` and three more; [chapter 11](11-solve-bench.md)) |
| `README-NIKQUE.en.md`, `README-NIKQUE.ja.md` | Changes of each version and the record of the measurements |
| `RELEASE-NOTES.md`, `RELEASE-NOTES.ja.md` | The list of fixes |
| `manual/en/`, `manual/ja/` | This manual (English and Japanese; since v4.5.5-nikque.13) |
| `LICENSE` | The licence (GPL-3.0) |

## 1.4 Which executable to use

`bin/` holds an executable for each operating system and kind of CPU. They do the same things and give the same search results; only the speed differs.

| Your machine | Executable |
|---|---|
| Windows (64-bit), any PC | `wEdax-x86-64.exe` |
| Windows, a CPU with AVX2 | `wEdax-x86-64-v3.exe` (faster than the one above) |
| Windows, a CPU with AVX-512 | `wEdax-x86-64-v4.exe` (faster still) |
| Windows (32-bit) | `wEdax-x86-sse.exe` (CPU with SSE2), `wEdax-x86.exe` |
| Windows (ARM64) | `wEdax-arm64.exe` |
| Linux (64-bit) | `lEdax-x86-64`, `lEdax-x86-64-v3` (AVX2), `lEdax-x86-64-v4` (AVX-512) |
| Linux (32-bit) | `lEdax-x86` |
| macOS | `mEdax-arm64` (Apple silicon), `mEdax-x64-modern` (Intel) |
| Android | `aEdax-arm64-v8a`, `aEdax-armeabi-v7a` |

- **If in doubt, use the one without `v3` or `v4` in its name** (`wEdax-x86-64.exe` on Windows). It runs on any 64-bit CPU.
- An executable with `v3` in its name needs a CPU with AVX2, one with `v4` a CPU with AVX-512. They do not run on a CPU without these instructions. If you do not know what your CPU supports, look up its model, or use the executable without the suffix.
- An idea of the speed difference (Ryzen 9 9950X, one thread, exact solving of 20 endgame positions; measured for the README of the package): standard 69 million nodes per second, `v3` 81 million, `v4` 92 million.
- According to the README of the package, the Windows ARM64 and Android executables were only built, not run on a device, and the macOS executables come from the automatic build of the release. The checks for this manual were made with the 64-bit Windows version.

## 1.5 Words used in this manual (the ones to know first)

The [glossary](15-glossary.md) has more.

| Word | Meaning |
|---|---|
| Position | The discs on the board and the side to move |
| Move | The square where a disc is placed, written like `f5` |
| Pass | Giving the turn to the opponent because no square can be played. Written `ps` |
| Empties | The number of empty squares: 60 at the start of a game, 0 at the end (a game also ends with empties left when neither side can move) |
| Score | "By how many discs the game should end with best play", **seen from the side to move**. `+4`: the side to move should win by 4 discs; `-2`: lose by 2; `+0`: draw |
| Level | A number (0 to 60) that sets the strength of the search. The larger, the deeper Edax reads and the longer it takes ([chapter 5](05-search.md)) |
| Exact solving | Reading everything to the end of the game, which gives the exact final disc difference |
| Book | Data that stores opening and midgame positions with their scores; an "opening book" ([chapter 6](06-book-basics.md)) |
| Thread | A unit of work that the CPU can run at the same time as others. More threads make the search faster |

Next: [2. Starting and playing a first game](02-quick-start.md)
