# 13. Troubleshooting

[Contents](README.md) | Previous: [12. Using Edax from other programs](12-integration.md) | Next: [14. Reference](14-reference.md)

Common messages and things that tend to happen, by situation. `WARNING:` means "the work can go on, but take care"; `ERROR:` means "that operation was not done" (an `ERROR:` line also shows the name of a source file and a line number).

## 13.1 At start

| Message, or what happened | Meaning | What to do |
|---|---|---|
| `Cannot open data/eval.dat`, and Edax ends | The evaluation data was not found. Edax looks for `data/eval.dat` in the current folder | Start from inside the `bin` folder. Or give the place with `-eval-file folder/eval.dat` |
| Edax ends at once without printing anything (`v3`, `v4` executables) | The CPU may not support those instructions (AVX2, AVX-512) | Use an executable without `v3` or `v4` in its name (what is printed in this case was not checked) |
| `WARNING: config.ini was not found in the current folder (Edax was started without its folder name): default settings are used` | The executable was started without a folder name, so the place of `config.ini` was unknown | Start it with its folder (`.\wEdax-x86-64.exe`). Or put a `config.ini` in the current folder |
| `WARNING: config.ini:7: unknown or incomplete setting "levle" ignored` | The name on line 7 of `config.ini` is wrong | Correct that line ([4.2](04-settings.md)) |
| `WARNING: level: "abc" is not a number; ignored` | Something that is not a number where a number is expected | Correct the value |
| A message like `WARNING: n-tasks = 99 is out of range. Set to 32` | The value is out of range. It was replaced by the end of the range | It works as it is. Correct the value if you mind |
| `New book 18 21...` | The file of `book-file` does not exist, so a new book was made | Normal at the very first start. If there should be a book, check the name in `book-file` and the folder Edax was started from |
| `data/book.dat could not be loaded: it is kept as data/book.dat.damaged` | The book file could not be read (damaged, cut short, in use by another program). When a save was due later, the original file was kept under another name | The `.damaged` file is the original book. If another program merely had it open, rename it back and it can be read. If it is really damaged, go back to a copy or to an intermediate file (`.dev2` and so on) |

## 13.2 While playing

| Message, or what happened | Meaning | What to do |
|---|---|---|
| `WARNING: Unknown command/Illegal move: "e3 "` | A square that cannot be played, or an unknown command | The playable squares are the `.` of the board on the left. For the spelling of the commands: [chapter 14](14-reference.md) |
| You play a move and Edax does not answer | The mode is `mode 3` (just after start, after `stop`) | `mode 0` (you are Black) or `mode 1` (you are White). For one move only: `go` |
| No square can be played and nothing moves on | It is a pass | `ps` |
| Edax keeps thinking and does not come back | The level is high, or the time limit is long | `stop`. Then lower `level` and type `mode 0` or `mode 1` |
| Edax plays at once without thinking | It plays moves of the book (`depth` shows `book` in the table). Or the level is low | `book off` if you do not want the book |
| The same position gives a different move each time | A random choice among moves of the same score in the book. Or the variation of a search with several threads | [6.5](06-book-basics.md), [5.7](05-search.md) |
| `WARNING: Cannot open file mygame.txt` | The file given to `load` cannot be opened (wrong name or folder). The board stays as it was | Check the name and the place of the file. A folder is relative to the current folder (`bin`) |
| `WARNING: Illegal move #2: A1` | The game given to `load` holds a move that cannot be played (`#2` is the number of the move, counted from 0). The game is not loaded and the board stays as it was | Correct the game file ([10.1](10-files.md)) |
| `WARNING: game text: "A1F4" is not a move that can be played here: the rest of the line is not read` | A line of a `.txt` game holds a move that cannot be played (or characters that are not a move). The moves before it were loaded | Correct the line. A line saved as `.txt` from a game that starts from a set-up position cannot be read back ([10.1](10-files.md)) |
| `load` prints nothing and changes nothing (versions up to v4.5.5-nikque.12) | The file cannot be opened, or the game holds a move that cannot be played. The old versions do not say why | Check the name and the place of the file, and the content of the game |
| `WARNING: Unknown game format extension: .abc` | The extension is not a known format | Use `.txt`, `.sgf`, `.ggf`, `.pgn` or `.edx` ([3.6](03-playing.md)) |
| `WARNING: error while importing a SGF game` (also GGF, PGN) | The game holds a move that cannot be played. Or it is an SGF file that starts from a set-up position. The game is not loaded and the board stays as it was | Correct the game file. Save games that start from a set-up position as `.ggf`, `.pgn` or `.edx` ([10.1](10-files.md)). **Up to v4.5.5-nikque.12 the result may be a wrong board, with a move put down without turning over any disc. In that case start again with `init`** |
| `WARNING: uncomplete game.` (versions up to v4.5.5-nikque.12) | A `.pgn` of a game that is not finished was loaded | Normal. The game is loaded |
| `WARNING: board_set: bad string input` | The board given to `setboard` has fewer than 64 squares, or the side to move is missing | Count the 64 squares again and write the side to move (`X` or `O`) at the end ([3.5](03-playing.md)) |
| A time limit was given, yet the reading stops at the depth of the level | When `-l` (or the `level` command) is given, it caps the reading | Do not give `-l`. A `level` in `config.ini` is not a cap ([3.8](03-playing.md)) |

## 13.3 With the book

| Message, or what happened | Meaning | What to do |
|---|---|---|
| `book show` prints nothing | The current position is not in the book | Normal. Add it with `book store` and the like |
| `book deviate` (`deviate2`, …) ends at once without printing anything | The position on the board is not in the book | `init` to start from the initial position. To start from another position, `book store` first |
| It ends with `Book deviate2 0 todo` | No leaf meets the condition (all were expanded already, or the numbers are too small) | Make the numbers (X, Y) larger. Check the depth of the book too (`Depth` of `book info`) |
| `Cannot save book to ; existing book was not replaced` | With a version up to v4.5.5-nikque.12: `book save` was typed without a file name | Write the name, as in `book save data/my.dat` (since v4.5.5-nikque.13, leaving the name out saves to the file of `book-file`) |
| `WARNING: data/book.dat holds the book in use before "book new": it is kept as data/book.dat.old` (or `"book load"`, `"book import"`) | After `book new`, `book load` of another file or `book import`, a save without a file name (quitting, or `book save` alone) replaced the file of `book-file` with another book. The previous file was kept under a name with `.old` added | Normal. To go back to the previous book, quit Edax and rename the `.old` file back ([6.7](06-book-basics.md)). If it is not needed, delete the `.old` file |
| `WARNING: wrong depth: % depth …` | The depth line of the file given to `book import` cannot be read (not a number from 1 to 60) | The import is done. Check the depth with `book info` and correct it with `book depth number` ([8.5](08-book-maintenance.md)) |
| `WARNING: wrong board: % depth 12` and `1 lines of … hold no position: skipped` (versions up to v4.5.5-nikque.12) | A file written by `book export` of v4.5.5-nikque.13 or later was given to `book import` of an old version. The last line (the depth) could not be read and was skipped | All the positions are read. Check the depth with `book info` and correct it with `book depth number` ([8.5](08-book-maintenance.md)) |
| `Cannot open temporary book …` (the book cannot be saved) | The destination cannot be written: the folder does not exist, no permission to write, or **the name, folder included, is too long** (13.4 below) | Create the folder, or use a shorter place |
| `Cannot write complete book to …; existing book was not replaced` | The write failed on the way (not enough free disk space, for example). The original file remains | Free some disk space and save again. Do it **before quitting Edax** (when Edax ends, the book in memory is lost) |
| `WARNING: Book … was not loaded; current book retained` | The file given to `book load` could not be read. The current book is unchanged | Check the name and the place (up to v4.5.5-nikque.12, when the file does not exist, a `New book …` is printed before it: it belongs to the failed attempt and does not affect the current book) |
| `(2245 positions have another level than the book: only the first 10 are shown)` | Printed by `book info`: the book holds 11 or more positions of another level than its own (after merging a book of another level, for example) | Normal. To avoid mixing levels, merge books grown at the same level ([8.3](08-book-maintenance.md)) |
| `cannot open …`, `WARNING: Book … was not merged` | The file given to `book merge` could not be read. The current book is unchanged | Check the name and the place |
| `WARNING: Unknown book command: "book link"` | There is no such book command | See the list of [chapter 14](14-reference.md). To make the links again, use `book fix` |
| `[stop: nothing is stopped while a book or base command is running]` | `stop` was typed while a book or base command was running. Nothing was stopped | To stop at once, stop Edax itself ([7.5](07-book-learning.md)) |
| `quit` was typed but Edax does not end | It waits for the book command to end | Wait. If you cannot, stop Edax itself (what came after the last save is lost) |
| `WARNING: book subtree: this position is deeper than the book depth; the book is not changed` | The position on the board is beyond the depth of the book | Check `book depth`. Go back to an earlier position and try again |
| `not enough memory to search the positions at the same time` | Not enough memory for the simultaneous searches. The work goes on with fewer of them | It works as it is. Lower `book-expand-tasks`, or set `hash-table-size` to `auto` |
| `not enough memory to play N games at the same time: they are learned one after the other` | Not enough memory to learn at the same time. The games are learned one by one | Lower `book-store-tasks` |
| `cannot create a thread: the search uses N threads instead of M` | A thread could not be created. The work goes on with those that could | It works as it is. Lower `n-tasks` |
| The book file suddenly became small (84 bytes or so) | Edax was ended after `book new`, or it was started without a book file and then ended | After `book new`, the previous book remains in the file with `.old` in the same folder (since v4.5.5-nikque.13; [6.7](06-book-basics.md)). Earlier versions do not keep it: go back to a copy or to an intermediate file |
| A file like `book.dat.tmp.12345` is left over | Edax was stopped during a save | It can be deleted (on Windows, Edax deletes it at the next save). `book.dat` is as it was |
| The same steps give a slightly different book each time | Searches with several threads and simultaneous expansions vary a little from run to run. Games that use the moves of the book choose at random among equal moves | Normal. If exactly the same book is needed: `n-tasks = 1`, `book-expand-tasks = 1`, `book-store-tasks = 1` (much slower) |
| The PC is slow while a book is grown | All the CPUs are in use | Start again with a lower `n-tasks` |

## 13.4 On Windows, the intermediate files of the book are not saved

When Edax saves a book, it first writes under the name "name to save + `.tmp.` + a number". On Windows a file name, folder included, can have 259 characters at most, so with a long `book-file` this temporary file cannot be created. This concerns above all the saves under a name with an added extension, such as `.store` or `.dev2`, which are longer by that much.

```
Cannot open temporary book …
```

- Only that save is not done; the book in memory is unchanged and the work goes on.
- **Keep the length of `book-file`, folder included, within 240 characters.** Within 240 characters every save fits.
- Even when a relative name (`data/book.dat`) is given, what counts is the whole length from the drive letter. Check that Edax was not unpacked into a deep folder.

## 13.5 If it is still unclear

- Check the current settings with `options` (is the `config.ini` you expect being read?).
- Check the number of positions, the level and the depth of the book with `book info`.
- Starting with a file name in the settings `ui-log-file` (record of the input and the output) and `search-log-file` (record of the searches) lets you see afterwards what happened.
- `README-NIKQUE.en.md` of the package lists, for each version, "what changes in behaviour" and "what was not fixed, what to know".

Next: [14. Reference](14-reference.md)
