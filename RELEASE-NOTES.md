# Edax 4.5.5 fixes and Windows build

This fork starts from upstream tag `v4.5.5` (`4cde6ff588f0eade07fcba0c7f02d5cd0cacd4ee`). It includes the `book deviate2` command, configurable book autosave interval, `book deviate3`, and the book root-pointer and merge corrections. This release also fixes the 17 issues identified in the 2026-09-27 audit.

| ID | Corrected behavior |
|---|---|
| A01 | Read and write binary `.edx` files in binary mode on Windows. |
| A02 | Reject unsupported game-save extensions before opening a file. |
| A03 | Save books through a checked temporary file, then replace the destination only after a complete write. |
| A04 | Reject truncated or surplus book records, retain the active book on explicit load/merge failure, and avoid overwriting a malformed startup book. |
| A05 | Continue `book enhance` until expansion converges and save each productive round. |
| A06 | Handle XBoard `level 0` without dividing by zero and credit Fischer increment after a move. |
| A07 | Allow enough space for a dense FEN in PGN export. |
| A08 | Recognize the PGN FEN tag and round-trip the time tag. |
| A09 | Bound backward game-analysis history traversal and initialize pass history. |
| A10 | Honor destination capacity in string field and command parsing. |
| A11 | Reserve a terminator for maximal-length GGF fields. |
| A12 | Bound PGN move import at 60 placements. |
| A13 | Handle filenames shorter than four characters safely. |
| A14 | Return the actual result of GTP `reg_genmove`. |
| A15 | Reject invalid GTP `time_left` colors before indexing player clocks. |
| A16 | Reset GTP response IDs per command and pass them in the correct order on errors. |
| A17 | Format NBoard node counts and elapsed seconds with their actual types. |

The release bundle includes binaries for the upstream v4.5.5 platform variants, `bin/data/eval.dat`, the initial `bin/data/book.dat`, and the problem files. The evaluation data is copied unchanged from the upstream v4.5.5 distribution. Run a binary from `bin/` or set `-eval-file` explicitly. To rebuild the Windows v4 binary, run `build-win-v4.cmd` from a Visual Studio 2022 x64 Developer Command Prompt; `.github/workflows/release-binaries.yaml` builds the other variants. The AVX-512 build requires an x86-64-v4-capable CPU. Keep the source, this modification notice, and `LICENSE` when redistributing the binaries.

`book-save-interval` in `config.ini` is in minutes. A book save needs enough free space for a second copy of the book while the temporary file is written. A failed write leaves the previous destination book in place. A malformed input book is rejected; the existing active book is retained for explicit `book load` and `book merge` commands.

Validation included the audit's regression cases for all 17 issues, 300 random legal games and 549,161 flip comparisons, eight symmetries per position, 96 independent exact endgame positions at empties 1–12 with one and four search threads, Cassio endgame API checks, and event-queue checks. The 28 GB user book was not modified or re-counted.
