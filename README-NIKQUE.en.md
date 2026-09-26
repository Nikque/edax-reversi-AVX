# Edax 4.5.5 corrected build

[日本語](README-NIKQUE.ja.md) · [Releases](https://github.com/Nikque/edax-reversi-AVX/releases) · [Full audit fix list](RELEASE-NOTES.md)

This public fork is based on upstream `v4.5.5` (`4cde6ff588f0eade07fcba0c7f02d5cd0cacd4ee`). It publishes the modified source, a Windows x86-64-v4 executable, the original GPL-3.0 [license](LICENSE), and the changes described below. The upstream `master` branch remains available; `edax-4.5.5-fixes` is this fork's default branch.

## Book learning and maintenance

`book deviate2 <move-loss> <total-loss>` follows existing book links while limiting the evaluation loss of any single move and the cumulative loss across both players. For example, `book deviate2 5 5` permits a line with losses of 2 and 3, or one loss of 5; it excludes a loss of 6 or cumulative losses above 5. It selects eligible unexpanded leaves but skips leaves already solved to the book's exact-search level. `book deviate3` applies the same loss limits and includes those solved leaves, matching the earlier `deviate2` behavior. The original `book deviate` retains its original scoring rules.

Book autosave reads `book-save-interval` from `config.ini` in minutes; `0` disables timed saves. The original `book deviate` now re-probes its root after expansion because adding positions can relocate a hash bucket. Book merge/fix handles inconsistent `nomove` data without the previously observed crash. Book loading rejects malformed records, and saving writes to a checked temporary file before replacing the destination. An interrupted or failed save therefore leaves the previous book intact, but needs enough free space for roughly another copy of the book.

## Other corrected behavior

The 17-item audit also covers these fixes; [RELEASE-NOTES.md](RELEASE-NOTES.md) maps each one to A01–A17.

| Area | Changes |
|---|---|
| Files and book operations | Binary `.edx` handling on Windows; reject unsupported save extensions before opening files; safe temporary book saves; reject malformed book records and preserve the active/startup book; run `book enhance` until expansion converges. |
| Game and clock handling | Avoid division by zero for XBoard `level 0`; apply Fischer increments after moves; bound analysis history traversal and initialize pass history. |
| PGN and GGF | Reserve enough room for dense FEN and maximal GGF fields; recognize the PGN FEN tag and round-trip its time tag; limit PGN import to 60 placements. |
| Commands and protocols | Respect parser destination sizes and short filenames; return the actual GTP `reg_genmove` result; validate GTP `time_left` colors and reset response IDs per command; format NBoard counts and seconds with matching types. |

## Build and use

The release bundle includes Edax evaluation data at `bin/data/eval.dat`, copied byte-for-byte from the [upstream v4.5.5 distribution](https://github.com/okuhara/edax-reversi-AVX/releases/tag/v4.5.5) (SHA-256 `f8b2299612d9fa4414157e70e932636e33111c2602d0c2fc382a7d90ef21b792`). It also includes the upstream initial `bin/data/book.dat` and problem files. Run an executable from `bin/` so its default `data/eval.dat` path resolves, or set `-eval-file` explicitly. Choose the executable for your operating system and CPU; `wEdax-x86-64-v4.exe` requires an x86-64-v4 capable CPU (AVX-512). `config.ini` is a starting configuration; set paths and the save interval for your environment. To rebuild the Windows v4 executable, run `build-win-v4.cmd` from a Visual Studio 2022 x64 Developer Command Prompt. The [release-binaries workflow](.github/workflows/release-binaries.yaml) builds the other platform variants.

The 24 regression cases passed on the normal Windows build. Additional checks covered 300 legal games, 549,161 flip comparisons, 96 independent exact endgame positions with one and four search threads, Cassio calls, and event queue handling. The 28 GB user book was not modified or re-counted. AddressSanitizer could not run successfully in the final environment, so the normal-build tests and code review are the basis for the reported results. Details and remaining limits are in the [validation report](https://github.com/Nikque/edax-reversi-AVX/releases).

Work on this fork used **ChatGPT-6 Astra** and **ChatGPT-6 Sol**. Edax and its original authors retain their respective attribution. This fork is distributed under the original GPL-3.0 license; keep the source and license available when redistributing the executable.
