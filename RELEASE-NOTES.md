# Edax 4.5.5: release notes

## v4.5.5-nikque.3

Book performance and 11 bug fixes. The book file format is unchanged. See the [English](README-NIKQUE.en.md) and [Japanese](README-NIKQUE.ja.md) READMEs for measurements and the output checks against v4.5.5-nikque.2.

Performance (the results of every command are unchanged, except where noted in the READMEs):

- Books are read and written through a 16 MB buffer.
- An in-memory position takes 56 bytes (was 64) and stores up to 4 links itself; a book saved by Edax is loaded into one exactly sized block.
- Negamax, the selection of the positions to expand by `book deviate`, `deviate2` and `deviate3`, and the link rebuild, check and sort of `book merge` run on `n-tasks` threads.
- `book_clean` no longer rewrites every position, and positions to expand are recorded instead of being searched for in the whole book (same expansion order).
- `deviate2`/`deviate3` process positions by increasing accumulated loss, so each position is walked once.
- `book merge` streams the source file (check pass, then merge pass) instead of loading it, and reuses the source Leaf of a relinked position instead of searching it again when the source Leaf is still not a Link.

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
