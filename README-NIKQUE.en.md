# Edax 4.5.5 corrected build

[日本語](README-NIKQUE.ja.md) · [Releases](https://github.com/Nikque/edax-reversi-AVX/releases) · [Change list](RELEASE-NOTES.md)

This public fork is based on upstream `v4.5.5` (`4cde6ff588f0eade07fcba0c7f02d5cd0cacd4ee`). It publishes the modified source, rebuilt Windows, Linux, macOS x64, and Android executables, the original GPL-3.0 [license](LICENSE), and the changes described below. The upstream `master` branch remains available; `edax-4.5.5-fixes` is this fork's default branch.

## Changes in v4.5.5-nikque.4

This release makes the search and book learning faster, lets a book hold up to 4.29 billion positions, and fixes one bug. With the same options the search results are unchanged: best moves, scores, principal variations and node counts match v4.5.5-nikque.3 in single-thread runs. Two new settings of `bin/config.ini` change the search when they are used: `hash-table-size = auto` (used by the bundled `config.ini`) and `book-expand-tasks` (1 by default, the original behavior). The book file format is unchanged.

### Search speed-ups (same results)

- Hash table cleanup: before each position searched by book learning, the three search hash tables were entirely rewritten (57 MB with the default size, 1.8 GB with `-h 26`). Now the old entries are only marked as empty through the date of the table and handled exactly as empty entries; the memory is wiped once every few dozen cleanups. Book learning at level 18 on one thread: `-h 21` 99.7 s → 97.3 s, `-h 24` 121.9 s → 107.6 s, `-h 26` 180.4 s → 108.1 s, with the same saved book.
- Windows: the locks of the hash tables, the searches and the parallel tasks were `CRITICAL_SECTION`. They are now a spin lock and an SRW lock, like the pthread locks used on the other systems.
- x86-64-v4 (AVX-512): the evaluation reads its 46 weights with three 16-lane gathers. Only the order of integer additions changes, so every evaluation is the same to the bit.
- The Windows executables (except ARM64) are built with profile-guided optimization (`vc-pgo-*` targets of `src/NMakefile`).

On one thread (Ryzen 9 9950X, x86-64-v4), 30 midgame positions at level 21 are solved 7 to 13% faster. For endgame solving the difference stays within the measurement noise of this PC (about ±5%).

### Book learning on several positions at the same time (new setting)

At low levels a search cannot use many threads: on 89 positions at level 18, 32 threads were only 2.5 times faster than one thread (4 threads: 2.1 times). With `book-expand-tasks = n` (`config.ini`, or `-book-expand-tasks n`), `book deviate`, `deviate2`, `deviate3`, `enhance` and `play` expand n positions at the same time, each with `n-tasks / n` threads and its own hash tables. The default 1 keeps the original one-by-one expansion.

Real book of 656.9 million positions, `book deviate3 2 5` stopped after its first 10,000 expansions, 32 threads:

| `book-expand-tasks` | Expansion time |
|---|---|
| 1 (two runs) | 831.9 s, 804.9 s |
| 16 (2 threads each) | 173.8 s |

The three books had the same positions and the same link moves. Leaves or scores differed in 258 positions between the two runs with 1, and in 484 and 493 positions between the run with 16 and each run with 1; every score difference was 1 or 2. Searches on several threads differ from run to run, and a search on 2 threads does not vary like one on 32 threads. Also, with n > 1 a new position does not see a position that another expansion of the same round adds at the same time (for example when two positions lead to the same child). Each expanding search has its own hash tables: with `hash-table-size = auto` they are sized for its threads (57 MB each for 2 threads).

### Hash table size setting

`hash-table-size = auto` (used by the bundled `config.ini`, or `-h auto`) chooses the size of the search hash tables from the number of search threads: 21 for 1 to 3 threads, 22 for 4 to 15, 23 for 16 to 63, at most 25 and at most 1/32 of the memory. A number keeps a fixed size (2^n entries, about 27 x 2^n bytes for the three tables); without `config.ini` the default is still 21. Like any change of `-h`, the size changes search results slightly. In tests at levels 18 and 24 with 32 threads, 23 was a little faster than 21, and larger sizes were not faster.

### Books of more than 2.1 billion positions

The position count in the book header is now read and written as an unsigned 32-bit number, so a book can hold up to 4,294,967,295 positions (was 2,147,483,647). Books below the old limit are saved byte for byte as before (checked with the real book: load time 29 s and memory 35.6 GiB as before, saved file identical except the date). A book of more than 2,147,483,647 positions cannot be read by earlier versions or by upstream Edax (they reject the count). About 240 GB of memory are needed for 4.29 billion positions.

### Output checks

Compared with v4.5.5-nikque.3 under the same conditions (fixed `-h`, `book-expand-tasks = 1`):

- Single-thread `-solve`: fforum-20-39 and 30 midgame positions on the Windows x86-64, v3, v4, x86 and x86-sse builds and the Linux x86-64, v3, v4 and x86 builds; fforum-1-19 and fforum-40-59 on Windows v4. Same best moves, scores, principal variations and node counts.
- fforum-40-59 on 32 threads (two runs of each version): same scores; best moves differ only between moves of equal score, as between two runs of the previous version.
- The book test suites (all book commands on 1 and 8 threads, merges, the 6.49-million-position book): same saved books. The log differences are the known ones: random choice among equal book moves, `.edx` values, times, and source line numbers in error messages.
- The real book: loaded and saved identical except the save date.

The ARM64 Windows and Android executables were built but not run.

### Bug fix

| Issue | Previous behavior |
|---|---|
| Stopping search threads | Freeing search threads soon after creating them could lose the stop signal (Edax then waited forever) or free a task whose thread had not started. |

## Changes in v4.5.5-nikque.3

This release makes `book deviate`, `book deviate2`, `book deviate3`, and `book merge` much faster on large books (hundreds of millions of positions, tens of GB), reduces book memory, fixes 11 bugs, and adds an option to save the book automatically after `book merge`. The book file format is unchanged: existing books load and save as before.

### Time and memory

Measured on a real 28.95 GB book of 656.9 million positions (Ryzen 9 9950X, 32 logical CPUs, with another Edax process running on the same machine).

| Operation (whole book) | v4.5.5-nikque.2 | v4.5.5-nikque.3 |
|---|---|---|
| Load the book | 445.6 s | 48.9 s |
| Memory after loading | 58.4 GiB | 35.5 GiB |
| Save the book | 657.2 s | 28.3 s |
| Negamax (run by the deviate commands at start and after every round) | 936.5 s | about 33 s |
| One round of `book deviate 2 4` (nothing to expand) | 2,909.6 s | 142.0 s |
| Selecting the positions to expand, deviate2 / deviate3 | 446.8 s / 479.2 s | 21.4 s / 22.9 s |
| `book merge` of two 660-million-position books (below) | 16,408 s (4 h 33 min), 117.2 GiB | 263 s, 37.0 GiB |

When many positions are selected, a `deviate2` or `deviate3` round spends most of its time searching them (two searches per position: the new child and the parent). That search time is unchanged.

### `book merge` measurement

Another real book (657 million positions, 29.03 GB) was merged into the 28.95 GB real book of 657 million positions (1.736 million positions added). Both released `wEdax-x86-64-v4.exe` builds were run one at a time on the same PC with the same books.

| Step | v4.5.5-nikque.2 | v4.5.5-nikque.3 | nikque.3 without leaf reuse (reference) |
|---|---|---|---|
| Load the book at startup | 222 s | 28 s | 30 s |
| Whole `book merge` command | 16,408 s (4 h 33 min) | 263 s | 10,934 s (3 h 2 min) |
| of which link rebuild | 15,332 s | 177 s | 10,845 s |
| `book save` after the merge | 251 s | 18 s | 62 s |
| Start to exit | 17,364 s (4 h 49 min) | 315 s | 11,027 s |
| Peak memory | 117.2 GiB | 37.0 GiB | 37.0 GiB |

Most of the merge time is spent searching again the positions whose best move without a link (the Leaf) became a link when the links were rebuilt (about 184,000 positions here). When the source book has already searched such a position at the same level, v4.5.5-nikque.3 reuses that result instead of searching again. This alone makes the merge about 42 times faster (10,934 s to 263 s). The other changes (streaming the source book, parallel processing and so on) make it about 1.5 times faster and reduce the memory from 117.2 GiB to 37.0 GiB.

**Because of this reuse, a few scores of the merged book may differ slightly.** The reused Leaf move is the move a new search should find, but its score can depend on the hash table contents and on the progress of the parallel search. In this merge, 27,897 of 659 million positions (0.004%) have scores that differ from nikque.2: 14,200 directly, the others through negamax to their parents. Most differences are 1 or 2. The set of positions, the link moves, the win/draw/loss counts and the levels are identical. nikque.2 itself gives different scores for the positions it searches again from one run to the next: in a merge of two reduced real books (34.84 million positions), two runs of nikque.2 differed in 1,655 positions, about as many as between nikque.2 and nikque.3 (about 2,500).

Main changes:

- Books are read and written through a 16 MB buffer (previously about 15 small reads or writes per position).
- A position takes 56 bytes in memory instead of 64. Up to 4 links are stored inside the position (99% of the positions of a large book) instead of a separate allocation.
- A book saved by Edax is loaded into one exactly sized block (previously about 20% of the arrays were spare capacity).
- Negamax, the selection of the positions to expand, and the link rebuild, check and sort of `book merge` run on `n-tasks` threads (the number of CPUs by default). The results are the same as with one thread.
- Each round no longer rewrites a flag in every position or scans the whole book for positions to expand. Positions are expanded in the same order as before.
- `book merge` reads the source file twice (first to check it, then to add positions) instead of loading it, and leaves the destination unchanged if the source is damaged. When the source book has already searched a position, its result (the best move without a link) is reused instead of searching again (scores may differ slightly; see "`book merge` measurement" above).

Use `n-tasks` (`-n`) to limit the threads if you play or analyze games while a book is learning.

### Automatic save after merge (new setting)

When `book-merge-auto-save` in `bin/config.ini` is `on` (the default), a successful `book merge` saves the merged book to `<book file>.mrg` (`data/book.dat.mrg` by default) and prints `Merged book saved to data/book.dat.mrg`. No separate `book save` is needed. As with the deviate progress files (`.dev`, `.dev2`, `.dev3`), the name is the configured book file name plus an extension; it is the same when another book was opened with `book load`.

- To use the `.mrg` file, load it with `book load data/book.dat.mrg` or rename it. The automatic save never overwrites the original book (`data/book.dat`).
- Nothing is saved when the merge fails (for example, a missing or malformed source).
- The save on exit is unchanged: if you quit Edax after a merge without `book save`, the book is saved to the configured book file (`data/book.dat`), as before.
- With `off`, no `.mrg` file is written and nothing is printed; Edax behaves exactly as before. The command-line option `-book-merge-auto-save off` does the same.

### Output checks

The following commands were run with the previous and the new version under the same conditions, and their outputs were compared. The only differences are the ones that also change between two runs of the previous version: the random choice among equally scored book moves, values stored in `.edx` files, and displayed times and speeds. The `.mrg` file and message added by the new automatic merge save are outside this comparison (with `book-merge-auto-save = off` the output matches the previous version exactly).

- book: `new`, `load`, `save`, `import`, `export`, `merge`, `info`, `stats`, `show`, `analyze`, `fix`, `negamax`, `correct`, `prune`, `subtree`, `add`, `check`, `problem`, `extract`, `deviate`, `deviate2`, `deviate3`, `enhance`, `play`, `deepen`, `feed-hash`, `store`, `depth`, `randomness`, `on`, `off`
- game: `play`, `go`, `hint`, `save`, `load`, `vmirror`, `hmirror`, `rotate`, `undo`, `redo`, `setboard`

The checks used one and eight threads, small generated books and a 6.49-million-position book cut from the real book, and five Windows builds (x86-64, v3, v4, x86, x86-sse). The search hash handling is unchanged.

Three outputs change on purpose:

- After `book merge`, the scores of the positions whose source search result was reused when the links were rebuilt, and of their parents (they may differ slightly; see "`book merge` measurement").
- The scores of positions added by `book merge` that cannot be reached from the initial position (they used to keep values such as +/-127).
- The order of the saved positions after a `book merge` whose result exceeds 32 positions per bucket of the destination (for example a large merge into a book just created with `book new`), because the number of buckets is increased. The positions are the same.

### Comparison with upstream v4.5.5

The executables of the upstream `edax-4.5.5.zip` (SHA-256 `6f446149092b37cbaa57f68a5610c6e960d45c4f0cbb9a3c8e7a563b503fa9a4`) and v4.5.5-nikque.3 were run under the same conditions and their outputs compared (Windows x86-64-v4, x86-64 and x86 builds). `deviate2`, `deviate3` and the save settings, which upstream does not have, were left out.

The saved books, text outputs and screen output match upstream for:

- a 6.49-million-position book cut from the real book: `load`, `stats`, `negamax`, `export`, `show`, `subtree`, `prune`;
- generated books: `new`, `deviate`, `enhance`, `negamax`, `fix`, `correct`, `export`, `import`, `subtree`, `prune`, `problem`, `extract`, `check`, `add`, `depth`, `store`, `play`, `deepen`, and the game commands (`play`, `go`, `hint`, `save`, `undo`, `redo`, and others).

The differences from upstream all come from fixes or display changes:

- Upstream crashes (fixed): displaying a position without any move (pass only) crashes upstream (with the real book, `book info` does). A stale `nomove` Leaf in the destination makes `book fix` crash after a merge.
- `book merge`: upstream does not rebuild Links; it prints "Book needs to be fixed before usage" and expects `book fix`. Running `book fix` right after the merge gives the same saved book as this version, which rebuilds, repairs, negamaxes and sorts the book during the merge. A missing or truncated source file is rejected here; upstream merged an empty book or the positions read before the truncation.
- `.edx` save: upstream writes the file in text mode on Windows, so a saved `.edx` cannot be read back correctly.
- Save on exit: upstream saves to the configured book file (`data/book.dat`) on exit even after `book save`. This version saves on exit only if the book changed after the last explicit save.
- Display: upstream also printed intermediate `todo` counts of `book deviate` every 10 positions; this version prints the final count only. The "Memory occupation" of `book info` follows the new in-memory size of a position.

While `book load` reads another book, the current book is kept so that it survives a failed load (since v4.5.5-nikque.2), so memory for both books is needed temporarily. Loading at startup and `book merge` do not need this extra memory.

### Bug fixes

| Issue | Previous behavior |
|---|---|
| Links to positions missing from the book | Negamax and other walks crashed. `book fix` now removes these links. |
| Commands sent through a pipe | A `quit` received while the engine was starting (loading the book) crashed Edax. |
| `book fill` | Could write to freed memory while adding positions, and crash. |
| Adding a position when memory is exhausted | The position was silently lost. Learning now stops. |
| Saving to disk | The file replaced the previous book before its data was flushed to disk. |
| Scores of positions added by a merge | Positions not reachable from the initial position kept +/-127. |
| Loading an empty or truncated `.edx` file, or one with an illegal move | The current game was erased. The file is now rejected and the game kept. |
| Windows clock | Wrapped around after 49.7 days of uptime. |
| Book header date | An uninitialized padding byte was written. |
| Number of positions | Overflowed beyond about 2.1 billion positions. Additions are now refused. |
| Number of buckets | Limited to 2^26, so searches slowed down beyond 1 billion positions. It now depends on the number of positions. |

[RELEASE-NOTES.md](RELEASE-NOTES.md) lists every change.

## Book learning and maintenance

Use these commands at the Edax prompt with the book loaded. They start from the current board, follow existing book Links, and expand eligible Leaves. A Leaf is a candidate move without a linked child position; the commands do not enumerate arbitrary unregistered moves. They stop at the configured book depth and repeat expansion until no more eligible positions or links are added.

| Command | Selection rule | Use when |
|---|---|---|
| `book deviate 2 4` | Original relative and root-score error limits; retains the original selection rule. | Continuing an existing `book deviate` workflow. |
| `book deviate2 5 5` | At most 5 points of loss on any move and 5 points in total across both players; skips Leaves at a fully solved book level. | Avoiding work on already solved Leaves. |
| `book deviate3 5 5` | Same per-move and cumulative limits, including fully solved Leaves. | Reproducing the earlier `deviate2` selection behavior. |

For `deviate2` and `deviate3`, a line with black losing 2 points and white losing 3 is eligible, as is one move losing 5; a move losing 6 or losses totaling 6 are excluded. The `todo` count is the number of eligible Leaf positions found during that pass, not a count of distinct game records. The commands write progress books with `.dev`, `.dev2`, or `.dev3` appended to the configured book filename.

`bin/config.ini` controls two independent save triggers for all three deviate commands. `book-save-interval` is the timed-save interval in minutes (`0` disables timed saves during expansion). `book-deviate-save-rounds` counts productive completed rounds: `1` keeps the original save-after-every-round behavior, `10` saves after every 10 rounds and at completion, and `0` saves only at completion apart from any timed saves. One round of original `book deviate` includes both of its expansion passes; one round of `deviate2` or `deviate3` is one expansion pass. A failed or interrupted run can lose work since the last save, especially with `0` and timed saves disabled. The default is `1` for compatibility.

### `book deviate` fix

The original `book deviate` had a pointer lifetime bug. Its first `book_expand` may call `book_add`, which can move a hash bucket's `Position` array with `realloc`. The second `position_deviate` pass previously reused the old `root` pointer and could read freed memory, potentially interrupting expansion or leaving positions missing. It now re-probes the root from the starting board after expansion. Its selection rules are unchanged; this fix does not retroactively fill gaps in an existing book.

### `book merge` and `book fix`

To combine books, load the destination book and run `book merge source.dat`. With `book-merge-auto-save = on` (the default) the result is saved to `<book file>.mrg` automatically; use `book save merged.dat` to keep it under another name or when the setting is `off`. Merge adds positions that exist only in the source; it does not overwrite positions already in the destination. It then rebuilds Links, repairs inconsistent positions (including stale `nomove` Leaves), recomputes scores and sorts moves. When the link rebuild empties the Leaf of a destination position, the Leaf of the same position in the source is used if it is still not a Link; otherwise the position is searched. A separate `book fix` remains available to repair the current book; it is not a prerequisite for this merge path. If the source file is missing or structurally malformed, merge is rejected and the current book is retained. During a merge, memory is needed for the destination book only.

Book saving writes to a temporary file, flushes it to disk, then replaces the destination. An interrupted or failed save leaves the previous book intact, but needs enough free space for roughly another copy of the book.

## Other corrected behavior (up to v4.5.5-nikque.2)

Up to v4.5.5-nikque.2 this fork corrected 18 bugs, including the original `book deviate` pointer bug; [RELEASE-NOTES.md](RELEASE-NOTES.md) lists all of them.

| Area | Changes |
|---|---|
| Files and book operations | Binary `.edx` handling on Windows; reject unsupported save extensions before opening files; safe temporary book saves; reject malformed book records and preserve the active/startup book; run `book enhance` until expansion converges. |
| Game and clock handling | Avoid division by zero for XBoard `level 0`; apply Fischer increments after moves; bound analysis history traversal and initialize pass history. |
| PGN and GGF | Reserve enough room for dense FEN and maximal GGF fields; recognize the PGN FEN tag and round-trip its time tag; limit PGN import to 60 placements. |
| Commands and protocols | Respect parser destination sizes and short filenames; return the actual GTP `reg_genmove` result; validate GTP `time_left` colors and reset response IDs per command; format NBoard counts and seconds with matching types. |

## Build and use

The release bundle includes Edax evaluation data at `bin/data/eval.dat`, copied byte-for-byte from the [upstream v4.5.5 distribution](https://github.com/okuhara/edax-reversi-AVX/releases/tag/v4.5.5) (SHA-256 `f8b2299612d9fa4414157e70e932636e33111c2602d0c2fc382a7d90ef21b792`). It also includes the upstream initial `bin/data/book.dat` and problem files. Run an executable from `bin/` so its default `data/eval.dat` path resolves, or set `-eval-file` explicitly. Choose from these packaged binaries:

| Environment | File in `bin/` |
|---|---|
| Windows x86-64, baseline / AVX2 / AVX-512 | `wEdax-x86-64.exe` / `wEdax-x86-64-v3.exe` / `wEdax-x86-64-v4.exe` |
| Windows 32-bit x86, baseline / SSE2 | `wEdax-x86.exe` / `wEdax-x86-sse.exe` |
| Windows ARM64 | `wEdax-arm64.exe` |
| Linux x86-64, baseline / AVX2 / AVX-512 | `lEdax-x86-64` / `lEdax-x86-64-v3` / `lEdax-x86-64-v4` |
| Linux 32-bit x86 | `lEdax-x86` |
| macOS Intel x86-64 | `mEdax-x64-modern` |
| Android ARM64 / 32-bit ARMv7 | `aEdax-arm64-v8a` / `aEdax-armeabi-v7a` |

The `v3` builds require an AVX2-capable x86-64 CPU; the `v4` builds require an AVX-512-capable x86-64-v4 CPU. Use the baseline build when unsure. `config.ini` is a starting configuration; set paths, `book-save-interval`, `book-deviate-save-rounds`, `book-merge-auto-save`, `hash-table-size`, and `book-expand-tasks` for your environment. To rebuild a Windows executable, open a Visual Studio 2022 Developer Command Prompt, change to `src`, and run a target such as `nmake -f NMakefile vc-x64-v4` (`build-win-v4.cmd` also builds the v4 executable). The [release-binaries workflow](.github/workflows/release-binaries.yaml) builds the other platform variants, and `package-release.py` assembles the runtime ZIP. The release Windows executables use the profile-guided targets (`vc-pgo-x64-v4`, `vc-pgo-x64-v3`, `vc-pgo-x64`, and `vc-pgo-x86-sse` and `vc-pgo-x86` from an x86 prompt) except ARM64. The v4.5.5-nikque.4 executables were built with Visual Studio 2022 (MSVC 19.44) for Windows, gcc 11.4 on Ubuntu 22.04 (WSL) for Linux and NDK r27d for Android; the macOS executable was built by the release-binaries workflow. The 32-bit Linux build (`lEdax-x86`) links libatomic statically for the atomic operations of the parallel book code, so it needs an i486 or later CPU.

The upstream 32-bit macOS `mEdax-x86` is deliberately omitted. Current Xcode SDKs lack the i386 libraries needed to link a corrected binary; including the upstream executable would leave this fork's fixes absent from that file.

Work on this fork used **ChatGPT-6 Astra** and **ChatGPT-6 Sol**; the performance work and bug fixes of v4.5.5-nikque.3 and v4.5.5-nikque.4 used **Claude Opus 5.5** (Claude Code). Edax and its original authors retain their respective attribution. This fork is distributed under the original GPL-3.0 license; keep the source and license available when redistributing the executable.
