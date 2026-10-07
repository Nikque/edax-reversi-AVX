# 4. Settings

[Contents](README.md) | Previous: [3. Commands for playing and analysing](03-playing.md) | Next: [5. Search basics](05-search.md)

The behaviour of Edax is changed with **settings**. There are three ways to give them, and all use the same names.

| Way | When it applies | Example |
|---|---|---|
| Write it in the settings file `config.ini` | At every start | `level = 18` |
| Give an option when starting (command line) | For that one run | `wEdax-x86-64.exe -level 18` (short: `-l 18`) |
| Type it after `>` while Edax runs | From the next search or command that starts after it | `level 18` |

## 4.1 The order in which settings are read

At start, Edax reads its settings in this order. **What is read later wins.**

1. The values built into the program ("built-in value" in the table of 4.5)
2. `edax.ini` in the current folder (if there is one; the package has none)
3. `config.ini` **in the folder of the executable** (`bin/config.ini` in the package)
4. The options of the command line

Notes:

- The `config.ini` that is read is the one next to the executable. A `config.ini` in the current folder is not read when the executable is in another folder.
- When the executable is started by its name alone, without a folder (because it is in a folder of the `PATH`), Edax does not know where the executable is and looks for `config.ini` in the current folder. If there is none, it prints this warning and runs with the built-in values:

  ```
  WARNING: config.ini was not found in the current folder (Edax was started without its folder name): default settings are used
  ```
- `options`, typed after `>`, shows all the current values.

## 4.2 How to write config.ini

`config.ini` is a plain text file that you can edit with Notepad or any editor. One setting per line, as `name = value`.

```
# strength of the search
level = 18
n-tasks = auto
book-file = data/book.dat
```

- From `#` to the end of the line is a comment. Empty lines are skipped.
- `level = 18`, `level=18`, `level 18` and `set level 18` all mean the same.
- Names ignore upper and lower case. Spaces, `_` and `-` inside a name are the same (`book-depth`, `book_depth` and `Book Depth` are one setting).
- For an `on`/`off` setting, `true`/`false`, `yes`/`no` and `1`/`0` can be written as well.
- With `=`, the whole rest of the line is the value (a file name may contain spaces).
- Full-width spaces and the full-width "＝" are accepted. The encoding is UTF-8 (with or without a BOM); Windows and Linux line ends are both fine.
- A wrong name gives a warning at start, with the file name and the line number (the line is ignored):

  ```
  WARNING: config.ini:7: unknown or incomplete setting "levle" ignored
  ```
- A value that is not a number (`level = abc`), or not `on`/`off`, also gives a warning and is ignored. A number out of range gives a warning and becomes the nearest value of the range.

Every option of the command line can be written in `config.ini` under its name without the leading `-` (`-n 16` is `n-tasks = 16`, `-book-file data/my.dat` is `book-file = data/my.dat`).

## 4.3 Giving options on the command line

```
wEdax-x86-64.exe -l 12 -n 4 -book-file data/my.dat
```

- Any number of `-name value` pairs.
- Some options are switches without a value: `-q` (quiet; the same as `-verbose 0`), `-vv` (verbose; the same as `-verbose 2`), `-cpu` (give each thread its own CPU: on Linux the thread is bound to that CPU; on Windows it is only a preference. On Linux, with this option the book commands do not use searches run at the same time (`book-expand-tasks`, `book-store-tasks`)).
- `-?` or `-help` prints the list of options (English) and ends. `-v` or `-version` prints the version.
- Besides settings, some options change what Edax does at start:

  | Option | Meaning | Chapter |
  |---|---|---|
  | `-solve file` | Solve a problem file, print the results and end | [11](11-solve-bench.md) |
  | `-bench n` | Measure the speed and end | [11](11-solve-bench.md) |
  | `-gtp`, `-nboard`, `-xboard`, `-ggs`, `-cassio` | Start with the language (protocol) used to talk to another program | [12](12-integration.md) |
  | `-o file` (`-option-file`) | Also read settings from that file (written like `config.ini`) | |

## 4.4 Changing settings while Edax runs

Type the name and the value after `>`.

```
>level 12
>n-tasks 4
>book-randomness 2
```

- If nothing is printed, the setting was accepted. A wrong value gives a warning:

  ```
  >level abc
  WARNING: level: "abc" is not a number; ignored
  ```
- Changing `hash-table-size` while Edax runs rebuilds the tables of the search at that size (their content is lost). With `hash-table-size = auto`, changing `n-tasks` while Edax runs rebuilds them too, for the new number of threads (started with 1 thread, typing `n-tasks 16` took the memory in use from 79 MB to 242 MB). Up to v4.5.5-nikque.12 only the output of `options` changed, and the tables kept the size they had at start.
- Changing `book-file` while Edax runs does not load another book. What changes is where the automatic saves go from then on ([chapter 6](06-book-basics.md)). To read another book, use `book load file`.

## 4.5 The settings of the config.ini of the package

The `bin/config.ini` of the package holds these settings. The "built-in value" is the value when there is no `config.ini` (or when the line is removed).

| Setting | In the package | Built-in value | Meaning |
|---|---|---|---|
| `level` | `18` | 18 | Strength of the search at start (0 to 60). [Chapter 5](05-search.md) |
| `n-tasks` | `auto` | number of logical CPUs | Number of threads of the search. `auto`: the number of logical CPUs of the PC |
| `book-depth` | `auto` | `auto` | Depth of the book at start. `auto`: the depth saved in the book file. A number from 1 to 60 sets that depth at start. [Chapter 6](06-book-basics.md) |
| `book-usage` | `on` | `on` | Play the moves of the book in games, or not |
| `book-save-interval` | `360` | 60 | Minutes between the timed saves while a book is being grown. `0`: no timed save. [Chapter 7](07-book-learning.md) |
| `book-deviate-save-rounds` | `1` | 1 | For `book deviate` and its family: save after this many rounds that added something. `0`: only at the end. [Chapter 7](07-book-learning.md) |
| `book-merge-auto-save` | `on` | `on` | After each successful `book merge`, save to `<book file>.mrg`. [Chapter 8](08-book-maintenance.md) |
| `hash-table-size` | `auto` | 21 | Size of the hash table of the search. `auto`: chosen from the number of threads. [Chapter 5](05-search.md) |
| `book-expand-tasks` | `auto` | 1 | Number of positions that the automatic book commands expand at the same time. Chapters [7](07-book-learning.md) and [9](09-large-books.md) |
| `book-leaf-recalculate-rounds` | `1` | 1 | Largest number of rounds of `book leaf-recalculate` and its family. [Chapter 8](08-book-maintenance.md) |
| `book-store-tasks` | `auto` | `auto` | Number of games learned at the same time by the commands that grow a book from games. Chapters [7](07-book-learning.md) and [9](09-large-books.md) |
| `probcut-model` | `standard` | `standard` | Pruning model of the search. `refit` is experimental (it changes the search results; read the v4.5.5-nikque.5 section of the README of the package before using it) |

**A note on `level` in `config.ini`**: a `level` written in `config.ini` (or `edax.ini`) only sets the level at start. In a game with a time limit (`-t`, `-move-time`) it does not cap the reading. To cap it, give `-l` on the command line or use the `level` command ([3.8](03-playing.md)).

## 4.6 Other settings used often

| Setting | Built-in value | Meaning |
|---|---|---|
| `book-file` | `data/book.dat` | The book file. It is read at start, and the automatic saves go there (and to this name with an extension added) |
| `eval-file` | `data/eval.dat` | The evaluation data file |
| `book-randomness` | 0 | When playing from the book: by how many discs a move may be worse than the best one and still be chosen. [Chapter 6](06-book-basics.md) |
| `book-store-auto-save` | `on` | Save to `<book file>.store` after `book store` and `book learn`, or not. [Chapter 7](07-book-learning.md) |
| `game-time` (`-t`) | none | Time for a whole game. [3.8](03-playing.md) |
| `move-time` | none | Time for one move |
| `ponder` | `on` | Think during the opponent's turn |
| `mode` | 3 | The `mode` at start ([3.2](03-playing.md)) |
| `verbose` | 1 | How much is displayed (0 to 4; [3.10](03-playing.md)) |
| `search-log-file` | none | File where the search is logged |
| `ui-log-file` | none | File where the input and the display are logged |
| `auto-start`, `auto-swap`, `auto-store`, `auto-quit`, `repeat` | `off`, 0 | What happens when games are played in a row ([3.9](03-playing.md)) |

All the settings are in the tables of [chapter 14](14-reference.md).

## 4.7 A guide to choosing settings

| Use | Settings |
|---|---|
| Playing and analysing only | The `config.ini` of the package can be used as it is. Adjust the strength with `level` |
| Using the PC for other things at the same time | Set `n-tasks` to about half the number of logical CPUs (the PC stays more responsive) |
| The same results at every run (for checks) | `n-tasks = 1`, and `hash-table-size` given as a number ([5.6](05-search.md)) |
| Growing a book | Read chapters [7](07-book-learning.md) and [9](09-large-books.md), then choose `book-expand-tasks`, `book-store-tasks` and `book-save-interval` |
| Growing a book exactly as earlier versions (the original Edax) did | `book-expand-tasks = 1` and `book-store-tasks = 1` |

Next: [5. Search basics](05-search.md)
