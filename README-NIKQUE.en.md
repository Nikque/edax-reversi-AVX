# Edax 4.5.5 corrected build

[日本語](README-NIKQUE.ja.md) · [Releases](https://github.com/Nikque/edax-reversi-AVX/releases) · [Change list](RELEASE-NOTES.md)

This public fork is based on upstream `v4.5.5` (`4cde6ff588f0eade07fcba0c7f02d5cd0cacd4ee`). It publishes the modified source, rebuilt Windows, Linux, macOS x64, and Android executables, the original GPL-3.0 [license](LICENSE), and the changes described below. The upstream `master` branch remains available; `edax-4.5.5-fixes` is this fork's default branch.

## Changes in v4.5.5-nikque.6

The startup values of the main settings can now be set in `config.ini`, the limit on the number of legal moves is raised to its theoretical maximum, and `book merge` shows its progress again. The evaluation data `eval.dat`, the book file format and the search results with the same options are unchanged.

### Startup settings (new settings and defaults)

`config.ini` (or the command line) sets these four startup values. The bundled `config.ini` uses the values below, which are also the defaults without `config.ini`.

| Setting | Bundled value and default | Meaning |
|---|---|---|
| `level` | `18` | Search level (0 to 60). The `level` command changes it after startup. |
| `n-tasks` (`n`) | `auto` | Number of search threads: 1 to the number of logical CPUs, or `auto` (the number of logical CPUs). |
| `book-depth` | `auto` | Depth of the opening book (as with the `book depth` command). `auto` keeps the depth saved in the book file (`data/book.dat`); a number from 1 to 60 sets it at startup (it is saved with the book). |
| `book-usage` | `on` | Play from the opening book (`on`/`off`). |

- **The built-in default level is now 18 instead of 21.** Searches without `-l` (`-solve`, games) give different results from previous versions; with `-l` they are the same.
- **A `level` written in `config.ini` (or in `edax.ini` of the current folder) only sets the startup level.** It does not cap the search in timed games (`-t`, `-move-time`); the "no level cap in timed games" change of v4.5.5-nikque.5 is kept. To cap the search, give the level with `-l` on the command line or `level` at the Edax prompt (a `level` in a settings file read with `-o` is still a cap, as before).
- On the command line: `-n auto` and `-book-depth <n|auto>`.
- The `options` command of the Edax prompt shows the values (with a new "book depth at startup" line); `book info` shows the book depth (`Depth`).

### Easier config.ini syntax

`config.ini` and `edax.ini` are now read more tolerantly, to make them easy to edit by hand:

- `level = 18`, `level=18`, `level 18` and `set level 18` are the same. Spaces around `=`, tabs, full-width spaces and the full-width equal sign are accepted.
- Names ignore the case, and spaces, `_` and `-` in names are the same (`book-depth`, `book_depth`, `book depth` and `Book Depth` are the same setting).
- The values `on`/`off`/`auto`/`true`/`false`/`yes`/`no` ignore the case.
- With `=`, the value is the rest of the line (a file name may contain spaces).
- `#` starts a comment. A UTF-8 BOM and CRLF line ends are fine.
- An unknown name (a typo, for example) is reported at startup with the file name and the line number (for example `WARNING: config.ini:7: unknown or incomplete setting "levle" ignored`). Out-of-range values are reported and bounded, as before.

### Legal moves: limit 33 -> 34

The maximum number of legal moves of a position (`MAX_MOVE`) is raised from 33 to 34. Positions reachable from the starting position have at most 33 legal moves ([eukaryote 2023](https://eukaryote.hateblo.jp/entry/2023/05/23/145945), [a note on the paper](https://othlog.hasera.net/20231112-2/)), but positions with 34 legal moves exist when unreachable positions are included, and none has 35 or more ([eukaryote 2020](https://eukaryote.hateblo.jp/entry/2020/04/13/150458)). Edax accepts any position with `-solve` or `setboard`, so the move list can no longer overflow on such a position.

- The move list (`MoveList`) is a temporary variable of the search: it grows by 32 bytes (at most about 2 KB per thread). Book positions (48 bytes) and the hash tables do not contain it, so the book memory is unchanged. The moves are processed as a list of the actual legal moves, so there is no extra work.
- In single-thread runs, the search results and node counts are identical to the previous version and the time difference stays within the measurement noise (level 18: x0.995, midgame: x1.008, endgame: x1.002).

### Bug fix: progress of book merge and book fix

Since the linking and fixing steps became parallel (v4.5.5-nikque.3), `book merge` showed "Linking book..." and "Fixing book..." without any progress until "done" (upstream printed it every 100,000 positions). The progress is now printed once per second; the book is unchanged.

- Checking the positions (parallel): `Linking book...3654055/6491163 positions checked` (positions checked / all positions)
- Adding links and searching leaves: `Linking book...16/17550 positions linked` (positions done / positions to process)
- `book fix` (and the fix after a merge) shows `Fixing book...` the same way.
- While `book merge` reads the file of a large book, `Checking book ...` and `Merging book ...` show the positions read (new: upstream showed nothing there).

## Changes in v4.5.5-nikque.5

This release makes Edax use its clock properly in timed games, which makes it stronger as a playing program, and reduces the book memory by about 14%. **Fixed-level searches (book learning, `-solve`, games at a given level) give the same results**: best moves, scores, principal variations and node counts match v4.5.5-nikque.4 in single-thread runs. The book file format and the saved book contents are unchanged (the only exception is the `book enhance` note in "Book memory" below). The evaluation data `eval.dat` is unchanged.

There are two new settings: `book-expand-tasks = auto` (used by the bundled `config.ini`) and the experimental `probcut-model = refit` (the default remains `standard`).

### Summary

| | v4.5.5-nikque.4 | v4.5.5-nikque.5 |
|---|---|---|
| Timed games, 16 s per game, 1 thread | uses about half of its clock | uses its clock; +52 Elo |
| Same, 64 s per game | uses less than 20% of its clock | +88 Elo |
| Same, 4 s per game | - | +5 Elo (time shared from the measured search speed) |
| Book memory (656.9 million positions) | 35.55 GiB (56 bytes per position) | 30.64 GiB (48 bytes per position) |
| Temporary memory while `book deviate`/`deviate2`/`deviate3` select positions | - | 1 byte per position + 4 bytes per bucket (about 0.9 GB for 656.9 million positions) |
| `book-expand-tasks` of the bundled `config.ini` | `1` | `auto` (16 positions at a time at level 18 or below with 32 threads) |

Elo differences come from matches between two versions under the same conditions, from balanced start positions with colors swapped (see "How strength was measured" below).

### Clock use in games

- **No level cap in timed games.** With `-t` (time per game) or `-move-time` (time per move), Edax used to stop searching at the default level 21: it used about half of a 16 s clock and less than 20% of a 64 s clock. Without an explicit level, the cap is now 60 and Edax searches until its time is used. An explicit level (`-l`, `level`, xboard `sd`, NBoard `depth`) remains the cap, as before. GTP already used 60.
- **Time shared from the measured search speed.** Edax estimated how many empty squares it can solve with a fixed assumption of 10 million nodes per second (`-speed`). It now measures its speed after each search and uses twice that value. An explicit `-speed n` is used as before (`-speed auto` restores the measured speed).

Fixed-level searches (book learning, `-solve`, games at a given level) are not affected by either change.

1 thread, 2204 games (1102 start positions, both colors), node-count clock (`-nps 20000000`, see below):

| Change | 4 s per game | 16 s per game | 64 s per game |
|---|---|---|---|
| No level cap | +1.3 [−9.0, +11.5] | +52.3 [+42.3, +62.3] | +87.5 [+60.7, +115.4] (300 games) |
| Time shared from the measured speed (x2) | +5.7 [+2.1, +9.2]; other start positions +4.6 [+1.1, +8.2] | (`-speed 4e7`) +1.7 [+0.5, +2.9] | - |

[ ] is the 95% confidence interval. In real time (16 s per game, 1 thread, 600 games, each game pinned to one physical core), both changes together gave +90.0 [+70.7, +109.8] (average time per move 0.25 s -> 0.56 s).

### Book memory (same file, same contents)

A position in memory now takes 48 bytes instead of 56. With the real book of 656.9 million positions, the memory after loading went from 35.55 GiB to 30.64 GiB (−4.91 GiB, −13.8%). The file format is unchanged: books can be exchanged with previous versions in both directions.

- **The deviate2 working value is no longer a field of every position.** `book deviate2`/`deviate3` (and the parallel `book deviate`) kept their visit marks (smallest accumulated loss, etc.) in 4 bytes of every position. The marks are only used while positions are selected, so each selection now allocates a table of 1 byte per position and frees it at the end (about 0.9 GB for 656.9 million positions, during the selection only).
- **Scores (value, lower, upper) take 1 byte each** in memory; the file still stores 2 bytes each.
- **The done and todo marks share 1 byte.** The learning epoch now has 5 bits instead of 7, so the full reset of the marks happens once every 31 epochs instead of 127 (a few seconds for 656.9 million positions; a learning round uses several epochs).

**Notes:**

- **`book enhance` with errors above 63 may give different results.** Score bounds (score ± error) that leave the 1-byte range (±127) are saturated to ±127. Errors of 63 or less are not affected. In a test (`book enhance 64 64` and `100 100` on a level 4 book) the books were the same as before (the range is only exceeded when a large error is added to a large score). When a book with out-of-range bounds (made with such errors by a previous version) is loaded, these bounds are saturated to ±127.
- The total loss of `book deviate2`/`deviate3` (second argument) is limited to 254; a larger value is reduced to 254 with a warning.
- With the real book, the position selection of `book deviate3 2 5` (first round, 12,308,548 positions) took 15.5 s instead of 12.6 s (allocation of the marks and a hash computation per position). The same positions were selected.

Checks: in the book regression tests (every book command with 1 and 8 threads; negamax, subtree and prune of a 6.49-million-position book; merge), every saved book matched v4.5.5-nikque.4 byte for byte (except the `book enhance 64 64` test from `book new 0 3`, whose result changes from run to run even with the same version). With the 6.49-million-position book, the peak memory went from 1.214 GiB to 1.066 GiB.

### book-expand-tasks = auto

`book-expand-tasks = auto` (the value of the bundled `config.ini`; `-book-expand-tasks auto` on the command line) chooses the number of positions expanded at the same time from the book level: each search uses 2 threads at level 18 or below, 4 up to level 24 and 8 above, and `n-tasks` divided by that number of positions are expanded at the same time (16 positions with 32 threads at level 18). Without `config.ini`, the default remains 1. Only level 18 was measured (2 threads per search was the fastest). The book is not the same as with one-by-one expansion: read the notes of "Book learning on several positions at the same time" in v4.5.5-nikque.4 below.

### Experimental setting: probcut-model = refit

This is a refit of the error model of the search pruning (ProbCut: a shallow search predicts the result of a deep one, and moves that are unlikely to matter are cut). The default remains `standard`; `refit` is for tests.

- Fit: exact fixed-depth scores (no pruning) at depths 0 to 16 (20 for 304 of them) of 1200 positions with 24 to 52 empty squares, taken from learning lines. The difference between the shallow and the deep search hardly depends on the depths and decreases with the number of empty squares (about 2.4 discs at 24 empties, 2.0 at 36, 1.4 at 48). The standard model grows with the depth. `refit` uses the fitted model with the t values of the selectivity levels (73% to 99%) scaled by 1.3.
- Level 18 against level 18 (2204 games): +3.5 [−4.4, +11.3], and +16.5 [+8.3, +24.7] from other start positions, in 0.92 times the time. Level 21: +2.2 (0.84 times the time); level 24: +10.0 (0.86 times the time).
- Score accuracy (mean absolute error of the level 18 score against level 26, 1200 positions):

| Empty squares | standard: error / nodes | refit: error / nodes |
|---|---|---|
| 24-29 | 2.106 / 2.25 G | 2.186 / 1.66 G |
| 30-35 | 1.986 / 0.65 G | 1.983 / 0.66 G |
| 36-43 | 0.884 / 1.07 G | 0.859 / 1.25 G |
| 44-52 | 0.632 / 0.37 G | 0.576 / 0.51 G |

In the opening and middle game (36 empties or more) it searches more and scores are a little more accurate; near the endgame (24-29 empties) it searches less and scores are a little less accurate. Used for book learning, it learns the opening and middle game more slowly and gives a different book than `standard`. Search results change: check carefully before mixing it with `standard` books.

### How strength was measured, and changes not taken

Two versions played two games (colors swapped) from each of 1102 balanced start positions (random 8-move openings scored within ±3 at level 18; 1087 10-move openings were also used) over the NBoard protocol. Elo and its 95% confidence interval were computed on the pairs of games. Timed comparisons used `-nps`, which counts the search clock in nodes: results do not depend on the CPU load or on the speed of each core, and a match is exactly reproducible (with `-nps`, the game clock is now also charged in nodes).

Changes that improved neither strength nor speed (1 thread), and were not taken:

- Search constants that only change node counts: `LASTFLIP_HIGHCUT`/`LASTFLIP_LOWCUT`, `ETC_MIN_DEPTH` 4 and 6, `DEPTH_MIDGAME_TO_ENDGAME` 13 and 17, move sorting depths (`inc-*-sort-depth`): speed differences within the measurement noise (±3%) or slower.
- ProbCut strength (t) and shallow depth (`probcut-d`): the defaults were close to the best (a lower t was much weaker, a higher one no better).
- Move sorting in depth-2 searches: 5.5% fewer nodes, but 8-13% slower.

## Changes in v4.5.5-nikque.4

This release makes the search and book learning faster, lets a book hold up to 4.29 billion positions, and fixes one bug. With the same options the search results are unchanged: best moves, scores, principal variations and node counts match v4.5.5-nikque.3 in single-thread runs. Two new settings of `bin/config.ini` change the search when they are used: `hash-table-size = auto` (used by the bundled `config.ini`) and `book-expand-tasks` (1 by default, the original behavior). The book file format is unchanged.

### Summary

| | v4.5.5-nikque.3 | v4.5.5-nikque.4 |
|---|---|---|
| Book learning on 32 threads, real 657M-position book (`book deviate3 2 5`, 10,000 expansions) | 832 s / 805 s | 174 s with `book-expand-tasks = 16` (4.7 times faster), 1 by default |
| Book learning at level 18 on one thread with a large hash table (`-h 26`) | 180 s | 108 s |
| Same with the default hash table (`-h 21`) | 100 s | 97 s |
| Search, 30 midgame positions at level 21, one thread (x86-64-v4) | 1 | 1.07 to 1.13 times faster |
| Maximum number of positions in a book | 2,147,483,647 | 4,294,967,295 |
| Book memory (657M positions) | 35.6 GiB | 35.6 GiB (unchanged) |
| Search hash tables with the bundled `config.ini` | 57 MB (`-h 21`) | chosen from the threads (`hash-table-size = auto`): 57 MB for 1-3 threads, 226 MB for 16-63 threads |
| Extra memory of `book-expand-tasks = n` | - | n hash tables (with `auto`, 57 MB each for 1-3 threads per position) |

See "Settings (config.ini)" below for how to choose `book-expand-tasks` and `hash-table-size`.

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

**Before using `book-expand-tasks` above 1:**

- **The resulting book differs from a one-by-one run.** The expanded positions and the link moves were the same in the test above, but some Leaves (the best moves not yet linked) and scores differ, by 1 or 2 points, about twice as often as between two one-by-one runs on 32 threads.
- **Positions expanded at the same time do not see each other.** When two positions of the same round lead to the same new position, or when a new position is a child of another new position of the round, the link is not made at that moment (a one-by-one run would make it). It can be made in a later round.
- **It pays when a round has many positions to expand.** Each position still uses `n-tasks / n` threads for its searches. A round with fewer positions than n uses fewer searches, and a round with one position uses the one-by-one expansion. On a small book whose rounds have only a few positions, the gain is small.
- **Choose a smaller n at higher levels.** Only level 18 was measured, where 2 threads per position (n = 16 with 32 threads) was fastest. A deeper search uses its threads better, so at higher levels start with fewer positions at the same time (for example n = 4 or 8 with 32 threads) and compare the speed.
- **Memory:** each of the n searches has its own hash tables (with `hash-table-size = auto`, sized for its threads; with a fixed `hash-table-size`, n times that size).
- Timed saves (`book-save-interval`) wait until the positions being written are finished; the other searches continue.
- `book-expand-tasks` cannot exceed `n-tasks`. Lower `n-tasks` (and `book-expand-tasks`) if you play or analyze with Edax while a book is learning.

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

## Settings (config.ini)

Edax reads its settings in this order, each one overriding the previous:

1. `edax.ini` in the current folder, if present;
2. `config.ini` in the folder of the executable (`bin/config.ini` of the release);
3. the command-line options.

Each line of these files is `name = value`; `#` starts a comment (see "Easier config.ini syntax" in v4.5.5-nikque.6 for the accepted forms). Every command-line option can be written there with its long name without the leading `-` (for example `n-tasks = 16` for `-n 16`, `level = 18` for `-l 18`, `book-file = data/book.dat`). At the Edax prompt, typing `name value` (for example `book-expand-tasks 16` or `n-tasks 16`) changes a setting for the next searches and learning commands; `hash-table-size` only takes effect at startup.

### Settings of the bundled config.ini

| Setting | Bundled value | Meaning |
|---|---|---|
| `level` | `18` | Startup search level. It does not cap timed games (see v4.5.5-nikque.6 above). |
| `n-tasks` | `auto` | Number of search threads (`auto`: the number of logical CPUs). |
| `book-depth` | `auto` | Book depth at startup (`auto`: the value of the book file). |
| `book-usage` | `on` | Play from the opening book. |
| `book-save-interval` | `360` | Minutes between timed saves of the book during learning (`0`: no timed save). |
| `book-deviate-save-rounds` | `1` | Save after this many productive rounds of `book deviate`, `deviate2`, `deviate3` (`0`: only when learning ends). |
| `book-merge-auto-save` | `on` | Save the merged book to `<book file>.mrg` after each successful `book merge`. |
| `hash-table-size` | `auto` | Size of the search hash tables (below). |
| `book-expand-tasks` | `auto` | Number of book positions expanded at the same time by the learning commands (below). |
| `probcut-model` | `standard` | Error model of the search pruning (ProbCut); `refit` is experimental (see v4.5.5-nikque.5 above). |

Other useful settings: `book-file` and `eval-file` (paths). A level given with `-l` on the command line (or `level` at the prompt) caps the search in timed games.

### hash-table-size

The search keeps what it has already searched in three hash tables. `hash-table-size = n` gives the main table 2^n entries (24 bytes each), and two smaller tables 2^(n-4) entries each, about 27 x 2^n bytes in total:

| n | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 28 | 30 |
|---|---|---|---|---|---|---|---|---|---|
| Memory | 57 MB | 113 MB | 226 MB | 453 MB | 906 MB | 1.8 GB | 3.6 GB | 7.2 GB | 29 GB |

Values from 10 to 30 are accepted (10 to 25 for the 32-bit executables). `auto` chooses 21 for 1 to 3 search threads, 22 for 4 to 15 and 23 for 16 to 63, at most 25 and at most 1/32 of the memory. Without `config.ini` the default is 21.

- `auto` is the recommended value for book learning and games. In tests with 32 threads at levels 18 and 24, 23 was a little faster than 21, and larger tables were not faster.
- Larger tables can help long searches (deep analysis, endgame solving with many empty squares). They no longer slow book learning down: since v4.5.5-nikque.4 the tables are not rewritten before each searched book position.
- The table size changes search results slightly. To reproduce a result exactly, use the same number (and the same `n-tasks`).
- With `book-expand-tasks = n`, each of the n searches has its own tables: with `auto` they are sized for the threads of each search, with a number each search uses that size.

### book-expand-tasks

With `book-expand-tasks = 1` (the default without `config.ini`), the learning commands (`book deviate`, `deviate2`, `deviate3`, `enhance`, `play`) expand the selected positions one by one, each search using all `n-tasks` threads, exactly as before. With `book-expand-tasks = n`, n positions are expanded at the same time, each with `n-tasks / n` threads.

`book-expand-tasks = auto` (bundled value) chooses n from the book level (each search uses 2 threads at level 18 or below, 4 up to level 24 and 8 above; n is `n-tasks` divided by that number).

A search at a low level cannot use many threads: at level 18, 32 threads were only 2.5 times faster than one thread. Expanding several positions at the same time uses the other threads. With 32 threads at level 18, `book-expand-tasks = 16` made learning on the real book 4.7 times faster. Read the notes in "Book learning on several positions at the same time" above before using it: the resulting book is not the same as with 1.

Suggested starting values (measure with your own books and levels):

| Use | `n-tasks` | `book-expand-tasks` | `hash-table-size` |
|---|---|---|---|
| Games and analysis, or learning that must reproduce the previous behavior | all CPUs (default) | 1 | auto |
| Book learning at level 18 or lower with 32 logical CPUs | 32 | auto (= 16, 2 threads each) | auto |
| Book learning at higher levels with 32 logical CPUs | 32 | auto (8 up to level 24, 4 above), or 4 to 8 after comparing speeds | auto |
| Book learning while using the PC for other work | about half of the CPUs | half of `n-tasks` or less | auto |

Example `config.ini` for book learning at level 18 on a 32-thread PC:

```
book-save-interval = 360
book-deviate-save-rounds = 1
book-merge-auto-save = on
n-tasks = 32
hash-table-size = auto
book-expand-tasks = auto
```

The book memory does not depend on these settings (about 50 bytes per position with its links, 30.6 GiB for 657 million positions; about 58 bytes and 35.6 GiB up to v4.5.5-nikque.4); add the hash tables (for example 16 x 57 MB = 0.9 GB above), and 1 byte per position while `book deviate`/`deviate2`/`deviate3` select positions.

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

The `v3` builds require an AVX2-capable x86-64 CPU; the `v4` builds require an AVX-512-capable x86-64-v4 CPU. Use the baseline build when unsure. `config.ini` is a starting configuration (see "Settings (config.ini)" above); set paths, `book-save-interval`, `book-deviate-save-rounds`, `book-merge-auto-save`, `hash-table-size`, and `book-expand-tasks` for your environment. To rebuild a Windows executable, open a Visual Studio 2022 Developer Command Prompt, change to `src`, and run a target such as `nmake -f NMakefile vc-x64-v4` (`build-win-v4.cmd` also builds the v4 executable). The [release-binaries workflow](.github/workflows/release-binaries.yaml) builds the other platform variants, and `package-release.py` assembles the runtime ZIP. The release Windows executables use the profile-guided targets (`vc-pgo-x64-v4`, `vc-pgo-x64-v3`, `vc-pgo-x64`, and `vc-pgo-x86-sse` and `vc-pgo-x86` from an x86 prompt) except ARM64. The v4.5.5-nikque.4 executables were built with Visual Studio 2022 (MSVC 19.44) for Windows, gcc 11.4 on Ubuntu 22.04 (WSL) for Linux and NDK r27d for Android; the macOS executable was built by the release-binaries workflow. The 32-bit Linux build (`lEdax-x86`) links libatomic statically for the atomic operations of the parallel book code, so it needs an i486 or later CPU.

The upstream 32-bit macOS `mEdax-x86` is deliberately omitted. Current Xcode SDKs lack the i386 libraries needed to link a corrected binary; including the upstream executable would leave this fork's fixes absent from that file.

Work on this fork used **ChatGPT-6 Astra** and **ChatGPT-6 Sol**; the performance work and bug fixes of v4.5.5-nikque.3 to v4.5.5-nikque.5 used **Claude Opus 5.5** (Claude Code). Edax and its original authors retain their respective attribution. This fork is distributed under the original GPL-3.0 license; keep the source and license available when redistributing the executable.
