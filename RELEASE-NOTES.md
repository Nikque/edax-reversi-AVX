# Edax 4.5.5: 18 bug fixes and build notes

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

`book-save-interval` in `config.ini` is in minutes. A book save needs enough free space for a second copy of the book while the temporary file is written. A failed write leaves the previous destination book in place. A malformed input book is rejected; the existing active book is retained for explicit `book load` and `book merge` commands.

Validation included regression cases for the corrected behavior, 300 random legal games and 549,161 flip comparisons, eight symmetries per position, 96 independent exact endgame positions at empties 1–12 with one and four search threads, Cassio endgame API checks, and event-queue checks.
