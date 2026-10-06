# Edax 4.5.5 corrected build

[日本語](README-NIKQUE.ja.md) · [Releases](https://github.com/Nikque/edax-reversi-AVX/releases) · [Change list](RELEASE-NOTES.md)

This public fork is based on upstream `v4.5.5` (`4cde6ff588f0eade07fcba0c7f02d5cd0cacd4ee`). It publishes the modified source, rebuilt Windows, Linux, macOS, and Android executables, the original GPL-3.0 [license](LICENSE), and the changes described below. The upstream `master` branch remains available; `edax-4.5.5-fixes` is this fork's default branch.

## Changes in v4.5.5-nikque.10

This version fixes three bugs found after v4.5.5-nikque.9, and makes `book negamax`, `book fix` and `book merge` on large books, and the expansion of `book deviate`, `deviate2` and `deviate3`, faster. There is no new feature or setting. The evaluation data `eval.dat`, the book file format, and the results and node counts of single-thread searches are unchanged. Two things behave differently (see "What behaves differently" below): the bundled setting `book-expand-tasks = auto` in the rounds with many positions to expand (the resulting book differs a little), and `edax_stop` during `edax_bench` in libedax.

### Bug fixes

- **A game of more than 80 plies wrote outside the game record** (from upstream): the record had room for 80 moves and passes. From a board set with `setboard`, a game with many passes can be longer, and the move counter and the clocks that follow the record were overwritten (the example found: after a game of 56 moves and 29 passes = 85 plies, the display shows "ply 5" and a clock of "6313 days"; the program does not crash). libedax (`edax_play`, `edax_move`, ...) had the same problem. The record now holds 128 entries (from any board there are at most 62 moves, and a pass is only recorded before a move: 124 at most). The same kind of write in the `force` line and in the saving of a game (60 moves) is fixed too. Games of 80 plies or less are unchanged. `edax_get_moves` still returns 80 plies at most (its buffer is documented as 161 characters).
- **The previous string was not released when a string setting was set again** (from upstream): `book-file`, `eval-file`, `game-file`, `name`, the log file names, and so on. Repeated calls of `edax_set_option` in libedax left a few dozen bytes each time.
- **libedax: `edax_stop` during `edax_bench` did not end the bench** (from the original libedax): `edax_stop` only cut the problem being solved, and the bench went on with the other problems. The time of the cut search was added as a clock value, so the result showed a time of several days. `edax_stop` now ends the bench. The problem that was cut is not counted in the positions, the nodes or the time.

### Faster book functions (same results)

`book negamax` (with several threads) and the "linking" step of `book fix`, `book link` and `book merge` looked in the book, for each position, for the positions its moves lead to, one after the other. On a large book, each of these lookups waits for memory. Now the places of all of them are computed first and asked for (prefetch), and then they are looked for together. The same positions are looked for in the same order, so the results do not change.

The real book of 661.62 million positions (level 18, 32 threads, builds without PGO, two runs each in the order previous, new, new, previous):

| Step | Previous | This version | |
|---|---|---|---|
| One `book negamax` | 18.8-19.0 s | 17.6-18.0 s | about 6% shorter (all 4 runs) |
| Linking of `book fix` | 153.7 s | 136.3 s | about 11% shorter |
| Whole `book fix` (load to save) | 240.8 s, 247.9 s | 222.0 s (242.0 s in the other run, whose load took 7 s more) | |
| Linking of `book merge` (merging a book of 6.49 million positions) | 139.6 s | 127.2 s | about 9% shorter |
| Whole `book merge` | 235.3 s, 239.8 s | 223.6 s, 217.6 s | about 7% shorter |

The saved books had the same content in all 12 runs, and the peak memory was the same (31.8 GB). Neither `book fix` nor `book merge` had anything to change in this book, so "the same result when the book changes" is checked by the 6.49 million position book below and by the regression tests.

The book of 6.49 million positions (previous and new in turn, 10 to 12 pairs, while other programs were running; time of the whole command):

| Command | Threads | Previous | This version | Pairs where this version was faster |
|---|---|---|---|---|
| 20 `book negamax` | 32 | 4.36 s | 4.16 s | 11 of 12 |
| | 8 | 6.99 s | 6.09 s | 12 of 12 |
| | 2 | 21.16 s | 17.04 s | 6 of 6 |
| `book fix` | 32 | 2.54 s | 2.38 s | 10 of 10 |
| | 8 | 3.87 s | 2.54 s | 10 of 10 |
| `book merge` (6.49 million positions into an empty book) | 32 | 5.53 s | 5.32 s | 10 of 10 |
| | 8 | 7.14 s | 5.63 s | 10 of 10 |

The books after `book fix`, after `book merge` and after `book negamax` (32 and 3 threads) had the same content with the previous code and with this version. `book negamax` with one thread (`-n 1`) is another function and is unchanged.

### book-expand-tasks = auto: one-thread searches, as many as threads, in the rounds with many positions

With `book-expand-tasks = auto` (the bundled `config.ini`), up to level 18, `n-tasks / 2` searches of 2 threads ran at the same time. In this version, up to level 18, a round with **at least 32 times `n-tasks`** positions to expand (1,024 with 32 threads) runs `n-tasks` searches of one thread. Rounds with fewer positions, levels above 18, and `book-expand-tasks` given as a number are as in v4.5.5-nikque.9. The hash tables of the one-thread searches have the size of the one-thread searches of the learning of games (19 bits = 14 MB up to level 18).

A round with many positions (the book of 6.49 million positions, level 18, `book deviate3 2 6`: 22,750 positions to expand; positions expanded in 60 s; builds without PGO):

| `n-tasks` | Rule of v4.5.5-nikque.9 | This version | Peak memory |
|---|---|---|---|
| 32 | 8,270, 8,086 | 10,247, 10,195 (about 1.25 times) | 1,432 MB → 1,001 MB |
| 8 | 2,628 | 3,988 (1.52 times) | 663 MB → 556 MB |

With the same book, the first round (22,750 positions) was done and saved after 154 s and 156 s before, 122 s and 123 s now (two runs each while no other program was running; load and selection included).

**On the real book (661.62 million positions) with `book deviate2 5 5` (2,753,399 positions to expand in a round), the difference is smaller.** The positions expanded there need heavier searches (about 33 million nodes for each position), and a 2-thread search wastes little. Over the first 420 s, where the same positions are expanded in the same order, the nodes for each position went from 3.49×10^7 to 3.26-3.29×10^7 (about 6.5% fewer; 4 runs of this version), and the speed of all the threads together was about the same. The two pairs measured by time gave 35.1 → 37.4 and 34.2 → 35.5 positions per second, but the same build varied from 32 to 40 positions per second from one hour to the next, so **the gain in time is "6 to 7% expected"; what was verified is the difference in nodes.**

A condition with rounds of few positions (the book of 270 thousand positions, `book deviate 1 2`: rounds of 0 to a few hundred positions, total of 8 runs each) took 11.45 ms → 11.10 ms for each position: no change (no round reaches 1,024 positions there, so the code path is the one of v4.5.5-nikque.9). Trials with a lower limit (one thread each from 1 and from 4 times `n-tasks`) took 19% and 9% more time for each position in this condition, hence 32 times.

**The resulting book is not the same as with `auto` in v4.5.5-nikque.9** (the number of positions expanded at the same time changes, as when the number of `book-expand-tasks` is changed). With the book of 6.49 million positions, the books after the first round (22,750 positions expanded, 22,671 positions added) were compared position by position:

| Books compared | Positions that differ |
|---|---|
| Two runs of the rule of v4.5.5-nikque.9 (2 pairs) | 464, 885 |
| Runs of this version (2 runs, then 3 runs) | 0 |
| Rule of v4.5.5-nikque.9 against this version | 4,231 to 4,251 (0.065% of the 6,513,834 positions) |

- The added positions and the moves of the links were the same in every pair. Of the 4,231 positions that differ between the rule of v4.5.5-nikque.9 and this version, 1,377 have another leaf move (the best move that is not a link yet) and 1,626 another value (by 1 for 1,420, by 2 for 167, by 3 to 6 for 39).
- The runs of this version all gave the same book: a one-thread search of a given position always gives the same result. **It is not verified that they always will** (the order of the expansions running at the same time can matter, for example when two expansions of a round reach the same position). With the rule of v4.5.5-nikque.9 (2-thread searches), a few hundred positions differ from one run to the next with the same settings.
- As before, the positions expanded at the same time do not see each other (32 of them now, instead of 16).

To get the previous rule back, write `book-expand-tasks = 16` (half of `n-tasks`).

### Speed and memory (release builds)

`wEdax-x86-64-v4.exe` for Windows, the release of v4.5.5-nikque.9 against this version (PGO build made the same way). Ryzen 9 9950X, 32 threads (1 thread on the first line), measured only while no other program was running, 8 to 32 runs. The ratio is this version ÷ v4.5.5-nikque.9, ± is the standard error.

| Condition | v4.5.5-nikque.9 | This version | Ratio | Peak memory |
|---|---|---|---|---|
| `-solve` (fforum-20-39), 1 thread | 1.207 s | 1.199 s | 0.994 ± 0.005 | same |
| the same, 32 threads | 0.362 s | 0.359 s | 0.992 ± 0.008 | same |
| 30 midgame positions, level 18 | 0.954 s | 0.954 s | 1.000 ± 0.005 | same |
| 30 midgame positions, level 21 | 4.031 s | 3.990 s | 0.990 ± 0.008 | same |
| 20 `book negamax` (6.49 million positions) | 3.986 s | 3.739 s | 0.938 ± 0.004 | same |
| `book fix` (6.49 million positions) | 2.223 s | 2.180 s | 0.981 ± 0.004 | same |
| `book merge` (6.49 million positions) | 4.966 s | 4.887 s | 0.984 ± 0.011 | same |
| `book fix`, 1000 leaves (level 18) | 9.169 s | 9.176 s | 1.001 ± 0.015 | same |
| `book deviate 0 2` (270 thousand positions, `auto`, rounds of 1 to 7 positions, 32 runs) | 8.386 s | 8.329 s | 0.993 ± 0.015 | same |
| `book learn`, level 21, 8 games | 10.265 s | 10.315 s | 1.004 ± 0.014 | same |
| `book learn`, level 24, 4 games | 10.182 s | 10.240 s | 1.006 ± 0.009 | same |
| 30 games stored one by one with `book store` (level 18) | 19.854 s | 19.948 s | 1.005 ± 0.008 | same |
| `book learn`, level 18, 128 games | 30.438 s | 30.312 s | 0.996 ± 0.003 | same |

No condition is slower beyond the error, and the peak memory is the same in every condition.

**Not measured**: `book deviate` in rounds with many positions with the release (PGO) builds (the comparison of `auto` above is between builds of the same source without PGO), the book of 657 million positions with the release builds, the speed of the 32-bit, Linux, macOS and ARM64 builds, `book deviate` above level 18 (its rule is unchanged).

### What behaves differently (summary)

| Case | Up to v4.5.5-nikque.9 | v4.5.5-nikque.10 |
|---|---|---|
| `book-expand-tasks = auto`, up to level 18, a round with at least 32 times `n-tasks` positions | `n-tasks / 2` searches of 2 threads | `n-tasks` searches of one thread (the book differs a little: see the table above) |
| A game of more than 80 plies (moves + passes) | writes outside the record (the move counter and the clocks are damaged) | recorded correctly (up to 124 plies are possible) |
| libedax: `edax_stop` during `edax_bench` | cuts one problem, the others go on; the time is wrong | ends the bench |

### Checks

- `-solve`: single-thread results and node counts are the same as v4.5.5-nikque.9 (the 5 Windows release executables, the 4 Linux ones, a 32-bit test build).
- Book regression tests (every book command; 1 and 8 threads, including the book of 6.49 million positions): every file is the same as with the code of v4.5.5-nikque.8 (only the files that hold a date differ).
- The tests of v4.5.5-nikque.9 were all run again on the code of this version: identical books with test builds that fix the searches to one thread, 240 runs of `book fix` with the "stop and continue" searches, repeated multi-thread `-solve` (8,000 positions on Windows, 3,000 on Linux, no wrong result), test builds where threads cannot be created (Windows and Linux), running out of memory in the 32-bit build, 94 cases of damaged books and 23 cases of settings, ThreadSanitizer on 8 cases (no new kind of report for the code added here), the Android build.
- libedax: 193 API checks (2 were added: `edax_stop` from another thread during `edax_bench`; 3 Windows and 3 Linux libraries), 81 cases of edge values and wrong calls, the tests of libedax4dart 7.67.0 (28 of 29; the remaining one is the same since v4.5.5-nikque.7).
- `book fix` (2 leaves) at levels 31 to 36 and the learning of one game at levels 31 and 32 end normally (v4.5.5-nikque.9 was checked up to level 30; one game took 104 s at level 31 and 188 s at level 32, with a peak of 2.15 GB; build without PGO, one run while other programs were running).

## Changes in v4.5.5-nikque.9

This version fixes the bugs found by a final audit of v4.5.5-nikque.8 and edax_runner v5.3.0-nikque.2, and makes the learning of games (`book learn`) a little faster. There is no new feature (two functions were added to libedax, and one setting). The evaluation data `eval.dat`, the book file format, and the results and node counts of single-thread searches are unchanged. What behaves differently is listed under "What behaves differently" below.

### Multi-thread search

Two races of the parallel search, both inherited from upstream (v4.5.5), are fixed.

- **A search that had been stopped could go on, and a helper thread could end without searching its move.** Nodes were then stored in the transposition table as "all moves searched" although a move had not been searched. In ordinary 32-thread searches: 1,294 such nodes in 4,000 solved endgame positions and 8,453 in 360 midgame positions at level 21 (0 in both after the fix). A wrong final result is rare (0 in 28,000 solved endgame positions with the old code).
- **A request to stop a search could be lost** (3 out of 2.4 million requests). The "stop, add threads and go on" step of v4.5.5-nikque.8 (`book-store-tasks`) searches again with the transposition table of the stopped search, so it was more exposed to both races: a leaf of the book could get a move that is not the best one, with its score (in a test that sends stop requests all the time: 2 wrong results in 12,800 positions before the fix, 0 in 13,920 after).
- Single-thread searches are unchanged (same results and node counts as v4.5.5-nikque.6 and v4.5.5-nikque.8). For multi-thread searches, with builds made the same way (without PGO), the time relative to v4.5.5-nikque.8 was 0.98 to 1.01 (solved endgames and midgame at levels 18 to 24, with 1, 2, 8 and 32 threads: 9 conditions, standard error 0.003 to 0.02): no condition can be said to be slower. The speed and the memory of the book commands are under "Speed and memory (measured)" below.

### Book bugs

- **A second `book merge` in the same run could fail** (since v4.5.5-nikque.3): after merging a book that holds positions unreachable from the root (for example games stored from a non-standard starting position), the next `book merge` failed with "duplicated position", and undoing that failed merge also removed the positions added by the previous merge from the book in memory.
- **`book negamax` with several threads crashed on a damaged book** (since v4.5.5-nikque.3): with a link that leads back to its own position or to a position above, the stack was exhausted. The negamax with threads now follows only the links that add a disc and the passes (all the links of a correct book).
- **A failed `book import` lost the current book** (from upstream; also `edax_book_import` of libedax): with a file that cannot be opened, the book was replaced by a new book of one position, which was then saved over the book file on exit. The current book is now kept when the file cannot be opened or holds no position. Lines that cannot be read are skipped (the reason is shown for the first 10).
- **A book file that could not be read at startup was overwritten after learning**: a book file that is damaged, or that could not be read because another program had it open, is now renamed to `<name>.damaged` (`.damaged.1`, ... if it exists) before the book is saved under that name. A book that could not be read only because another program had it open is no longer taken for a missing file.
- **`base complete` and `base correct` deleted a file that could not be loaded** (from upstream; also in libedax): the file is now deleted and saved only when it was loaded.
- **`book new` with a level out of range** (not 0 to 60) read outside a table and could never return. It now warns and keeps the current book.
- **A book file name that is too long** (more than `FILENAME_MAX - 8` characters: 252 on Windows) crashed the program. It is now ignored with a warning (the previous or the default name is kept).
- **A book with positions at a level above 60 (a damaged file)**: `book info` wrote outside an array. `book fix` now rebuilds such positions. The level in the header of a book file is clamped to 0 to 60 (with a warning).
- When `book store` only added links, the book was not saved to the book file on exit.
- The search log (`-search-log-file`) was closed when the searches done at the same time were released.

### Learning (`book learn`, `edax_book_store_games` of libedax)

- **Lines longer than 255 bytes were cut** (since v4.5.5-nikque.7): for a line with many spaces between its moves, when the cut fell between two moves, the shorter game was "learned". The spaces are now removed first (a line of 128 moves or more is not a game and is not learned).
- **With a time per game, the games played at the same time did not use up their time** (since v4.5.5-nikque.7): every move was searched as if the whole time was left (8 games at 10 seconds per side: 52.1 s → 19.5 s). Learning at a fixed level is unchanged.
- **No upper limit for the randomness of a line (`<number>,<moves>`)**: it was limited to 127, and a line with 128 or more was "not a game". Any number is now accepted, as for the `book-randomness` setting (scores differ by 128 at most, so 128 or more always means "any move of the book").

### When memory or threads are missing

- **The program terminated when the memory for the searches done at the same time could not be allocated** (32-bit builds with `-h 25`, for example); what had been learned and not yet saved was lost. It now goes on with fewer searches at the same time (one after the other if none could be made). Warnings: `not enough memory to search the positions at the same time`, `not enough memory to play N games at the same time`.
- **When a thread could not be created**, a search or a book command could hang for ever (the upstream search, and the book commands of v4.5.5-nikque.3 to nikque.7). It now goes on with the threads that were created (warning `cannot create a thread: the search uses N threads instead of M`).
- 32-bit builds: 128 MB are kept free when the searches done at the same time are created. Loading a book of 89.47 million positions or more overflowed the computation of the size to allocate and wrote outside the block (now an error, and the current book is kept: a 32-bit build cannot load such a book).

### Settings and commands

- In `config.ini`, on the command line and in commands, **a value that is not a number, or not on/off** (`level = abc`, `n-tasks = 4x`, ...) is ignored with a warning (it was ignored silently or taken up to the first wrong character).
- Lowering `n-tasks` while running no longer lowers `book-store-tasks` and `book-expand-tasks` (they were cut, and stayed cut when `n-tasks` was raised again).
- When Edax is started by its name alone through the PATH and the working folder has no `config.ini`, a warning is shown (the defaults are used).
- **With commands from a file (`edax < file`), `quit` is run in its turn** (the commands before `quit` could be dropped). Nothing changes for a terminal or a pipe.
- Windows: a `book.dat.tmp.<number>` left by a killed run is removed at the next save.
- `cores` and `memory` of xboard, `depth` of NBoard: clamped to their range.
- `-cpu` on Linux: the threads of the book commands are also bound to one CPU each (they were all on CPU 0). With `-cpu`, the searches done at the same time (`book-store-tasks`, `book-expand-tasks`) are not used.
- The error messages of book load, save and merge now end with a new line.
- **New setting `book-store-auto-save`** (`on`/`off`, default `on` = as before): with `off`, `book store` and `book learn` (`edax_book_store` and `edax_book_store_games` of libedax) do not save the book to `<book-file>.store` afterwards. It is for a program that saves the book itself after each learning (edax_runner), where the whole book was written twice. What is learned does not change. The setting is not in the bundled `config.ini` (the default applies).

### libedax

- **New function `edax_book_save_checked`**: it saves the book as `edax_book_save` does, and returns 1 if the book was saved, 0 otherwise (`edax_book_save` returns nothing, as in the original libedax: when another program had the book file open, for example, there was only a message on stderr).
- **New function `edax_book_failed`**: it returns 1 when the last book function (`edax_book_store`, `edax_book_deviate`, `edax_book_add_board`, ...: they return nothing, as in the original libedax) could not add a position to the book because the memory was exhausted. Each book function clears this state when it starts, so it is called right after the function to check. There are now 7 added functions; the library exports 100 functions (the 93 of the original libedax and these 7).
- **`edax_book_store_games`: a new status character `'2'`**: `'1'` = learned, `'0'` = not learned (illegal move, or not a game), `'2'` = failure (not enough memory to add a position to the book: the line is still to be learned).
- **While a function that changes the book is running, `edax_stop` no longer stops the search** (it only sets the mode to 3). The result of the interrupted search used to go into the book as it was (calling `edax_stop` 40 times while a game of 60 moves was stored at level 16 changed 10 positions out of 25). `edax_go`, `edax_hint`, ... are stopped as before.
- Calling a book function, `edax_bench` or a base function while pondering (`ponder on`) hung (the original libedax crashed): the pondering is now stopped first.
- NULL arguments that crashed (41 of 52 pointer arguments) now do nothing (functions with a result return -1, 0 or NULL). Also fixed: a negative number for `edax_hint`, a square outside the board for `edax_board_get_square_color`, and the `edax_get_bookmove` functions when there is no move to return (an empty list is returned).
- The `link` of a `LibedaxPosition` stays valid until the next 63 positions are fetched (it was 8).
- The Linux library is compiled with `-fno-semantic-interposition` (about 8% faster: see "Speed and memory (measured)").
- The Linux and Android libraries are linked with `-Bsymbolic`: if the program that uses the library has functions or variables with the same names as Edax (`board_init`, ...), the library still uses its own. The exported names and the API are the same. As before, the Android libraries were only built, not run on a device.
- Tests: `tests/libedax_test.c` now has 191 checks (passed on 3 Windows and 3 Linux builds).

### Learning games a little faster (the threads of the games that are over go to the games still played)

`book learn` (`edax_book_store_games` of libedax, the grouped learning of edax_runner) plays the games of a group at the same time (one thread per game with `book-store-tasks = auto`). The threads of the games that ended early used to wait until the longest game was over (in a group of 32 games most games end during the first second, and a few went on alone for 2 or 3 more seconds). Now **a thread that has no game left is given to a search that still plays, from its next move on** (between two moves: no search is stopped).

- Only a game played with 7 threads or fewer gets more threads. A game played with 8 threads or more (few games) did not get faster with more (see the note under the table): it is played exactly as before.
- Nothing changes with `book-store-tasks = 1`, or with a single game.

Measured (Ryzen 9 9950X, builds made the same way without PGO, 12 to 24 rounds; "before" is the code without this change only):

| Work | Before | This version | Ratio |
|---|---|---|---|
| Level 18: 128 games (32 threads, book of 270,000 positions) | 33.6 s | 30.3 s | **0.901 ± 0.002** |
| Level 18: 30 games | 8.64 s | 8.15 s | 0.944 ± 0.004 |
| Level 18: 30 games, `n-tasks` 8 | 21.6 s | 20.2 s | 0.935 ± 0.003 |
| Level 18: 30 games, `book-store-tasks` 8 | 9.42 s | 9.29 s | 0.986 ± 0.015 |
| Level 18: 8 games | 2.61 s | 2.59 s | 0.993 ± 0.009 |
| Level 21: 8 games | 10.33 s | 10.26 s | 0.994 ± 0.006 |
| Level 21: 8 games, `n-tasks` 4 | 40.9 s | 40.2 s | 0.981 ± 0.003 |
| Level 24: 8 games | 21.5 s | 21.4 s | 0.997 ± 0.006 |
| Level 24: 4 games / 2 games (8 and 16 threads each: no thread is added) | 10.03 s / 6.85 s | 10.05 s / 6.86 s | 1.002 ± 0.006 / 1.001 ± 0.011 |

- The peak memory is the same in every row.
- Note: a first version also gave threads to the games played with 8 threads or more; with 2 games at level 24 (16 threads each) it took 1.008 ± 0.005 times the time (72 rounds), slightly on the slower side. So only the games played with 7 threads or fewer get more threads.
- **Resulting book**: the last games of a group are now played by searches with several threads, so their moves can change from a run to the next. Learning 128 games (272,576 positions) three times: 9 to 13 positions differ between two runs before this change, 6 to 11 after it (the searches that get more threads while they run, since v4.5.5-nikque.8); between before and after, one more game was played differently (7 positions). To make the same books as the older versions, set `book-store-tasks = 1`, as before.
- Checked: with this change, the tests of the audit were run again (single-thread `-solve`, book regression, identical books with the test build where every search keeps one thread, builds where threads cannot be created, ThreadSanitizer, API checks), and 48 learning runs with different numbers of games, threads and `book-store-tasks` all ended normally, with nothing to fix in the books.

### Speed and memory (measured)

Ryzen 9 9950X (32 logical CPUs), `n-tasks` 32, `hash-table-size = auto`. The code of v4.5.5-nikque.8 and the code with the fixes of this version (without the change of "Learning games a little faster" above, whose effect is in the table above) were built with the same compiler and options (AVX-512, without PGO), and run in turn, in a changing order; the ratio is taken between the runs of the same round (± is the standard error). The release builds (with PGO) are compared under the table.

| Work | v4.5.5-nikque.8 | Rounds | This version / v4.5.5-nikque.8 (time) | Peak memory |
|---|---|---|---|---|
| Level 18: `book learn` of 128 games (book of 270,000 positions) | 32.5 s | 12 | 1.000 ± 0.004 | same (718 MB) |
| Level 21: `book learn` of 8 games | 10.0 s | 12 | 0.996 ± 0.008 | same (2.0 GB) |
| Level 24: `book learn` of 4 games | 9.6 s | 12 | 1.005 ± 0.008 | same (2.4 GB) |
| Level 24: `book learn` of 1 game (37 searches) | 5.1 to 11.7 s (mean 8.5) | 40 | 1.07 ± 0.06 | same (2.2 GB) |
| Level 21: the same | 2.0 to 4.7 s (mean 2.9) | 40 | 0.95 ± 0.04 | same (1.35 GB) |
| Level 18: 30 games, each one played then stored with `book store` | 18.3 s | 12 | 0.997 ± 0.006 | same (720 MB) |
| Level 18: `book add` of 30 games | 2.8 s | 24 | 1.00 ± 0.01 | same (720 MB) |
| Level 18: `book fix` with 1000 leaves to search again | 9.0 s | 12 | 1.003 ± 0.007 | same (701 MB) |
| Level 18: `book deviate 1 2` (book of 270,000 positions, `book-expand-tasks = auto`, about 3400 positions expanded) | 28.6 to 49.7 s (mean 35.6) | 9 | 0.95 ± 0.09 | 1,297 MB → 1,149 MB |
| Level 18: `book deviate 0 2` (the same book, about 370 positions expanded, 102 rounds) | 9.2 s | 16 | 0.918 ± 0.015 | 1,297 MB → 1,149 MB |
| `book merge` of a book of 6.49 million positions into an empty book | 4.86 s | 24 | **1.0075 ± 0.0028** | same (746-761 MB) |
| `book merge` of two real books of 270,000 positions | 0.13 s | 48 | 0.98 ± 0.01 | same (272 MB) |

- **`book merge` of the 6.49 million position book is 0.75% (about 0.04 s) slower** (2.7 times the standard error). Measured again with builds holding the fixes step by step (24 rounds): 1.008 ± 0.004 with the fixes of the parallel code and of the book, 1.005 ± 0.004 with all the fixes of this version; the difference comes from the former (the marks of all the positions checked when a merge starts, the links checked by negamax, ...). The peak memory is the same (746 to 761 MB). Between the release builds this difference does not appear (1.001 ± 0.003). For the other rows the difference is within the error, or on the faster side.
- `book deviate`: the searches of the concurrent expansion are now created with their final size at once (a part of the fix under "When memory or threads are missing"): the peak memory is about 150 MB lower, and a learning made of many rounds is a little faster. The time of a concurrent expansion varies much from a run to the next (the order of the expansions changes).
- Learning a single game (levels 21 and 24) takes a time that changes by a factor of 2 or more with the same executable. The time is the one of the last search still running (at level 24, the solving of a position with 30 empties): continued with 32 threads, it visited from 1.5 to 5.9 billion nodes depending on the run.
- Values of `book-store-tasks` (128 games at level 18, seconds / peak memory): `auto` 34.3 / 718 MB, `1` 109.0 / 281 MB, `2` 59.8 / 1155 MB, `4` 46.3 / 1155 MB, `8` 37.9 / 1587 MB, `16` 34.1 / 1587 MB. 8 games at level 21: `auto` 10.0, `1` 29.2, `2` 12.9, `4` 11.0, `8` and `16` 10.0 to 10.2. No value is slower than `1`, but with many games 2 to 8 are slower than `auto` and use more memory.
- `n-tasks` 4, 8 and 16 (ratio to the code of v4.5.5-nikque.8): 30 games learned at level 18: 0.996 and 0.996 (not enough rounds with 16); 8 games at level 21: 0.990, 0.997 and 0.985 (± 0.003 to 0.008): no difference. `auto` is still faster than `1` with 4 threads (8 games at level 21: 40.1 s against 43.2 s).
- Size of the hash tables of the searches done at the same time (19 bits up to level 18, 20 up to level 21, 21 above), changed in test builds: 128 games at level 18 take 1.017 ± 0.002 times the time with 1 bit less and 1.055 ± 0.004 with 2 bits less (peak memory 719 → 503 → 393 MB); 4 games at level 24: 1.021 ± 0.007 and 1.053 ± 0.009 (2433 → 1568 → 1136 MB); 8 games at level 21: 0.999 ± 0.011 and 1.024 ± 0.011. Larger tables only gain 1 to 2%. The sizes were kept.
- 32-bit build (without PGO, ratio to the code of v4.5.5-nikque.8): `-solve` 0.93 ± 0.02 with 1 thread, 1.005 ± 0.005 with 8 and 32 threads; 30 games learned at level 18: 0.997 ± 0.003; `book fix` with 1000 leaves: 1.015 ± 0.014; 8 games at level 21 (8 threads): 1.000 ± 0.002. **The peak memory of the commands that use searches done at the same time is about 110 to 120 MB higher** (718 → 830 MB, for example: 128 MB are allocated for a moment to check that they are free before a search is created; see "When memory or threads are missing").
- Level 30 (one run each): `book fix` with 2 leaves to search: 3.4 s with `auto`, 3.6 s with `1`; with 4 leaves: 11.9 s and 18.4 s (peak memory 684 MB and 262 MB). Learning one game (6 runs each): 97.4 s with the code of v4.5.5-nikque.8, 96.6 s with this version (ratio 0.99 ± 0.05), 136.1 s with `book-store-tasks = 1`.
- **The Linux library is about 8% faster** (x86-64-v3, the 30 problems of `edax_bench`, gcc 11.4; time against a library built as in v4.5.5-nikque.8): 0.987 ± 0.002 with `-Bsymbolic`, and 0.926 ± 0.001 with `-fno-semantic-interposition` as well (1 thread, same node counts); 0.924 ± 0.010 with 8 threads. With `-fPIC` alone, the calls between the functions of the library were kept replaceable by functions of another program, and were not inlined. The library grows from 622 KB to 765 KB. The API, the exported names and the search results are unchanged.
- **The release builds (with PGO, `wEdax-x86-64-v4.exe`) against each other** (time of this version against the released v4.5.5-nikque.8; 8 to 24 rounds): `-solve` 1.000 ± 0.002 with 1 thread and 1.000 ± 0.007 with 32; midgame with 32 threads 0.983 ± 0.004 at level 18 and 0.996 ± 0.009 at level 21; learning games: 0.899 ± 0.003 for 128 games at level 18 (34.9 → 31.4 s), 0.983 ± 0.007 for 8 games at level 21, 0.992 ± 0.008 for 4 games at level 24; 30 games stored one by one 1.006 ± 0.014; `book fix` with 1000 leaves 0.996 ± 0.009; `book merge` of the 6.49 million position book 1.001 ± 0.003; `book deviate 0 2` 0.954 ± 0.033. **No condition is slower.** Same peak memory (1,298 → 1,150 MB for `book deviate`).
- Linux executables (against the released v4.5.5-nikque.8, `-solve`, 8 rounds): 0.994 for x86-64, 0.988 for x86-64-v3 (midgame 1.001, 8 threads 0.990), 0.995 for x86-64-v4: the same speed.
- **Not measured**: the macOS, Android and Windows ARM64 builds (they cannot be run here), levels above 30.

- `book-store-auto-save = off`: 1.004 ± 0.005 times the time of `on` for 128 games learned on the book of 270,000 positions, 0.99 ± 0.04 on a book of 770,000 positions, 0.99 ± 0.01 for 30 games learned one by one: with books of this size the difference cannot be measured (a save takes about 0.013 s). With the book of 6.49 million positions (a save takes 0.2 to 0.3 s): 0.966 ± 0.002 for 128 games (23.8 → 23.0 s), 0.87 ± 0.02 for 8 games learned one by one (18.7 → 16.3 s). The setting halves what is written to the disk, and the time of a save grows with the book.
- `book negamax` and the number of threads (6.49 million positions, one negamax): 1.97 s with 1 thread, 1.00 s with 2, 0.53 s with 4, 0.30 s with 8, 0.21 s with 16, 0.19 s with 32: almost proportional up to 8 threads, flat above. The work lost by threads walking the same positions is about 20% with 32 threads, 3% with 8.
- edax_runner at level 18 with 32 threads, 3,200 games learned in 16 minutes: its memory stayed between 717 and 725 MB.

**The real book of 657 million positions** (28.95 GB, level 18; the first check on it since v4.5.5-nikque.5). 32 threads, one run each.

| Work | v4.5.5-nikque.8 | This version | Result |
|---|---|---|---|
| Load → `book info` → save | 35.9 s, peak 31.6 GB | 36.3 s, 31.6 GB | the saved file holds the same positions' bytes as the original file (both versions) |
| `book negamax` (with the load and the save) | 53.4 s (negamax: about 19 s) | 56.0 s (about 19.6 s) | same book with both versions |
| `book merge` of another real book of 657 million positions (1.74 million positions added; with the load and the save) | 254.5 s, peak 32.9 GB | 256.0 s, 32.9 GB | same book with both versions (658,615,773 positions) |
| `book fix` (with the load and the save) | 239.6 s, peak 32.0 GB | 239.2 s, 32.0 GB | 64 links added, 31 searches. The saved books differ by the leaf move of 1 or 2 positions between two runs of the same version, and by as much (1 position) between the two versions |

- The release executable of this version also loaded the book, ran `book negamax` and `book merge` in a row and ended normally, with the same numbers of positions, links and leaves as above (peak 32.9 GB).
- No crash, error or warning. These are single runs: a difference of a few percent is within the noise. The times are about 100 times the ones of the 6.49 million position book (proportional to the number of positions). `book merge`: checking the file 13 to 16 s, adding the positions 18 s, rebuilding the links 145 to 150 s, checking the positions 17 s, negamax 19 s. `book fix`: checking the positions about 20 s, rebuilding the links 164 s, negamax 19 s.
- The book made by `book fix` changes slightly from a run to the next: with each version run twice and the books compared position by position, the leaf move differs in 1 position between the two runs of v4.5.5-nikque.8, in 2 positions between the two runs of this version, and in 1 position between the two versions (same scores; all the other 656.87 million positions are equal). The leaf searches, done with several threads, choose one or the other of two moves with the same score.

### What behaves differently (summary)

| Case | Up to v4.5.5-nikque.8 | v4.5.5-nikque.9 |
|---|---|---|
| Multi-thread search | nodes with unsearched moves could be stored | none. Results still vary from run to run |
| `book negamax` with threads on a damaged book (links leading back) | crash | ends. Values can differ from a single thread (`book fix` repairs such a book) |
| `book learn` with a time per game | every move searched with the whole time left | the remaining time is respected |
| Learning lines longer than 255 bytes, or with a randomness of 128 or more | a shorter game learned / not learned | learned as written |
| Saving under the name of a book file that could not be read | overwritten | the old file is kept as `.damaged` |
| Failed `book import`, `book new` with a level out of range | the current book is lost / never returns | the current book is kept |
| A setting that is not a number | no warning | ignored with a warning |
| `quit` with `edax < file` | previous commands could be dropped | run in its turn |
| `-cpu` on Linux and the searches done at the same time | threads piled up on the same CPUs | not used (as `book-store-tasks = 1`) |
| The last games of a group of `book learn` / `edax_book_store_games` (`book-store-tasks` not 1) | one thread per game to the end | the threads of the games that are over are added (the moves of these games can change from a run to the next) |
| libedax: `edax_stop` while the book is being changed | search interrupted, its partial result stored | not stopped |
| libedax: NULL arguments | crash | nothing happens |

### Checks

- `-solve`: single-thread results and node counts equal to v4.5.5-nikque.8 (Windows x64 and 32-bit, Linux).
- Book regression tests (all the book commands; 1 and 8 threads; including a book of 6.49 million positions): all files equal to v4.5.5-nikque.8 (except the files that hold a date, and the messages described above).
- With the test build whose searches all use one thread, the books are identical to "one search after the other, with an empty hash table for each" (the ten damaged books, `book store`, `book add`, `book learn`, and merges).
- 240 runs of `book fix` with 32 threads for the "stop and go on" step, repeated multi-thread `-solve` (no wrong result), test builds where threads cannot be created or the search memory cannot be allocated, the 32-bit build with its address space exhausted, damaged books, unwritable targets and odd settings, ThreadSanitizer (Linux).
- The real book of 657 million positions: load, save, `book negamax` and `book merge` give the same result as v4.5.5-nikque.8 (see "Speed and memory" above).
- `book-store-auto-save`: with `off` no `.store` file is written, and the saved book is the same as with `on`.
- With the two last changes (this setting, and the threads handed over while learning), all the tests above were run again (including the damaged books, the odd settings and the tests of edax_runner).
- libedax: 191 API checks, 81 cases of edge values and wrong calls, the tests of libedax4dart 7.67.0 (28 of 29; the remaining one is the same as in v4.5.5-nikque.7).

The bugs of the upstream parallel search have not been reported upstream (okuhara/edax-reversi-AVX).

## Changes in v4.5.5-nikque.8

`book-store-tasks = auto`, the default since v4.5.5-nikque.7, was **slower than v4.5.5-nikque.6 when few searches were needed or at a high level**. The way the threads are given to the searches done at the same time is fixed. This is the only change: the evaluation data `eval.dat`, the book file format, the search results and `book-store-tasks = 1` are unchanged. The books were not damaged: the books made with v4.5.5-nikque.7 can be used as they are.

### The problem

v4.5.5-nikque.7 does the searches of the learning commands (`book store`, `book add`, `book learn`) and of the link rebuild (`book fix`, `book merge`, `book import`, ...) at the same time, one thread each. With fewer searches than threads, or with a long search among short ones, the other threads waited idle until the longest one-thread search ended. The higher the level, the more long searches there are (searches that several threads speed up well), and the worse it was. It does not show when many games are learned at level 18, which is what was measured before that release.

### The fix

- **Fewer searches than threads**: each search starts with `n-tasks / searches` threads.
- **When no search is left to start**: the searches still running are stopped and continued with the threads of the ones that ended. They keep their hash tables, so what was searched before the stop is not lost. This is done each time a search can get at least twice its threads.
- **A single search**: it is exactly the search of `book-store-tasks = 1` (no more memory either).
- **The searches of the pool** (each one with its hash tables) are created while there are searches left to start: a few short searches no longer create as many of them as threads.

### Measurements

Ryzen 9 9950X (32 logical CPUs), `n-tasks` 32, `hash-table-size = auto`, the AVX-512 Windows build. Times in seconds.

| | `1` (the searches of v4.5.5-nikque.6) | `auto` of v4.5.5-nikque.7 | `auto` of v4.5.5-nikque.8 |
|---|---|---|---|
| level 24: play a game, then `book store` (34 searches) | 13.4 | **26.0** | 5.8, 10.6 |
| level 21: the same | 4.2 | **6.4** | 2.0 |
| level 18: 30 games, each one played then stored (a book of 270,000 positions) | 34.5 | 29.0 | 18.2, 18.3 |
| level 24: `book fix` with 1 leaf to search again | 1.1 to 1.5 | **6.2** | 1.2 (the same search as `1`) |
| level 24: the same, 4 leaves | 2.0 to 3.0 (average 2.5) | **6.1** | 1.4 to 2.5 (average 1.7) |
| level 24: the same, 16 leaves | 6.8 to 9.2 (average 7.8) | **8.9** | 2.9 to 3.7 (average 3.2) |
| level 18: the same, 1,000 leaves (a book of 270,000 positions) | 54.1 to 58.8 | 17.7 to 18.2 | 9.2 to 9.7 |
| level 24: `book learn` of 4 games (new book) | 36.0 | 13.2 | 10.6, 10.7 |
| level 21: `book learn` of 8 games (new book) | 31.2 | 17.9 | 11.0, 11.1 |
| level 18: `book learn` of 128 games (a book of 270,000 positions) | - | 36.1 | 33.5, 33.6 |
| level 18: 128 games learned by edax_runner | 112.9 (measured with v4.5.5-nikque.7) | 37.2, 38.8 | 35.1, 35.9 |
| level 18: `book add` of 30 games | 24.0 (the same) | 3.1 | 3.0 |
| `book merge` of a book of 6.49 million positions into an empty book (127 searches) | as v4.5.5-nikque.6 | about 0.4 s slower (4.7 to 5.1) | as v4.5.5-nikque.6 (4 alternate runs each: 5.7 to 6.3 and 5.8 to 6.3) |

- In bold: where `auto` of v4.5.5-nikque.7 was slower than `1`. `auto` of v4.5.5-nikque.8 was as fast as `1` or faster in everything that was measured.
- With few searches, the time changes from a run to the next (searches with several threads). The averages are those of 10 to 20 runs.
- `book merge` of two real books of 270,000 positions (1 search) took 0.12 s, against 0.21 s with v4.5.5-nikque.6.
- `book fix` of a sound book (6.49 million positions, no search) took 2.2 s, against 21.5 to 21.9 s with v4.5.5-nikque.6 (23.3 s and 23.2 s with one thread), with the same peak memory of 407 MB (this code is the one of v4.5.5-nikque.7).
- **Against several edax_runner at the same time** (128 games at level 18, with the time to merge their books): 42.3 s + 1.9 s of merges with 8 of them and 4 threads each, 34.9 s + 2.6 s with 16 and 2 threads each. One edax_runner with `auto` took 35.1 and 35.9 s.

**Peak memory** (each of the searches done at the same time has its hash tables):

| | `1` | `auto` of v4.5.5-nikque.7 | `auto` of v4.5.5-nikque.8 |
|---|---|---|---|
| level 24: `book store` of a game | 262 MB | 2.2 GB | 2.2 GB |
| level 24: `book fix` with 4 / 16 leaves | 261 MB | 462 MB / 1.1 GB | 684 MB / 1.1 GB |
| level 21: `book store` of a game | 261 MB | 1.3 GB | 1.4 GB |
| level 18: `book fix` with 1,000 leaves | 274 MB | 696 MB | 701 MB |
| `book merge` of 6.49 million positions | 0.55 GiB (v4.5.5-nikque.6) | 0.96 GiB | 0.57 GiB |

- With 32 threads, the hash tables of the searches done at the same time take up to about 450 MB up to level 18, 0.9 GB up to level 21 and 1.8 GB above (in proportion to the threads). With few searches, each one uses more threads and gets larger tables (never more in total than these values). Set `book-store-tasks = 1` if this memory is a problem.

**Resulting books**:

- A search that ends with its single thread gives the same result as with v4.5.5-nikque.7 (it only depends on the position). A search that got more threads, or that started with several, can give a slightly different result from a run to the next, as any search with several threads. Two runs of the learning of 128 games gave books (272,576 positions) with another leaf move in 7 positions and the same scores everywhere (`auto` of v4.5.5-nikque.7 gave the same book every time). The book differs from the one of `auto` of v4.5.5-nikque.7 in 19 positions.
- With a test build where every search keeps one thread, the books are still exactly those of the searches done one after the other with empty hash tables (the 10 damaged books, `book store`, `book add`, `book learn`, merges), as with v4.5.5-nikque.7.
- Test of the searches continued with more threads: 300 runs of `book fix` with a few leaves and 32 threads all ended normally (840 continued searches counted in 120 of them), and `book fix` of v4.5.5-nikque.6 has nothing to change in the books they make.

## Changes in v4.5.5-nikque.7

Edax can now be used as a library by other programs (libedax); games are learned with several threads (`book store`, `book add` and the new `book learn`, with the new setting `book-store-tasks`; its default, `auto`, learns `n-tasks` games at the same time); `book fix` is faster; and two book bugs are fixed. The evaluation data `eval.dat`, the book file format and the search results are unchanged.

**The books learned from games slightly differ from those of v4.5.5-nikque.6** (372 positions with different contents in a measured book of about 270,000 positions: about as much as the original learning differs when the number of threads changes). With `book-store-tasks = 1`, the learning and its books are the same as with v4.5.5-nikque.6.

### Learning games with several threads: book-store-tasks (new setting)

(v4.5.5-nikque.8 fixed how the threads are given to the searches done at the same time. The times of this section, and "the longest search still has to end", are those of v4.5.5-nikque.7: see "Changes in v4.5.5-nikque.8" above.)

`book store` (add the game just played to the book) and `book add` (add a file of games) search the positions of a game one after the other, from its end, and add them to the book. Each search is short (about 0.1 s at level 18), so more search threads stop helping at about 8 threads, and most CPUs stay idle. This is why several Edax or edax_runner were run at the same time, and their books merged afterwards. With the new setting `book-store-tasks`, one Edax learns with many threads.

| `book-store-tasks` | What happens |
|---|---|
| `auto` (the default: in the bundled `config.ini`, and without `config.ini`) | `n-tasks` games are learned at the same time (one thread per game). |
| a number n (2 or more) | n games are learned at the same time (`n-tasks / n` threads per game). |
| `1` | As up to v4.5.5-nikque.6: the positions are searched one after the other, with all the `n-tasks` threads. The book is the same as with v4.5.5-nikque.6. |

When `n-tasks` is 1, `auto` is the same as `1`. With `book-store-tasks` other than 1:

1. **Plan**: without changing the book, find which positions of the games will be added, and which moves are excluded from the search of each one (the moves that are already links). This only depends on the positions of the book and on the positions that the games add.
2. **Search at the same time**: do all these searches as one-thread searches, at the same time (up to `n-tasks` at once). Each search has its own hash tables, empty when it starts.
3. **Add in the usual order**: the positions are added in the same order, and linked in the same way, as before. Only the results of the searches come from step 2 (if a search that was not planned is needed, it is done on the spot, as before).

The new command `book learn <file>` does this for each game of the file: play its moves, let Edax play both sides to the end, then `book store` (as `init`, `play <moves>`, `go` until the game is over, and `book store` at the prompt; these are the "edax vs edax" lines of the learning list of edax_runner). Each line is a game: its moves (`f5d6c3`), or a `book-randomness` value and the moves (`2,f5d6c3`). Empty lines, lines starting with `#` and lines with `//` are skipped.

- With `book-store-tasks = 1`, each line is learned exactly as these commands do.
- With `book-store-tasks = n` (2 or more; `auto` is `n-tasks`), n games are played at the same time (`n-tasks / n` threads each); then the positions of these n games are added to the book with steps 1 to 3. The book is linked, negamaxed and saved (to `<book file>.store`) once for the n games. The games of a group are played with the book as it was before the group (what a game of the group teaches is not used by the other games of the group).
- `book add` does the searches of n games at the same time. `book store` does the searches of the positions of its game at the same time.
- The leaf searches needed while the links of a book are rebuilt (`book fix`, `book merge`, `book import`, ...) are also done at the same time when this setting is not 1 (see "A faster book fix" below).
- `book deviate`, `deviate2`, `deviate3`, `enhance` and `play` (which use `book-expand-tasks`), and `book fill`, are unchanged.

**Measured speed** (Ryzen 9 9950X with 32 logical CPUs; a real book of 270,000 positions at level 18, depth 40; the AVX-512 Windows build). 128 games (their first 32 moves on average are given; Edax plays the rest of each game) learned by edax_runner with the libedax of this fork; the times include playing and learning.

| `n-tasks` | `book-store-tasks` | 128 games | Per game | Peak memory |
|---|---|---|---|---|
| 8 | 1 | 133.5 s | 1.04 s | 179 MB |
| 32 | 1 | 112.9 s | 0.88 s | 290 MB |
| 32 | 2 (16 threads per game) | 105.3 s | 0.82 s | 1.2 GB |
| 32 | 4 (8 threads per game) | 76.4 s | 0.60 s | 1.2 GB |
| 32 | 8 (4 threads per game) | 59.7 s | 0.47 s | 1.6 GB |
| 32 | 16 (2 threads per game) | 48.7 s | 0.38 s | 1.6 GB |
| 32 | auto (32 games, 1 thread per game) | 39.4 to 41.6 s (6 runs) | 0.31 to 0.33 s | 722 MB |
| 16 | auto (16 games) | 63.4 s | 0.50 s | 503 MB |
| 8 | auto (8 games) | 102.9 s | 0.80 s | 285 MB |

- `auto` is 2.8 times (`n-tasks` 32) to 3.3 times (`n-tasks` 8) faster than `1`. The more games at the same time, the faster (a one-thread search does the most work per core).
- **Against several edax_runner at the same time** (the usual way so far): the same 128 games shared between several edax_runner (all with `book-store-tasks = 1`, each one with its own book) took 53.4 s with 4 of them and 8 threads each, 41.1 s with 8 x 4 threads, 35.2 s with 16 x 2 threads and 41.3 s with 32 x 1 thread (0.7 to 3.5 GB of memory in total; the time to merge the books afterwards is not included). One edax_runner with `auto` (40.2 to 40.8 s, 722 MB) is about as fast, with a single book and no merge.
- With `1`, this version is also faster than the libedax before this work (165.3 s to 133.5 s with `n-tasks` 8): the links of the book are now rebuilt with several threads after `book store` (see "A faster book fix" below).
- `book add` (30 games added to the book above; from the start of Edax to the saved book) took 3.4 s with `auto` and `n-tasks` 32, 24.0 s with `1` (24.4 s with `1` and `n-tasks` 8).
- **`book store` of a single game gains little.** Playing then storing 30 games one by one took 33.2 s with `auto` and 37.9 s with `1` (`n-tasks` 32; the time includes the games). The 20 to 30 positions of a game are searched at the same time, but the longest search still has to end. For the same reason, a small `book-store-tasks` such as 2 or 4 gains little (see the table above). The default, `auto`, was the fastest of the measured values.

**How much the book differs** (the same 128 games; position by position). The book learned with `auto` is not the same as the book learned game by game, but it differs no more than the books of the original learning differ when the number of threads changes. And `auto` gives the same book every time (its searches use one thread and start with empty hash tables). This is why `auto` is the default. Set `book-store-tasks = 1` when you need the same book as v4.5.5-nikque.6.

| Compared books (about 272,600 positions each) | Positions with different contents | Positions in one book only | Positions with different scores (by 1-2 / 3-4 / 5-8) |
|---|---|---|---|
| 1 thread, `1`: 2 runs | 0 | 0 | 0 |
| 32 threads, `auto`: 3 runs | 0 | 0 | 0 |
| 1 thread, `1` and 32 threads, `auto` | 372 | 5 and 8 | 95 (66 / 13 / 16) |
| 1 thread, `1` and 8 threads, `1` | 381 | 192 and 196 | 136 (123 / 9 / 4) |
| 1 thread, `1` and 32 threads, `1` | 439 | 185 and 186 | 141 (128 / 8 / 5) |
| 8 threads, `1` and 32 threads, `1` | 261 | 90 and 87 | 75 (67 / 8 / 0) |

- "Different contents": the position is in both books with another score, leaf or link. Of the 372 positions that differ between `auto` and one thread, 152 have another leaf move and 88 another leaf score only.
- Two things make the difference: (1) each search starts with empty hash tables (in the original learning, what the searches of the previous positions of the game left in the hash tables slightly changes the next searches); (2) the games of a group are played without what the other games of the group teach.
- With `book add` (30 games), the book of 32 threads with `auto` differs from the book of 1 thread with `1` in 60 positions (all the score differences are 1 or 2), with no position in one book only; the book of 8 threads with `1` differs in 97 positions. Two runs with `auto` gave the same book.
- **Check of the mechanism**: with a test build of the original learning that also empties the hash tables before every search, and one search thread, the books made by `book store` and `book add` are exactly the same as with the planned searches. So the difference from the original learning only comes from the empty hash tables. Emptying them makes the searches visit about 9% more nodes (30 games at level 18).

**Memory**: each of the one-thread searches has its hash tables. They take 14 MB each (19 bits) up to level 18 (the searches are short: they visit only 0.7% more nodes than with 21 bits), whatever `hash-table-size` is. The 722 MB of `auto` in the table above are these 32 searches (about 450 MB), the hash tables of the search of the prompt (226 MB), the book, etc.

### A faster book fix (links rebuilt with several threads)

`book fix` checks the positions (Fixing), rebuilds the links (Linking), negamaxes and sorts the book (the same steps follow `book import`, `correct`, `prune` and `subtree`; the links are also rebuilt and the book negamaxed after `book store`). The checking, linking and sorting steps used one thread; they now use all the threads.

- **Checking**: the threads check the positions; the positions found are fixed in the order of the book. Fixing a wrong position (a board that is not normalized, ...) can change what is found for the next positions, so from the first wrong position the positions are checked again one after the other, as before.
- **Linking**: the threads look for the missing links (and refresh the scores of the existing links); the links are added in the order of the book. The links to the positions whose score changed meanwhile are then set to the value that the original code gives. `book merge` rebuilds its links as up to v4.5.5-nikque.6 (with several threads since v4.5.5-nikque.3, reusing the leaves of the merged book).
- **Leaf searches of the linking**: when the best unlinked move (the leaf) of a position becomes a link, its leaf is searched again. When `book-store-tasks` is not 1, the positions to search and the moves to exclude are found first, and these searches are done at the same time, one thread each (as the games are learned). With `1`, they are done one after the other with all the `n-tasks` threads, as before.
- **The checking and the linking give the same book as one position after the other**: checked with the regression tests of all the book commands, and with 10 kinds of damaged books (removed positions, removed links, changed scores, mirrored boards, ...), with one search thread. With the leaf searches done at the same time, the book is also exactly the same as the one of a test build that does them one after the other and empties the hash tables before each search (the same 10 books, a real level 18 book with 1,000 links removed, and a merge). Against the original searches (several threads, with what the previous searches left in the hash tables), a leaf that is searched again can slightly differ, for the same reasons as when games are learned.
- The leaf searches done by the checking step (to repair a position whose leaf is wrong) are done one after the other, as before.

Measured (load a book of 6.49 million positions, 286 MB, then `book fix`; seconds):

| | Upstream v4.5.5 | v4.5.5-nikque.6 | v4.5.5-nikque.7 |
|---|---|---|---|
| 32 threads: total | 25.4 | 22.9 | **2.3** |
| checking / linking / negamax | 0.1 / 22.4 / 2.9 | 2.2 / 20.5 / 0.2 | 0.2 / 1.9 / 0.2 |
| 1 thread: total | 25.4 | 24.7 | 24.7 |
| Peak memory | 660 MB | 407 MB | 407 MB |
| `book negamax` (32 threads / 1 thread) | 3.2 / 3.2 | 0.2 / 2.3 | 0.2 / 2.3 |

- About 10 times faster with 32 threads; with one thread, the same time and memory as v4.5.5-nikque.6.
- The checking is longer than upstream because the links to positions missing from the book are also looked for since v4.5.5-nikque.2.
- On a book of 770,000 positions (32 threads): upstream 2.3 s, v4.5.5-nikque.6 1.9 s, this version 0.1 s.

**With many leaf searches** (a real level 18 book of 270,000 positions where a link of 1,000 positions was moved back to the leaf, then `book fix`: 1,000 leaves have to be searched; 32 threads, `hash-table-size = auto`):

| `book-store-tasks` | Time | Peak memory |
|---|---|---|
| `1` (one after the other, 32 threads each) | 56.5 to 58.3 s | 288 MB |
| `auto` (default: 32 one-thread searches at the same time) | 18.2 to 18.5 s | 730 MB |

- About 3 times faster. The book is exactly the same as the one of a test build that does these searches one after the other and empties the hash tables before each one.
- **A usual `book merge` gains almost nothing from this**: it reuses the leaves that the merged book searched at the same level, so it has almost no search to do (merging two real books, each one learned from 64 other games on the same book, added 908 positions and needed 1 search; merging a book of 6.49 million positions into an empty book needed 127). Many leaves are searched when `book fix` repairs a book with missing links, when the merged book has another level, etc.


### libedax: Edax as a library

The 93 functions of [libedax by lavox](https://github.com/lavox/edax-reversi) ([as maintained by sensuikan1973](https://github.com/sensuikan1973/edax-reversi)) are now provided by this Edax. Programs written for libedax ([libedax4dart](https://pub.dev/packages/libedax4dart), [edax_runner](https://github.com/sensuikan1973/edax_runner), ...) work by replacing the library file.

| File | CPU |
|---|---|
| `libedax-x64.dll` (Linux: `libedax-x86-64.so`) | Any x86-64 CPU. This is the name loaded by the programs written for libedax (on Linux, rename it to `libedax.so`). |
| `libedax-x64-v3.dll` (`libedax-x86-64-v3.so`) | CPUs with AVX2 |
| `libedax-x64-v4.dll` (`libedax-x86-64-v4.so`) | CPUs with AVX-512 |
| `libedax.universal.dylib` | macOS (both Apple silicon and Intel). |
| `libedax-arm64-v8a.so`, `libedax-armeabi-v7a.so` | Android (ARM64, 32-bit ARMv7). In an application, rename it to `libedax.so` in the folder of its ABI (`jniLibs/arm64-v8a`, ...). Only the build was checked: they were not run on a device. |

**To use it from another program** ([edax_runner](https://github.com/Nikque/edax_runner), or your own GUI, analysis tool, ...):

1. Take from `bin/` of the release ZIP the library of your OS and CPU (table above) and `data/eval.dat` (required), and put them next to your program (`data/book.dat` and `config.ini` if you need them).
2. In C or C++, include `src/libedax.h`. In another language, load the library with its foreign function interface (Dart has [libedax4dart](https://pub.dev/packages/libedax4dart); ctypes of Python, P/Invoke of C#, ... call the same functions).
3. Call `libedax_initialize` first (it reads the settings), `edax_init` to start a game, and `libedax_terminate` at the end. The functions are the commands of edax (`edax_play`, `edax_go`, `edax_hint`, `edax_book_*`, ...): they are listed in `src/libedax.h`, and `tests/libedax_test.c` calls every one of them.
4. A process has one Edax (as with the original libedax). Call the functions from one thread, one after the other (`edax_stop` and `edax_book_stop_count_bestpath` can be called from another thread to stop what is running).

A short example (`tests/libedax_example.c`; built and run on Windows and Linux):

```c
#include <stdio.h>
#include "libedax.h"

int main(void)
{
	char *args[] = {"", "-eval-file", "data/eval.dat", "-book-file", "data/book.dat", "-level", "12", "-n-tasks", "2"};
	char moves[] = "f5d6c3";
	static LibedaxHintList hints;
	LibedaxMove last;
	int i;

	libedax_initialize(9, args);  /* edax.ini, config.ini, then these arguments */
	edax_init();                  /* new game */
	edax_play(moves);
	edax_hint(2, &hints);         /* the 2 best moves: hint[1] to hint[n_hints] */
	for (i = 1; i <= hints.n_hints; ++i)
		printf("%c%c %+d\n", 'a' + hints.hint[i].move % 8, '1' + hints.hint[i].move / 8, hints.hint[i].score);
	edax_go();                    /* Edax plays a move */
	edax_get_last_move(&last);    /* squares: A1 = 0, B1 = 1, ..., H8 = 63 */
	libedax_terminate();
	return 0;
}
```

- **The functions and the layout of the data exchanged with the caller are those of the original libedax** (`src/libedax.h`). The structures of Edax changed in 4.5, so they are not passed as they are: the data are copied to structures with the original layout.
- **Settings** are read from `edax.ini` and `config.ini` of the current folder, then from the arguments of `libedax_initialize` (the last one wins). The syntax and the settings are those of the edax program.
- New functions: `edax_book_deviate2` and `edax_book_deviate3` (`book deviate2` and `deviate3`), `libedax_cpu_level` (which build the CPU can run: a program can ask `libedax-x64.dll`, then load the v3 or v4 library), `edax_book_store_games` (play and learn several games together, as `book learn` above does; the games are given as a string, one game per line) and `edax_book_store_tasks` (the number of games learned at the same time: `n-tasks` when `book-store-tasks` is `auto`). v4.5.5-nikque.9 adds `edax_book_save_checked` (saves the book and tells whether it was saved) and `edax_book_failed` (tells whether the last book function could not add a position).
- Differences from the original libedax (Edax 4.4):
  - The default level is 18 (it was 21). At the same level, the scores and moves of a search can differ from Edax 4.4.
  - `edax_book_merge` does what `book merge` does in this version: it also rebuilds the links, fixes and negamaxes the book (the original only added the positions).
  - The counts of the best paths (`edax_book_count_bestpath`, `edax_book_count_board_bestpath`) were kept in every position of the book by the original libedax; here they are kept beside the book, only while they are used (a book position still takes 48 bytes). They are counted again when the limits change or when the book changes.
  - The `link` array of a `Position` stays valid until 63 other positions are asked (8 up to v4.5.5-nikque.8). The lock which followed the result of `edax_bench` is not used.
  - Calls which crashed the original libedax (a second initialization or termination, `edax_get_last_move` before any move, `edax_book_show` on a position missing from the book, a read-only string given to `edax_get_bookmove_with_position_by_moves`) now do nothing or return an empty result.
  - On Windows, file names are read as UTF-8 (then as ANSI).
- **The edax program is not affected.** The code of libedax is only compiled when the library is built. An edax built from the sources with libedax added is byte-identical to one built from the v4.5.5-nikque.6 sources (except the build time and the name of the source folder).
- Speed and memory (one thread, the 20 endgame positions of `bench`, Ryzen 9 9950X):

  | | Speed | Peak memory |
  |---|---|---|
  | Original libedax (Edax 4.4, x86-64) | 53 million nodes/s | 159 MB |
  | `libedax-x64.dll` | 69 million nodes/s | 82 MB |
  | `libedax-x64-v3.dll` | 81 million nodes/s | 82 MB |
  | `libedax-x64-v4.dll` | 92 million nodes/s | 82 MB |

  The library and the edax program (for the same CPU) search the same number of nodes at the same speed.
- Tests: `tests/libedax_test.c` calls every function (147 checks, 191 in v4.5.5-nikque.9, 193 in v4.5.5-nikque.10; passed by the 3 Windows and the 3 Linux libraries; the original libedax gives the same results on the checks it supports). 28 of the 29 tests of libedax4dart 7.67.0 pass; the other one compares a search score (it differs because of the default level and of the state left by the previous searches: a fresh search at the same level gives the same score as the original libedax). The books saved by the libedax of Edax 4.4 are read by this version, and the books saved by this version are read by the libedax of Edax 4.4 (all the positions of a 270,000-position book are the same).
- Build: on Windows, `nmake -f NMakefile vc-lib` (`vc-lib-x64`, `vc-lib-x64-v3`, `vc-lib-x64-v4`); elsewhere, `make libbuild ARCH=<x86-64|x86-64-v3|x86-64-v4> COMP=gcc OS=linux`. For Android: `ndk-build -C src NDK_PROJECT_PATH=. NDK_APPLICATION_MK=./Application-lib.mk NDK_OUT=./obj-lib NDK_LIBS_OUT=./libs-lib` (it makes `src/libs-lib/<ABI>/libedax.so`; checked with NDK r27d). Test: `tests\build-libedax-test.cmd`. For macOS, the release-binaries workflow builds the arm64 and x86-64 libraries and joins them in one file, `libedax.universal.dylib` (the name loaded by the programs written for libedax).

### Bug fix: the book was sometimes not saved on exit after learning

Since v4.5.5-nikque.2, the book is saved on exit only if it changed after the last save. But the saves of the progress of the learning commands to a side file (`.store` of `book store`, `.dev`, `.dev2`, `.dev3` of `book deviate`, `.enh` of `book enhance`, `book play`, `book fill`, `.gam` of `book add`, `book deepen`, and the timed saves) also counted as a save. So after `book store`, for example, quitting without `book save` left the learned positions in `data/book.dat.store` only: `data/book.dat` was not saved. A progress save does not count as a save anymore. After `book save`, or when nothing changed, the book is still not saved on exit, as before.

### Progress of book fix

"Fixing book..." of `book fix` (also used by `book import`, `correct`, `prune` and `subtree`) only showed its progress when positions were fixed, so nothing was printed until the end with a correct large book. The checked positions are now printed once per second (for example `Fixing book...3137536/6491163 positions checked`). The book is unchanged.
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

**Since v4.5.5-nikque.10**, up to level 18, a round with at least 32 times `n-tasks` positions to expand (1,024 with 32 threads) runs `n-tasks` searches of one thread (see "Changes in v4.5.5-nikque.10" above). Rounds with fewer positions are as described here.

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
| `book-store-tasks` | `auto` | Number of games learned at the same time by `book store`, `book add` and `book learn` (and by edax_runner) (below). |
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

**Since v4.5.5-nikque.10**, up to level 18, a round with at least 32 times `n-tasks` positions to expand (1,024 with 32 threads) runs `n-tasks` searches of one thread (see "Changes in v4.5.5-nikque.10" above). Rounds with fewer positions are as described here.

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

### book-store-tasks

With `book-store-tasks = auto` (the bundled value, and the default without `config.ini`), `book store`, `book add` and `book learn` learn `n-tasks` games at the same time (one thread per game); with a number n (2 or more), n games at the same time (see "Learning games with several threads" above for how it works and for the measurements). With `book-store-tasks = 1`, they search the positions of a game one after the other, with all the `n-tasks` threads: exactly as up to v4.5.5-nikque.6, with the same book as a result.

- **Speed**: `auto` is the fastest: 2.8 to 3.3 times faster than `1` in the measurement above (32 logical CPUs, level 18). `auto` is faster than 8 or 16, with less memory.
- **Resulting book**: with `auto` (or a number of 2 or more) it is not the same as with `1` (each search starts with empty hash tables, and the games of a group are played without what the other games of the group teach). See the table above for how much it differs. Set `1` to make the same books as the previous versions. Since v4.5.5-nikque.8 some searches get more threads while they run, so the book of `auto` can also slightly change from a run to the next (7 positions of 270,000).
- **Leaf searches while the links are rebuilt** (`book fix`, `book merge`, `book import`, ...): they are also done at the same time, one thread each, when this setting is not 1.
- **Memory**: the positions are searched by up to `n-tasks` searches at the same time, each one with its hash tables. With fewer searches than `n-tasks`, each search uses several threads and larger tables (never more in total than `n-tasks` one-thread searches). The one-thread searches take 14 MB each (19 bits) up to level 18, 28 MB up to level 21 and 57 MB above, whatever `hash-table-size` is (there are `n-tasks` of them: a large number would take too much memory; only a smaller number is used as it is). When `n` is smaller than `n-tasks`, add the hash tables of the n searches that play the games (`n-tasks / n` threads each).
- edax_runner (built with the libedax of this fork) learns the "edax vs edax" lines of its learning list by groups of that many games when this setting is not 1.

### book-store-auto-save

With `book-store-auto-save = on` (the default; the setting is not in the bundled `config.ini`), the book is saved to `<book-file>.store` (for example `data/book.dat.store`) after `book store`, and after each group (`book-store-tasks` games) of `book learn`, as in the previous versions. With `off` this save is not done (the book is saved to the book file by `book save`, or on exit). Added in v4.5.5-nikque.9. Write `book-store-auto-save = off` in `config.ini`, or `-book-store-auto-save off` on the command line. edax_runner (v5.3.0-nikque.3) saves `book.dat` itself after each learning, and sets it to `off` by itself.

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
| macOS Intel x86-64 / Apple silicon (arm64) | `mEdax-x64-modern` / `mEdax-arm64` |
| Android ARM64 / 32-bit ARMv7 | `aEdax-arm64-v8a` / `aEdax-armeabi-v7a` |
| Library (libedax), Windows x86-64: baseline / AVX2 / AVX-512 | `libedax-x64.dll` / `libedax-x64-v3.dll` / `libedax-x64-v4.dll` |
| Library, Linux x86-64: baseline / AVX2 / AVX-512 | `libedax-x86-64.so` / `libedax-x86-64-v3.so` / `libedax-x86-64-v4.so` |
| Library, macOS (both Apple silicon and Intel) | `libedax.universal.dylib` |
| Library, Android ARM64 / 32-bit ARMv7 | `libedax-arm64-v8a.so` / `libedax-armeabi-v7a.so` |

The `v3` builds require an AVX2-capable x86-64 CPU; the `v4` builds require an AVX-512-capable x86-64-v4 CPU. Use the baseline build when unsure. `config.ini` is a starting configuration (see "Settings (config.ini)" above); set paths, `book-save-interval`, `book-deviate-save-rounds`, `book-merge-auto-save`, `hash-table-size`, `book-expand-tasks`, and `book-store-tasks` for your environment. To rebuild a Windows executable, open a Visual Studio 2022 Developer Command Prompt, change to `src`, and run a target such as `nmake -f NMakefile vc-x64-v4` (`build-win-v4.cmd` also builds the v4 executable). The [release-binaries workflow](.github/workflows/release-binaries.yaml) builds the other platform variants, and `package-release.py` assembles the runtime ZIP. The release Windows executables use the profile-guided targets (`vc-pgo-x64-v4`, `vc-pgo-x64-v3`, `vc-pgo-x64`, and `vc-pgo-x86-sse` and `vc-pgo-x86` from an x86 prompt) except ARM64. The v4.5.5-nikque.4 executables were built with Visual Studio 2022 (MSVC 19.44) for Windows, gcc 11.4 on Ubuntu 22.04 (WSL) for Linux and NDK r27d for Android; the macOS executable was built by the release-binaries workflow. The 32-bit Linux build (`lEdax-x86`) links libatomic statically for the atomic operations of the parallel book code, so it needs an i486 or later CPU.

The upstream 32-bit macOS `mEdax-x86` is deliberately omitted. Current Xcode SDKs lack the i386 libraries needed to link a corrected binary; including the upstream executable would leave this fork's fixes absent from that file.

Work on this fork used **ChatGPT-6 Astra** and **ChatGPT-6 Sol**; the performance work and bug fixes of v4.5.5-nikque.3 to v4.5.5-nikque.5 used **Claude Opus 5.5** (Claude Code). Edax and its original authors retain their respective attribution. This fork is distributed under the original GPL-3.0 license; keep the source and license available when redistributing the executable.
