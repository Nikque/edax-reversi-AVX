# 6. Book basics

[Contents](README.md) | Previous: [5. Search basics](05-search.md) | Next: [7. Growing a book](07-book-learning.md)

## 6.1 What a book is

A **book** (opening book) is data that holds positions of the opening and the midgame together with their scores. When the current position is in the book, Edax **plays the move of the book without thinking**. A book brings these advantages:

- The opening is played from a much deeper reading (the time spent when the book was built) than a search during a game.
- No time is used for the opening moves.
- The score of each position can be seen at once (useful for study).

A book is usually one file (`bin/data/book.dat` in the package). The `book.dat` of the package is an almost empty book: **it holds the initial position only**. Grow it yourself ([chapter 7](07-book-learning.md)), or replace it with an Edax book made by someone else (the format of Edax 4.4 and 4.5).

All book commands start with `book` (or `b` for short). The word after `book` may be written in upper case (`Book Info`).

## 6.2 Inside a book: positions, links and leaves

A book is **a set of positions**. For each position it records:

| Item | Meaning |
|---|---|
| Board | The discs. The 8 shapes obtained by rotating and mirroring are kept as one position |
| Level | The level at which the position was searched ([chapter 5](05-search.md)) |
| Score | The score of the position (the expected disc difference, seen from the side to move), with its lower and upper bounds |
| **Link** | A move that "leads to another position of the book", and its score |
| **Leaf** | **The one best move** among those that are not links, and its score (a value found by a search) |
| Game counts | The number of lines through this position (won, drawn, lost, not finished) |

What matters is **the difference between a link and a leaf**.

- **The move of a link**: the position after it is in the book too. What was read further down is reflected in its score.
- **The move of a leaf**: the position after it is not in the book yet. It is a mark that says "nothing was examined beyond this, but among the remaining moves this one looks best". **Growing a book means turning promising leaves into links (adding the positions after them to the book).**
- A move that is neither a link nor a leaf is not recorded in the book (the search judged it "worse than the leaf move").

The score of a position is the best of the scores of its links and its leaf. When the score of a lower position changes, the scores of the positions above it change too. This computation, "carrying the scores from the bottom to the top again", is called **negamax**. The commands that change the book run negamax by themselves at their end.

## 6.3 Looking into the book

### book show: the current position

It shows how the position on the board is recorded in the book.

```
>book show

Level: 8
Best score: +0 [-2, +2]
Moves: [d3:+0] <f4:-3>
       152 incomplete lines.
```

| Line | Meaning |
|---|---|
| `Level: 8` | The position was searched at level 8 |
| `Best score: +0 [-2, +2]` | The score is +0. `[lower, upper]` is a margin that allows for the error of the search |
| `Moves:` | The recorded moves. **`[move:score]` is a link, `<move:score>` a leaf.** Best first |
| `152 incomplete lines.` | 152 lines from this position do not reach the end of the game |

- The case of a move tells the side (Black's moves in upper case, White's in lower case).
- When some lines reach the end of the game, a line `Lines: N full games with …% win, …% draw, …% loss` is printed too.
- When the current position is not in the book, **nothing is printed**.

Typing `book show` as the game goes on lets you follow an opening move by move.

### book info: the whole book

```
>book info
Edax Book 4.5; 2026-10-7 17:14:18;
Positions: 2246 (moves = 2301 links + 2243 leaves);
Level 8 : 2246 nodes
Depth: 12
Memory occupation: 1160986
Hash balance: 0 < 0 < 2
```

| Line | Meaning |
|---|---|
| First line | The version of the book format, and the date and time when the book was made (or last rebuilt) |
| `Positions` | The number of positions, of links and of leaves |
| `Level 8 : 2246 nodes` | The number of positions for each level. Usually one line only (the level of the book) |
| `Depth: 12` | The depth of the book (6.4) |
| `Memory occupation` | The size the book takes in memory (bytes) |
| `Hash balance` | How even an internal table is (of no concern) |

When positions of another level than the level of the book are mixed in, `book info` prints the first 10 of them, one per line (lines that start with `{board:…}`). When there are more, a line such as `(2245 positions have another level than the book: only the first 10 are shown)` gives their number (up to v4.5.5-nikque.12 every one of them was printed).

### book stats: the counts in detail

`book stats` shows the number of positions, links and leaves for each number of empty squares, and the number of positions for each score.

```
Stage distribution:
stage    positions        links       leaves      terminal nodes
   48          880            0          880          880
   49          625          909          625            0
   50          352          639          352            0
 …
   60            1            4            0            0
```

`stage` is the number of empty squares. `terminal nodes` is the number of positions without any link (the positions of the deepest layer, beyond which nothing is in the book, for example).

## 6.4 The level and the depth of a book

A book has two settings of its own.

**Level**: the level used to search positions. The commands that grow the book search each new position at this level. **Mixing books of different levels makes the reliability of the scores uneven**, so one book is normally grown at one level. A higher level makes the scores more reliable, and adding one position takes longer.

**Depth**: up to which move the book goes. A book of depth 40 holds 40 moves from the initial position. In detail:

- The positions up to "the next move is move 40" (positions with 21 or more empty squares) are handled by the growing commands and can have links.
- The positions after move 40 (20 empty squares) are stored as **the deepest layer**, with a leaf only. Nothing beyond them is stored.

There is a usual way to combine level and depth. As the table of [chapter 5](05-search.md) shows, a search at level 18 solves exactly from 21 empty squares. If the depth of the book goes "to where exact solving starts", the scores of the last positions of the book are exact values. When Edax makes a book by itself (6.6 below), it chooses this depth from the level.

| Level of the book | Depth chosen automatically (`Depth` of `book info`) |
|---|---|
| 10 or less | 61 − level×2 (level 8 gives 45) |
| 11 to 18 | 40 |
| 19 to 24 | 37 |
| 25 to 29 | 34 |
| 30 to 35 | 31 |
| 36 to 41 | 67 − level (level 36 gives 31) |
| 42 or more | 25 |

The depth can be changed with `book depth n` (this only changes the range that is grown; no position is removed. To remove the positions that fall outside the range, use `book subtree`: [chapter 8](08-book-maintenance.md)). A number written as `book-depth` in `config.ini` sets that depth at every start (`auto` keeps the depth saved in the book file).

## 6.5 Using the book in a game

| Input or setting | Meaning |
|---|---|
| `book on` (setting `book-usage = on`) | Play the moves of the book in a game (the setting of the package) |
| `book off` (`book-usage = off`) | Do not play the moves of the book (think every time) |
| `book randomness n` (setting `book-randomness = n`) | When choosing a move from the book, also accept moves up to n discs worse than the score of the position (0 by default) |

When Edax chooses a move from the book, it picks **one at random among the recorded moves (links and leaf) whose score is at least "the score of the position − randomness"**.

- With randomness 0, only the best moves. When there are several best moves (the same score), one of them is chosen at random, so **the same position does not always give the same move**.
- With randomness 2, moves "up to 2 discs worse than the best" are played as well: the openings vary more (and slightly losing moves are played).
- When a position is not in the book, Edax searches from there on.

When Edax played from the book, the `depth` column of the search table shows `book`.

```
 depth|score|       time   |  nodes (N)  |   N/s    | principal variation
------+-----+--------------+-------------+----------+----------------------
book    -2                                          F4 e3 F6 d3
------+-----+--------------+-------------+----------+----------------------

Edax plays F4
```

`hint` too, when the book is in use (`book on`) and holds the current position, shows **the moves recorded in the book first** (the `book` lines). The other moves are searched and shown after them. To see the result of the search alone, type `book off` and then `hint`.

```
>hint 4
 depth|score|       time   |  nodes (N)  |   N/s    | principal variation
------+-----+--------------+-------------+----------+----------------------
book    +0                                          D3 c4 F4 f6 F3 e6 E7 f7
book    -3                                          F4
    8   -03        0:00.000         18124            G5 c6 C5 c4 B5 e6
    8   -04        0:00.000           711            F3 d3 F4 f6 G5 e6
------+-----+--------------+-------------+----------+----------------------
```

## 6.6 The book files

### Which file is read

At start, Edax reads the file of the setting `book-file` (`data/book.dat` by default).

- If the file does not exist, a new book (the initial position only) is made in memory. Its level is the setting `level`, its depth follows the table of 6.4. The screen shows something like `New book 18 21...` (the second number is "down to how many empty squares the book holds positions": 21 for depth 40). This new book is saved under that file name when Edax ends (even if you do nothing, a small file with the one initial position appears).
- If the file exists but could not be read (damaged, opened by another program, and so on), Edax starts with a new book as well. In this case nothing is saved at the end unless the book was changed. **The file that could not be read is not overwritten**: if something has to be saved under that name later, the original file is first renamed to `book.dat.damaged` (`.damaged.1`, … if that name is taken) and kept.

To switch to another book, do one of these:

- Write `book-file = data/my.dat` in `config.ini` and start again.
- Write `-book-file data/my.dat` at start.
- Type `book load data/my.dat` while running (read the notes below).

### How saving works

| When | Where |
|---|---|
| When you type `book save file` | That file |
| When you type `book save` alone (no name) | The file of `book-file` |
| When Edax ends (`quit`). **Only if the book was changed and not saved yet** | The file of `book-file` |
| During and at the end of the commands that grow or maintain the book | Files named `book-file` plus an extension (table below) |

**Note 1: `book save` alone saves to the file of `book-file`.** The file is named on the screen.

```
>book save
Book saved to data/book.dat
```

This is the behaviour since v4.5.5-nikque.13. Up to v4.5.5-nikque.12, leaving the name out gave the error `Cannot save book to ; existing book was not replaced` and nothing was saved. With an older version, always write the name, as in `book save data/book.dat`.

**Note 2: a book read with `book load` is "saved automatically" to `book-file`, not to the file it was read from.** For example, start with `data/book.dat`, type `book load data/other.dat`, grow the book and quit: the grown book is saved to `data/book.dat` (`other.dat` stays as it was). When this happens, **the previous `data/book.dat` is kept as `data/book.dat.old`** (since v4.5.5-nikque.13; the same mechanism as after `book new`: [6.7](06-book-basics.md)). The same holds when Edax ends after `book import` of a text file.

```
WARNING: data/book.dat holds the book in use before "book load": it is kept as data/book.dat.old
```

- To save to the file that was read, type `book save data/other.dat` yourself. If you always use the same book, naming it with `book-file` is safer.
- **A file named `book-file` plus an extension** (`data/book.dat.dev2`, `.store`, `.mrg`, …: the files that the book commands save by themselves) is taken as the same book going on: after loading it, `book-file` is replaced as before (no `.old` is kept), so that going on from an intermediate file does not leave one more copy of a large book each time. After loading `book.dat.old` or `book.dat.damaged`, the previous file is kept.
- When Edax ends without a change to the book that was read, nothing is saved.
- Up to v4.5.5-nikque.12 the previous file of `book-file` is not kept: it is overwritten.

**Note 3: while saving, the disk needs about as much free space as the book.** Edax first writes everything under another name (`file.tmp.number`) and swaps it with the original file once it is complete. If the save fails on the way, the original file remains.

### Files that the commands write by themselves

The commands that grow or maintain the book save their progress and their result under the name of `book-file` plus an extension (`data/book.dat.dev2` and so on when `book-file` is `data/book.dat`). **These saves do not overwrite the original `book.dat`.**

| Extension | Written by |
|---|---|
| `.store` | `book store`, `book learn` (not written with the setting `book-store-auto-save = off`) |
| `.gam` | `book add` |
| `.dev`, `.dev2`, `.dev3` | `book deviate`, `book deviate2`, `book deviate3` |
| `.enh` | `book enhance` |
| `.fill` | `book fill` |
| `.play` | `book play` |
| `.leaf`, `.leaf2`, `.leaf3`, `.leaf4` | `book leaf-recalculate`, `2`, `3`, `4` |
| `.mrg` | `book merge` (not written with the setting `book-merge-auto-save = off`) |
| `.err` | `book correct` |
| `.dep` | `book deepen` |

- These are ordinary book files. `book load data/book.dat.dev2` reads one, and renamed to `book.dat` it can be used as it is.
- When a growing command has ended normally and you quit Edax without `book save`, the grown book is saved to `book-file` (`data/book.dat`) too. So in normal use **`book.dat` is the latest**, and the files with an extension are "an insurance for when the command was stopped on the way".
- **On Windows, keep the length of `book-file`, folder included, within 240 characters.** If it is too long, the files with an extension cannot be saved ([chapter 13](13-troubleshooting.md)).

## 6.7 Making a new book

```
>book new 18 40
New book 18 21......done>
```

This makes, in memory, a book of level 18 and depth 40 that holds the initial position only (the current book is dropped from memory; if you changed it and did not save it yet, save it first with `book save`). Without the numbers, the level is 21 and the depth 36.

The new book is not a file yet. Save it with `book save data/my.dat`, or quit Edax: it is then saved to `book-file`.

### Saving after book new, and the previous book

After `book new`, when **a save without a file name** (quitting Edax as it is, or typing `book save` alone) replaces the file of `book-file` with the new book, **the previous file is kept under a name with `.old` added** (since v4.5.5-nikque.13).

```
>book new 8 12
New book 8 49......done>
>quit
WARNING: data/book.dat holds the book in use before "book new": it is kept as data/book.dat.old
```

In this example, after the end, `data/book.dat` is the new book (one initial position, 84 bytes) and the previous book (98,976 bytes) remains as `data/book.dat.old`. To go back to the previous book, quit Edax, delete or rename `book.dat`, and rename `book.dat.old` to `book.dat`.

- If `book.dat.old` already exists, the name is `book.dat.old.1` (then `.old.2`, …). A file kept earlier is not overwritten.
- When the previous file of `book-file` held the initial position only (the `book.dat` of the package, for example), or when there was no file, nothing is kept.
- After `book new`, typing `book save data/book.dat` **with the file name** replaces the file as typed (no `.old` is kept).
- The same holds when the new book is grown after `book new` and Edax is then ended (`book.dat` is the grown new book, `book.dat.old` the previous book).
- If, after typing `book new`, you want the previous book after all, type `book load data/book.dat` to read it again: no file changes.
- Edax never deletes an `.old` file by itself. Delete it yourself when it is no longer needed (for a large book it is as large as the book).

The same mechanism works for the saves after `book load` of another file and after `book import` (note 2 of [6.6](06-book-basics.md)).

**Up to v4.5.5-nikque.12, the previous file is not kept.** Quitting Edax right after `book new` overwrites the file of `book-file` with the new empty book (in a test, a `book.dat` of 98,976 bytes was replaced by a file of 84 bytes with one initial position, by `book new` and `quit` alone). With an older version, do not type `book new` while a book you care about is in use.

**So as not to mix up the previous book and a new one**: to start growing a new book, it is safer not to use `book new` but to set `book-file` in `config.ini` to a new name (`data/my.dat`, for example) and then start Edax. When no file of that name exists, Edax makes a new book at the setting `level` and starts with it. Copy the files of the books you care about to another place from time to time.

## 6.8 The commands of this chapter

| Command | Meaning |
|---|---|
| `book show` | Show how the current position is recorded in the book |
| `book info` | Information about the whole book |
| `book stats` | The counts of positions, links and leaves in detail |
| `book on`, `book off` | Use the book in a game or not |
| `book randomness n` | The margin for choosing a move from the book |
| `book depth n` | Change the depth of the book (the range that is grown) |
| `book new level depth` | Make a new empty book |
| `book load file` (`book open`) | Read a book |
| `book save file` | Save the book (without a file name: to the file of `book-file`) |
| `book analyze n` (`book a`) | Look back at the last n moves of the current game with the scores of the book ([3.4](03-playing.md)) |
| `book verbose n` | How much the book commands print (0: nothing, 1: normal, 2: the search table too) |

Next: [7. Growing a book](07-book-learning.md)
