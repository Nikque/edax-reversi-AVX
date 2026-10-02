# Edax 4.5.5: release notes

[日本語](RELEASE-NOTES.ja.md)

## v4.5.5-nikque.7

A library (libedax), learning games with several threads (new setting `book-store-tasks`; nothing changes by default), a faster `book fix`, and two book bug fixes. `eval.dat`, the book file format and the search results are unchanged. With the default settings, the learned books are also the same as with v4.5.5-nikque.6. See the [English](README-NIKQUE.en.md) and [Japanese](README-NIKQUE.ja.md) READMEs for details.

libedax (Edax as a library for other programs):

- The same 93 functions and the same data layout as libedax by lavox and sensuikan1973: programs written for libedax (libedax4dart, edax_runner, ...) work by replacing the library file.
- `libedax-x64.dll` (any x86-64 CPU), `libedax-x64-v3.dll` (AVX2), `libedax-x64-v4.dll` (AVX-512); `libedax-x86-64.so`, ... on Linux. The macOS library is not built yet.
- Settings are read from `edax.ini` and `config.ini` of the current folder, then from the arguments of the initialization.
- New functions: `edax_book_deviate2`, `edax_book_deviate3`, `libedax_cpu_level`, `edax_book_store_games` (play and learn several games together), `edax_book_store_tasks`.
- Android libraries (`libedax-arm64-v8a.so`, `libedax-armeabi-v7a.so`) are also built (build checked only).
- The edax program is not affected (an edax built from the sources with only libedax added is byte-identical to the one of v4.5.5-nikque.6). The library and the edax program search the same nodes at the same speed. Compared with the original libedax (Edax 4.4), a single-thread endgame search is 1.3 to 1.7 times faster, with about half the peak memory.

Learning games with several threads (new setting `book-store-tasks`):

- `book-store-tasks = 1` (default) works as before, with the same book as a result. With a number of 2 or more, or `auto` (the value of `n-tasks`), that many games are learned at the same time, by `book store`, `book add`, the new command `book learn <file>` (for each line: play the moves, let Edax play both sides to the end, then `book store`; the "edax vs edax" of edax_runner) and `edax_book_store_games` of libedax.
- How: the positions to add and the moves excluded from their searches are found first; all the searches are done at the same time as one-thread searches; then the positions are added to the book in the usual order. `book learn` also plays the games at the same time, and the book is linked, negamaxed and saved once per group of games.
- Measured (32 logical CPUs, a book of 270,000 positions at level 18, 128 games learned by edax_runner): 112.9 s with `1` (133.5 s with `n-tasks` 8), 39.4 to 41.6 s with `auto` (2.8 to 3.3 times faster). Peak memory: 290 MB to 722 MB. This is about the speed of several edax_runner at the same time (41.1 s with 8 of them and 4 threads each, 35.2 s with 16 x 2 threads, without the time to merge their books), with one process and one book. `book store` of a single game, and small values such as 2 or 4, gain little (and can be slower).
- The book is not the same as with `1` (for these 128 games, compared with the book of one thread and `1`: 372 positions with different contents out of about 272,600, and 5 and 8 positions in one book only; this is no more than the difference made by 8 threads in the original learning (381, and 192 and 196). `auto` gives the same book every time). The expansion commands (`book deviate`, ..., which use `book-expand-tasks`) and `book fill` are unchanged.

Faster `book fix` (same result):

- The checking (Fixing), linking and sorting steps of `book fix` (also used by `book import`, `correct`, `prune`, `subtree`, and after `book store`) use all the threads. The book is the same as with one thread. `book fix` of a book of 6.49 million positions: 22.9 s to 2.3 s with 32 threads (25.4 s with upstream v4.5.5), unchanged with one thread (24.7 s); same peak memory (407 MB).

Bug fixes:

- When a learning command saved its progress to a side file (`.store`, `.dev`, ...), the book was marked as saved, so quitting without `book save` did not save `data/book.dat` (since v4.5.5-nikque.2). A progress save does not mark the book as saved anymore.
- "Fixing book..." of `book fix` printed nothing until the end when no position needed a fix. The checked positions are now printed once per second. The book is unchanged.
## v4.5.5-nikque.6

Startup settings, the limit on legal moves, and the progress display of book merge. `eval.dat`, the book file format and the search results with the same options are unchanged (single-thread search results and node counts checked against v4.5.5-nikque.5). See the [English](README-NIKQUE.en.md) and [Japanese](README-NIKQUE.ja.md) READMEs for details.

Startup settings (values of the bundled `config.ini`, also the defaults without `config.ini`):

- `level = 18`: startup search level. **The built-in default level is now 18 instead of 21** (searches without `-l` give different results from previous versions). A `level` in `config.ini` or `edax.ini` only sets the startup level and does not cap timed games (use `-l` or the `level` command of the prompt for a cap).
- `n-tasks = auto`: number of search threads (1 to the number of logical CPUs; `auto` is the number of logical CPUs). `-n auto` also works on the command line.
- `book-depth = auto` (new setting): book depth at startup (as with the `book depth` command). `auto` keeps the depth of the book file; a number from 1 to 60 sets it. `-book-depth <n|auto>` on the command line.
- `book-usage = on`: play from the opening book.

Syntax of `config.ini` and `edax.ini`:

- `name = value`, `name=value` and `name value` are all accepted, with tabs, full-width spaces and equal signs, and a UTF-8 BOM.
- Names and values such as on/off/auto ignore the case; spaces, `_` and `-` in names are the same (`book depth` = `book-depth`). `#` starts a comment.
- Unknown names are reported at startup with the file name and the line number.

Bug fix:

- Since v4.5.5-nikque.3, "Linking book..." and "Fixing book..." of `book merge` and `book fix` showed no progress. The progress (positions) is printed once per second again, also while `book merge` reads the file. The book is unchanged.

Limit on legal moves:

- The maximum number of legal moves of a position (`MAX_MOVE`) is now 34 instead of 33. Reachable positions have at most 33 legal moves, but positions entered with `-solve` or `setboard` can have 34 (never 35 or more). The move list only grows by 32 bytes; the book memory and the speed are unchanged.

## v4.5.5-nikque.5

Better clock use in games and less book memory (48 bytes per position instead of 56). Fixed-level search results (book learning, `-solve`, games at a given level) are unchanged (single-thread best moves, scores, principal variations and node counts checked against v4.5.5-nikque.4). The book file format and the saved contents are unchanged (except, possibly, `book enhance` with errors above 63). `eval.dat` is unchanged. See the [English](README-NIKQUE.en.md) and [Japanese](README-NIKQUE.ja.md) READMEs for measurements.

Games with a time control:

- Without an explicit level, timed games (`-t`, `-move-time`, xboard time controls) cap the search at level 60 instead of the default 21. Edax used half of its clock or less (16 s per game, 1 thread: +90 Elo, 600 real-time games).
- The time is shared from the measured search speed (twice the measured value). An explicit `-speed n` is used as before (`-speed auto`: measured).

Book memory (657 million positions: 35.55 GiB to 30.64 GiB):

- The deviate2 working value (4 bytes per position) is no longer part of each position: the position selection of `book deviate`, `deviate2` and `deviate3` uses a table of 1 byte per position while it runs.
- Scores (value, lower, upper) take 1 byte each in memory (2 bytes each in the file, as before). With `book enhance` errors above 63, out-of-range bounds are saturated to ±127.
- The done and todo marks share one byte (5-bit learning epoch; the marks of every position are reset once every 31 epochs).
- The total loss of `book deviate2`/`deviate3` is limited to 254.

New settings:

- `book-expand-tasks = auto` (used by the bundled `config.ini`): the number of positions expanded at the same time comes from the book level (each search uses 2 threads at level 18 or below, 4 up to level 24, 8 above). Without `config.ini` the default is still 1.
- `probcut-model = refit` (experimental; default `standard`): a ProbCut error model refit on measurements. It changes the search results.
- With `-nps`, the game clock is also charged in nodes (reproducible match comparisons).

## v4.5.5-nikque.4

Search and book learning speed, books of up to 4,294,967,295 positions and 1 bug fix. The book file format is unchanged. With the same options the search results are unchanged (single-thread best moves, scores, principal variations and node counts checked against v4.5.5-nikque.3); the new `hash-table-size = auto` of the bundled `config.ini` changes the hash table size, and `book-expand-tasks` above 1 changes the learning order. See the [English](README-NIKQUE.en.md) and [Japanese](README-NIKQUE.ja.md) READMEs for measurements and output checks.

Performance (same results):

- `search_cleanup()` no longer rewrites the search hash tables before each position searched by book learning: entries older than the last cleanup are handled exactly as empty ones, and the memory is wiped only when the date range is used up (level 18, one thread: `-h 26` 180 s to 108 s, `-h 21` 100 s to 97 s).
- Windows: the hash table, search and task locks are a spin lock and an SRW lock instead of `CRITICAL_SECTION` (like the pthread locks of the other systems).
- x86-64-v4: the evaluation reads its 46 weights with three 16-lane AVX-512 gathers (integer sums, the same evaluation to the bit).
- The Windows release executables except ARM64 are built with profile-guided optimization (`vc-pgo-*` targets in `src/NMakefile`).

New settings:

- `book-expand-tasks` (`config.ini`, or `-book-expand-tasks n`; default 1): `book deviate`, `deviate2`, `deviate3`, `enhance` and `play` expand n positions at the same time, each search with `n-tasks / n` threads and its own hash tables. On a 657-million-position book (deviate3, 10,000 expansions, 32 threads), 16 took 174 s instead of 832 s and 805 s, with the same positions and link moves and a few leaf and score differences of 1 or 2 points.
- `hash-table-size = auto` (`config.ini`, or `-h auto`; used by the bundled `config.ini`): 21 for 1-3 search threads, 22 for 4-15, 23 for 16-63, at most 25 and at most 1/32 of the memory. Without `config.ini` the default is still 21.

Change of ID 28: the position count of the book header is read and written as an unsigned 32-bit number, so a book can hold up to 4,294,967,295 positions (was 2,147,483,647). Smaller books are saved byte for byte as before; larger ones cannot be read by earlier versions.

| ID | Corrected behavior |
|---|---|
| 30 | Search threads freed soon after being created are stopped reliably (the stop signal could be lost and Edax wait forever, or a task be freed before its thread started). |

Builds: Windows with Visual Studio 2022 (MSVC 19.44), profile-guided except ARM64 (the x86 targets from an x86 prompt); Linux with gcc 11.4 on Ubuntu 22.04; Android with NDK r27d; macOS x64 with the release-binaries workflow.

## v4.5.5-nikque.3

Book performance, 11 bug fixes and an automatic save after `book merge`. The book file format is unchanged. See the [English](README-NIKQUE.en.md) and [Japanese](README-NIKQUE.ja.md) READMEs for measurements and the output checks against v4.5.5-nikque.2.

Performance (the results of every command are unchanged, except where noted in the READMEs):

- Books are read and written through a 16 MB buffer.
- An in-memory position takes 56 bytes (was 64) and stores up to 4 links itself; a book saved by Edax is loaded into one exactly sized block.
- Negamax, the selection of the positions to expand by `book deviate`, `deviate2` and `deviate3`, and the link rebuild, check and sort of `book merge` run on `n-tasks` threads.
- `book_clean` no longer rewrites every position, and positions to expand are recorded instead of being searched for in the whole book (same expansion order).
- `deviate2`/`deviate3` process positions by increasing accumulated loss, so each position is walked once.
- `book merge` streams the source file (check pass, then merge pass) instead of loading it, and reuses the source Leaf of a relinked position instead of searching it again when the source Leaf is still not a Link. This reuse makes a merge of two 657-million-position books about 42 times faster (10,934 s to 263 s; 16,408 s and 117.2 GiB with v4.5.5-nikque.2, 263 s and 37.0 GiB now), but a few scores may differ slightly from a new search: 27,897 of 659 million positions (0.004%) in that merge, mostly by 1 or 2. v4.5.5-nikque.2 itself varies by a similar amount between runs.

| ID | Corrected behavior |
|---|---|
| 19 | Skip links to positions missing from the book instead of crashing; `book fix` removes them. |
| 20 | Initialize the engine before reading piped commands: a `quit` received while the book was loading crashed Edax. |
| 21 | `book fill` no longer walks a board stored in a bucket array that may move while positions are added (use after free). |
| 22 | Report duplicate or failed position additions, release the links of a child that is not added, and stop learning when memory is exhausted. |
| 23 | Flush a saved book to disk before it replaces the previous file. |
| 24 | Positions added by `book merge` start with their Leaf score, so positions not reachable from the root no longer keep +/-127. |
| 25 | An empty or truncated `.edx` file, or one with an illegal move, no longer erases the current game. |
| 26 | Use `GetTickCount64` on Windows (`GetTickCount` wraps after 49.7 days). |
| 27 | Clear the padding byte of the book date written in the header. |
| 28 | Refuse to add positions beyond the `int` limit of the book format. |
| 29 | Choose the number of buckets from the number of positions (no fixed 2^26 limit) and grow them before a merge that would overload them. |

New option: `book-merge-auto-save` (`config.ini`, or `-book-merge-auto-save on/off`; default `on`) saves the book to `<book file>.mrg` after each successful `book merge`. The original book file is not overwritten, and the save on exit is unchanged. With `off`, `book merge` behaves exactly as before.

Builds: Windows with Visual Studio 2022 (MSVC 19.44), Linux with gcc 11.4 on Ubuntu 22.04, Android with NDK r27d, macOS x64 with the release-binaries workflow. The 32-bit Linux build links libatomic statically (i486 or later). The 32-bit macOS executable is still omitted.

## v4.5.5-nikque.2 and earlier: 18 bug fixes and build notes

This fork starts from upstream tag `v4.5.5` (`4cde6ff588f0eade07fcba0c7f02d5cd0cacd4ee`). It adds `book deviate2`, `book deviate3`, and configurable book autosave, and corrects the following 18 bugs. The original `book deviate` now re-probes its root after the first `book_expand`: `book_add` can move a hash bucket's `Position` array with `realloc`, so passing the old root pointer to the second `position_deviate` could access freed memory. Its selection rules are unchanged. See the [English](README-NIKQUE.en.md) and [Japanese](README-NIKQUE.ja.md) READMEs for command examples and the `book merge` workflow.

| ID | Corrected behavior |
|---|---|
| 01 | Re-probe the original `book deviate` root after expansion to avoid a stale pointer. |
| 02 | Read and write binary `.edx` files in binary mode on Windows. |
| 03 | Reject unsupported game-save extensions before opening a file. |
| 04 | Save books through a checked temporary file, then replace the destination only after a complete write. |
| 05 | Reject truncated or surplus book records, retain the active book on explicit load/merge failure, and avoid overwriting a malformed startup book. |
| 06 | Continue `book enhance` until expansion converges and save each productive round. |
| 07 | Handle XBoard `level 0` without dividing by zero and credit Fischer increment after a move. |
| 08 | Allow enough space for a dense FEN in PGN export. |
| 09 | Recognize the PGN FEN tag and round-trip the time tag. |
| 10 | Bound backward game-analysis history traversal and initialize pass history. |
| 11 | Honor destination capacity in string field and command parsing. |
| 12 | Reserve a terminator for maximal-length GGF fields. |
| 13 | Bound PGN move import at 60 placements. |
| 14 | Handle filenames shorter than four characters safely. |
| 15 | Return the actual result of GTP `reg_genmove`. |
| 16 | Reject invalid GTP `time_left` colors before indexing player clocks. |
| 17 | Reset GTP response IDs per command and pass them in the correct order on errors. |
| 18 | Format NBoard node counts and elapsed seconds with their actual types. |

The release bundle includes rebuilt Windows, Linux, macOS x64, and Android binaries, `bin/data/eval.dat`, the initial `bin/data/book.dat`, and the problem files. The evaluation data is copied unchanged from the upstream v4.5.5 distribution. The old 32-bit macOS `mEdax-x86` is omitted because current Xcode SDKs cannot link a corrected i386 binary. Run a binary from `bin/` or set `-eval-file` explicitly. To rebuild the Windows v4 binary, run `build-win-v4.cmd` from a Visual Studio 2022 x64 Developer Command Prompt; `.github/workflows/release-binaries.yaml` builds the other variants and `package-release.py` assembles the runtime ZIP. The AVX-512 build requires an x86-64-v4-capable CPU. Keep the source, this modification notice, and `LICENSE` when redistributing the binaries.

`book-save-interval` in `config.ini` is in minutes. `book-deviate-save-rounds` independently controls completed-round saves for all three deviate commands: `0` saves on completion, `1` after every productive round (the default), and `N` after every N productive rounds and on completion. The original deviate command counts its two expansion passes as one round. A book save needs enough free space for a second copy of the book while the temporary file is written. A failed write leaves the previous destination book in place. A malformed input book is rejected; the existing active book is retained for explicit `book load` and `book merge` commands.

Validation included regression cases for the corrected behavior, 300 random legal games and 549,161 flip comparisons, eight symmetries per position, 96 independent exact endgame positions at empties 1–12 with one and four search threads, Cassio endgame API checks, and event-queue checks.
