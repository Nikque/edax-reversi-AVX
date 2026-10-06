# Edax 4.5.5: release notes

[日本語](RELEASE-NOTES.ja.md)

## v4.5.5-nikque.11

Makes the commands that cut a book down, `book subtree` and `book prune`, and `book correct` and `book enhance`, faster. No new feature or setting. `eval.dat`, the book file format and the search code are unchanged. See the [English](README-NIKQUE.en.md) and [Japanese](README-NIKQUE.ja.md) READMEs for details.

Speed (same book):

- `book subtree`, `book prune`: while marking the positions to keep, a position was walked again once for every line of play leading to it (since upstream). The repeated walks are gone, and the negamax before the cut and the removal of the links to the removed positions use several threads. On a book of 6.49 million positions with 32 threads, reducing the depth by one (`book depth 19`, then `book subtree` at the initial position) went from 120.4 s to 5.5 s, and `book prune` from 6.3 s to 1.6 s (builds without PGO, one run each). The saved books and the peak memory are the same.
- On the real book of 661.62 million positions, reducing the depth from 40 to 39 (`book depth 39`, `book subtree`) took 603.6 s from loading to the end, with a peak memory of 31.8 GB (one run; not timed with the former version; with the upstream version it took a very long time).
- After the cut, the memory of the removed positions is given back (Windows builds only): 31.8 GB → 24.3 GB in the case above.
- `book enhance`: the negamax at the start and after each round uses several threads.

What behaves differently:

- `book subtree` from another position than the initial one, with a depth reduced by `book depth`: the negamax is run from the position the book was cut from. So far the negamax did nothing (since upstream), and the counts of wins, draws, losses and lines and the score bounds were those of the former book. Moves and scores do not change. Only the display of `book show` uses these values.
- `book correct`: with `book-expand-tasks` 2 or more, or `auto`, several solved positions are searched at the same time (as many as `book deviate` expands). A small book with 120 solved positions, 32 threads: 2.0 s → 0.47 s. It uses more memory, for the searches run at the same time. When several moves have the same score, the choice of the leaf move can vary from a run to the next, as before. Unchanged with `book-expand-tasks = 1` or `-n 1`.
- Display: the N of `Book subtree N... done` and `Book prune N... done` no longer counts repeated walks, so it is smaller. The progress line `Book prune N to keep` is printed once per 100,000 positions.
- `book check`: when no move of the games is in the book, the percentage of bad moves is shown as 0% (it was 0/0).

Checks: book regression (all files identical to the release build of v4.5.5-nikque.10), comparison of the saved books for each change, the 193 checks of the API test, results and node counts of single-thread `-solve`. The release builds were not compared on an idle machine, since only book commands changed.

## v4.5.5-nikque.10

Fixes three bugs found after v4.5.5-nikque.9, and makes `book negamax`, `book fix` and `book merge` on large books, and the expansion of `book deviate`, faster. No new feature or setting. `eval.dat`, the book file format, and the results and node counts of single-thread searches are unchanged. See the [English](README-NIKQUE.en.md) and [Japanese](README-NIKQUE.ja.md) READMEs for details.

Bug fixes:

- A game of more than 80 plies (moves + passes) wrote outside the game record (from upstream; games with many passes from a board set with `setboard`; the move counter and the clocks were damaged). The record now holds 128 entries. libedax had the same problem.
- The previous string was not released when a string setting (`book-file`, ...) was set again (from upstream).
- libedax: `edax_stop` during `edax_bench` now ends the bench (only one problem was cut, the others went on, and the time of the result was wrong).

Speed (same results):

- `book negamax` with several threads and the linking step of `book fix`, `book link` and `book merge` ask for the memory of all the positions that the moves of a position lead to before reading them. The real book of 661.62 million positions, 32 threads: one negamax 18.9 s → 17.8 s, the linking of `book fix` 153.7 s → 136.3 s, the linking of `book merge` 139.6 s → 127.2 s. The saved books are the same, and so is the peak memory.

What behaves differently:

- `book-expand-tasks = auto` (the value of the bundled `config.ini`): up to level 18, a round with at least 32 times `n-tasks` positions to expand runs `n-tasks` searches of one thread (it was half as many searches of 2 threads). The book of 6.49 million positions, 32 threads: about 1.25 times as many positions expanded in 60 s, peak memory 1,432 MB → 1,001 MB. On the real book of 661.62 million positions with `book deviate2 5 5`, the nodes for each position went down by about 6.5% (the gain in time is expected to be 6 to 7%; what was verified is the node count). **The resulting book differs a little from the one of `auto` in v4.5.5-nikque.9** (the leaf move or the value of 0.065% of the positions after a round; almost all the values differ by 1). Runs of this version made one after the other (2 runs, then 3 runs) gave the same book each time (it is not verified that they always will). Rounds with fewer positions, levels above 18 and a number instead of `auto` are unchanged. The previous rule is `book-expand-tasks = 16` (half of `n-tasks`).

In the comparison of the release builds (Windows v4, 14 conditions), no condition used more memory than v4.5.5-nikque.9. For the time, only the `-solve` of 0.36 s with 32 threads came out slower beyond the error (1.012 ± 0.005); a `-solve` with 32 threads that takes 23 s shows no difference (1.002 ± 0.006), and no other condition is slower (see the table of the README).

Checked: single-thread `-solve` results and node counts; the book regression tests (every file the same); all the tests of v4.5.5-nikque.9 run again (identical books with test builds that fix the searches to one thread, repeated "stop and continue" searches, test builds short of threads and memory, the 32-bit build, ThreadSanitizer); 193 API checks; `book fix` at levels 31 to 36.

## v4.5.5-nikque.9

Fixes the bugs found by a final audit of v4.5.5-nikque.8 and edax_runner v5.3.0-nikque.2, and makes the learning of games a little faster. No new feature (two functions added to libedax, and one setting). `eval.dat`, the book file format, and the results and node counts of single-thread searches are unchanged. See the [English](README-NIKQUE.en.md) and [Japanese](README-NIKQUE.ja.md) READMEs for details.

Search:

- Two races of the parallel search, inherited from upstream (v4.5.5), are fixed. A stopped search could go on and a helper thread could end without searching its move, so nodes were stored in the transposition table with a move left unsearched (1,294 nodes in 4,000 solved endgame positions with 32 threads; 0 now). A request to stop a search could also be lost. A wrong final result is rare, but the "stop, add threads and go on" step of v4.5.5-nikque.8 was more exposed: a leaf of the book could get a move that is not the best one. Single-thread searches are unchanged. Multi-thread searches took 0.98 to 1.01 times the time of v4.5.5-nikque.8 (builds made the same way).

Book:

- A second `book merge` in the same run could fail with "duplicated position" and remove the positions added by the previous merge (since v4.5.5-nikque.3).
- `book negamax` with several threads crashed on a damaged book (a link leading back to its own position or above) (since v4.5.5-nikque.3).
- A failed `book import` lost the current book, and the book file was overwritten on exit (from upstream). The current book is kept when the file cannot be opened or holds no position; lines that cannot be read are skipped.
- A book file that could not be read at startup (damaged, or open in another program) is kept as `<name>.damaged` when the book is saved under that name (it used to be overwritten after learning).
- `base complete` and `base correct` no longer delete a file that could not be loaded. `book new` with a level out of range, a book file name that is too long, and a damaged book with positions above level 60 no longer crash or hang.
- The book is saved on exit also when `book store` only added links. The search log is no longer cut by the searches done at the same time.

Learning (`book learn`, `edax_book_store_games`):

- Lines longer than 255 bytes (many spaces between the moves) were cut, and a shorter game could be learned (since v4.5.5-nikque.7).
- With a time per game, the games played at the same time did not use up their time (since v4.5.5-nikque.7).
- No upper limit (it was 127) for the randomness of a line, as for the `book-randomness` setting.
- **A little faster**: the threads of the games that ended early are given to the searches of the games still played, from their next move on (only for games played with 7 threads or fewer). 128 games at level 18 (32 threads): 33.6 s → 30.3 s (0.90); 30 games: 0.94; a few games at level 21 or 24: same time (0.98 to 1.00). Same peak memory. The last games of a group are played by searches with several threads, so their moves can change from a run to the next (one game = 7 positions on a book of 270,000 positions). `book-store-tasks = 1` is unchanged.

When memory or threads are missing:

- The program terminated when the memory for the searches done at the same time could not be allocated (32-bit builds, ...): it now goes on with fewer searches. It no longer hangs when a thread cannot be created (it goes on with the threads that were created). 32-bit builds: an out-of-bounds write when loading a book of 89.47 million positions or more is fixed, and 128 MB are kept free when the searches done at the same time are created.

Settings and commands:

- A value that is not a number, or not on/off, is ignored with a warning. Lowering `n-tasks` keeps `book-store-tasks` and `book-expand-tasks`. A warning is shown when Edax is started through the PATH and no `config.ini` is found.
- With commands from a file (`edax < file`), `quit` is run in its turn.
- Windows: a `book.dat.tmp.<number>` left by a killed run is removed at the next save.
- `-cpu` on Linux: the threads of the book commands are bound to one CPU each; the searches done at the same time are not used with `-cpu`.
- New setting `book-store-auto-save` (default `on`, as before): with `off`, `book store` and `book learn` do not save the book to `<book-file>.store`. For programs that save the book themselves after each learning (edax_runner sets it to `off`).

Speed and memory (32 threads; builds made the same way, without PGO):

- Against v4.5.5-nikque.8, the time ratio is 0.95 to 1.005 for `book learn` (levels 18, 21 and 24), `book store`, `book add`, `book fix` and `book deviate`, within the measurement error, with the same peak memory (about 150 MB less for `book deviate`). `book merge` of a book of 6.49 million positions is 0.5 to 0.75% slower (4.86 s, ratio 1.0075 ± 0.0028; it comes from the fixes of the parallel code and of the book; same peak memory).
- The real book of 657 million positions (28.95 GB): load + save 36 s (the saved file equals the original), `book negamax` 19 s, `book merge` of another book of that size 255 s with a peak of 32.9 GB, `book fix` 239 s; same times and memory as v4.5.5-nikque.8, and identical books for load/save, negamax and merge. The book saved after `book fix` differs by the leaf move of 1 or 2 positions between two runs of the same version (same scores: searches with several threads), and by 1 position between the two versions.
- No condition slower than v4.5.5-nikque.8 with the 32-bit build, `n-tasks` 4 to 16 or `book-store-tasks` 2 to 16 (the 32-bit build uses about 110 to 120 MB more at its peak in the commands that use searches done at the same time). The hash tables of these searches keep their size: smaller ones are slower (about 2% for 1 bit less).
- Between the release builds (with PGO), no condition is slower either (`-solve` 1.000, 128 games learned at level 18: 0.90, `book merge` of the 6.49 million position book: 1.001 ± 0.003). The Linux executables have the same speed. Learning one game at level 30 takes 0.99 ± 0.05 times the time.

libedax:

- New function `edax_book_save_checked` (`book save` that returns 1 if the book was saved, 0 otherwise) and `edax_book_failed` (1 if the last book function could not add a position, the memory being exhausted). New status character `'2'` of `edax_book_store_games` (failure: the line is still to be learned).
- While a function that changes the book is running, `edax_stop` no longer stops the search (the partial result of the interrupted search went into the book).
- Fixed: a hang when a book function is called while pondering, crashes on NULL arguments, a negative number for `edax_hint`, the `edax_get_bookmove` functions when there is no move. The `link` of a `LibedaxPosition` stays valid for the next 63 positions.
- The Linux and Android libraries are linked with `-Bsymbolic` (functions of the same name in the program do not take the place of the ones of the library). The Linux library is also compiled with `-fno-semantic-interposition`, and is about 8% faster (`edax_bench`: 0.926 times the time, same node counts).

Checked: single-thread `-solve` results and node counts; the book regression tests (every file as v4.5.5-nikque.8); identical books with the test build where every search keeps one thread; repeated runs of the "stop and go on" step; test builds without enough threads or memory; 32-bit builds; ThreadSanitizer; 191 API checks.

## v4.5.5-nikque.8

Fixes `book-store-tasks = auto`, the default since v4.5.5-nikque.7, which was slower than v4.5.5-nikque.6 when few searches were needed or at a high level. This is the only change: `eval.dat`, the book file format, the search results and `book-store-tasks = 1` are unchanged, and no book was damaged. See the [English](README-NIKQUE.en.md) and [Japanese](README-NIKQUE.ja.md) READMEs for details.

- The problem: the searches of the learning commands (`book store`, `book add`, `book learn`) and of the link rebuild (`book fix`, `book merge`, `book import`, ...) ran at the same time with one thread each, so with few searches, or with a long search among short ones, the other threads waited until the longest search ended. Measured (32 threads, `1` against `auto` of v4.5.5-nikque.7): playing a game then `book store` at level 24 took 13.4 s against 26.0 s, at level 21 4.2 s against 6.4 s; `book fix` with one leaf at level 24 took 1.1 s against 6.2 s. `book merge` of 6.49 million positions was about 0.4 s slower, with a peak memory of 0.96 GiB instead of 0.55.
- The fix: with fewer searches than threads, each search starts with several threads; when no search is left to start, the searches still running are stopped and continued with the threads of the ones that ended (same hash tables). A single search is the search of `1`. The searches of the pool are only created when needed.
- Measured after the fix (`1` against `auto` of v4.5.5-nikque.8): a game then `book store` at level 24: 13.4 s against 5.8 and 10.6 s; at level 21: 4.2 s against 2.0 s; 30 games stored one by one at level 18: 34.5 s against 18.2 s (29.0 s with v4.5.5-nikque.7); `book fix` with 16 leaves at level 24: 7.8 s against 3.2 s; with 1,000 leaves at level 18: 54 to 59 s against 9.2 to 9.7 s (about 18 s with v4.5.5-nikque.7); 128 games learned by edax_runner: 37.2 and 38.8 s before, 35.1 and 35.9 s now; `book merge` of 6.49 million positions: the time of v4.5.5-nikque.6 and nearly its memory (0.57 GiB). `auto` was as fast as `1` or faster in everything that was measured.
- A search that gets more threads is a search with several threads: its result can slightly change from a run to the next (two runs of the learning of 128 games: another leaf move in 7 of 272,576 positions, same scores). `auto` uses more memory than `1` (with 32 threads: up to about 450 MB up to level 18, 0.9 GB up to level 21, 1.8 GB above).
- Checked: `-solve` results and node counts; the book regression tests (every file as v4.5.5-nikque.6 with `-book-store-tasks 1`, and also with the default in these tests); with a test build where every search keeps one thread, the same books as the searches done one after the other; 300 runs for the searches continued with more threads.

## v4.5.5-nikque.7

A library (libedax), learning games with several threads (new setting `book-store-tasks`; its default, `auto`, learns `n-tasks` games at the same time), a faster `book fix`, and two book bug fixes. `eval.dat`, the book file format and the search results are unchanged. **The books learned from games slightly differ from those of v4.5.5-nikque.6** (with `book-store-tasks = 1`, the learning and its books are the same as with v4.5.5-nikque.6). See the [English](README-NIKQUE.en.md) and [Japanese](README-NIKQUE.ja.md) READMEs for details.

libedax (Edax as a library for other programs):

- The same 93 functions and the same data layout as libedax by lavox and sensuikan1973: programs written for libedax (libedax4dart, edax_runner, ...) work by replacing the library file.
- `libedax-x64.dll` (any x86-64 CPU), `libedax-x64-v3.dll` (AVX2), `libedax-x64-v4.dll` (AVX-512); `libedax-x86-64.so`, ... on Linux; `libedax.universal.dylib` on macOS (both Apple silicon and Intel).
- Settings are read from `edax.ini` and `config.ini` of the current folder, then from the arguments of the initialization.
- New functions: `edax_book_deviate2`, `edax_book_deviate3`, `libedax_cpu_level`, `edax_book_store_games` (play and learn several games together), `edax_book_store_tasks`.
- Android libraries (`libedax-arm64-v8a.so`, `libedax-armeabi-v7a.so`) are also built (build checked only).
- The edax program is not affected (an edax built from the sources with only libedax added is byte-identical to the one of v4.5.5-nikque.6). The library and the edax program search the same nodes at the same speed. Compared with the original libedax (Edax 4.4), a single-thread endgame search is 1.3 to 1.7 times faster, with about half the peak memory.

Learning games with several threads (new setting `book-store-tasks`):

- `book-store-tasks = auto` (the default: in the bundled `config.ini`, and without `config.ini`) learns `n-tasks` games at the same time (one thread per game); a number of 2 or more, that many games. `1` works as up to v4.5.5-nikque.6, with the same book as a result. This applies to `book store`, `book add`, the new command `book learn <file>` (for each line: play the moves, let Edax play both sides to the end, then `book store`; the "edax vs edax" of edax_runner) and `edax_book_store_games` of libedax.
- How: the positions to add and the moves excluded from their searches are found first; all the searches are done at the same time as one-thread searches; then the positions are added to the book in the usual order. `book learn` also plays the games at the same time, and the book is linked, negamaxed and saved once per group of games.
- Measured (32 logical CPUs, a book of 270,000 positions at level 18, 128 games learned by edax_runner): 112.9 s with `1` (133.5 s with `n-tasks` 8), 39.4 to 41.6 s with `auto` (2.8 to 3.3 times faster). Peak memory: 290 MB to 722 MB. This is about the speed of several edax_runner at the same time (41.1 s with 8 of them and 4 threads each, 35.2 s with 16 x 2 threads, without the time to merge their books), with one process and one book. `book store` of a single game, and small values such as 2 or 4, gain little (105.3 s with 2, 76.4 s with 4).
- The book learned with `auto` is not the same as with `1` (for these 128 games, compared with the book of one thread and `1`: 372 positions with different contents out of about 272,600, and 5 and 8 positions in one book only; this is no more than the difference made by 8 threads in the original learning (381, and 192 and 196). `auto` gives the same book every time). This is why `auto` is the default. The expansion commands (`book deviate`, ..., which use `book-expand-tasks`) and `book fill` are unchanged.

Faster `book fix`:

- The checking (Fixing), linking and sorting steps of `book fix` (also used by `book import`, `correct`, `prune`, `subtree`, and after `book store`) use all the threads. The book is the same as with one position after the other. `book fix` of a book of 6.49 million positions: 22.9 s to 2.3 s with 32 threads (25.4 s with upstream v4.5.5), unchanged with one thread (24.7 s); same peak memory (407 MB).
- The leaf searches needed while the links are rebuilt (`book fix`, `book merge`, `book import`, ...) are done at the same time, one thread each, when `book-store-tasks` is not 1 (as the games are learned; the book is the same as with these searches done one after the other with empty hash tables). `book fix` of a level 18 book of 270,000 positions with 1,000 leaves to search again (32 threads): 56.5 to 58.3 s with `1`, 18.2 to 18.5 s with `auto` (peak memory 288 MB to 730 MB). A usual `book merge` reuses the leaves of the merged book and has almost no search to do (1 search when merging two real books), so it gains almost nothing. `book merge` rebuilds its links as in v4.5.5-nikque.6.
- The hash tables of each of these one-thread searches take 14 MB up to level 18, 28 MB up to level 21 and 57 MB above, whatever `hash-table-size` is.

Executables:

- macOS: `mEdax-arm64` for Apple silicon, and the library `libedax.universal.dylib` (built by the release-binaries workflow).

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
