# 7. Growing a book

[Contents](README.md) | Previous: [6. Book basics](06-book-basics.md) | Next: [8. Maintaining a book](08-book-maintenance.md)

This manual calls adding positions to a book "growing" it (Edax's own word is "learning"). There are two main ways.

| Way | Commands | When it fits |
|---|---|---|
| **From games**: the positions of games you played, or of game files, are put into the book | `book store`, `book add`, `book learn` | Taking in your own games, or games of strong players and programs. Entering the openings you want to study |
| **Automatic expansion**: Edax itself chooses "promising leaves" of the book and expands them | `book deviate`, `book deviate2`, `book deviate3`, `book enhance`, `book fill`, `book play` | Making the book wider and deeper along the lines with good scores |

In practice both are combined (a skeleton from games, then automatic expansion).

**Before you start**: read [6.6](06-book-basics.md) (which file is read, where the book is saved) and [6.7](06-book-basics.md) (`book new` and the file of the previous book).

## 7.1 Try a small book first

Before growing a large book, it is a good idea to watch what the commands do on a small book at a low level. The following example ends in about a minute (the output is real; the numbers may differ a little with the PC and from run to run).

Start with a new name for the book file (`data/my.dat` does not exist yet; `-n 2` means 2 threads, `-l 8` level 8).

```
wEdax-x86-64.exe -book-file data/my.dat -n 2 -l 8
```

Make it a book of level 8 and depth 12, and expand it automatically.

```
>book new 8 12
New book 8 49......done>

>book deviate 1 2
Book deviate 1 2:
Book deviate 1 todo
Book deviate...
Book deviate...1/1 done: 1 positions, 1 links
 … (many rounds) …
Book deviate 1 2...finished

>book info
Edax Book 4.5; 2026-10-7 17:10:44;
Positions: 110 (moves = 111 links + 108 leaves);
Level 8 : 110 nodes
Depth: 12
```

The book of 1 position now has 110. Another command, with wider conditions, expands it further.

```
>book deviate2 2 4
Book deviate2 2 4:
Book deviate2 32 todo
Book deviate2...
Book deviate2...1/32 done: 1 positions, 1 links
 …
Book deviate2 2 4...finished

>book info
Positions: 1952 (moves = 1989 links + 1949 leaves);
```

When you end with `quit`, the book is saved to `data/my.dat`. The intermediate files `data/my.dat.dev` and `data/my.dat.dev2` exist too ([6.6](06-book-basics.md)).

## 7.2 Growing from games

### book store: adding the current game

The game played on the board (a sequence entered with `play` will do) is put into the book.

```
>play f5d6c3d3c4
>book store
Searching positions...
Searching positions...2 done
Linking book...
Linking book...3 done
Negamaxing book...done
```

- Among the positions of the game, those within the depth of the book (and the one just beyond, which becomes a position of the deepest layer) are searched **from the end backwards** and added to the book. Positions further on are not added. An unfinished game is fine.
- After adding, the links are made again, negamax is run, and the book is saved to `<book file name>.store`.
- With the setting `auto-store on`, `book store` runs by itself each time a game ends ([3.9](03-playing.md)).

### book add: adding a file of games

```
>book add games.txt
Adding 3 games to book...
Searching positions...
Searching positions...4 done
Adding games...3/3 done: 3 positions, 3 links
```

All the games of a game file (the format follows the extension: `.txt`, `.ggf`, `.sgf`, `.pgn`, `.wtb`, `.edx`; [chapter 10](10-files.md)) are added as `book store` does. The result is saved to `<book file name>.gam`.

In a `.txt` file, each line is one game, the moves written one after the other.

```
f5d6c3d3c4f4c5b3c2
f5f6e6f4e3c5c4e7c6e2
```

### book check: comparing games with the book (the book is not changed)

```
>book check games.txt
Checking 3 games to book...
Positions : 0 missing, 20 good, 4 bad (16.67% bad)
```

It counts the pairs "position, move played there" of the games against the book (within the depth of the book only; only games that start from the usual initial position).

| Word | Meaning |
|---|---|
| `missing` | The position is not in the book |
| `good` | The move played is the best move of the book (or has the same score) |
| `bad` | The move played is recorded in the book but is worse than the best |

### book learn: Edax plays given lines to the end and adds them

```
>book learn learn.txt
Playing games...
Playing games...2 done
Searching positions...
Searching positions...5 done
Linking book...
Linking book...2253 done
Negamaxing book...done
 …
3/3 games learned
```

For each line of the file: "play these moves → Edax plays against itself from there to the end of the game → `book store` of that game". List the opening lines you want to examine, and Edax plays them out and puts them into the book.

How to write the file:

```
f5d6c4
2,f5f6e6f4e3
# this line is a comment
f5f4e3
```

- One game per line, the moves one after the other (spaces are allowed).
- A number and a comma at the head, as in `2,moves`, sets the margin for choosing from the book (`book-randomness`) to that number for this game only.
- Empty lines, lines that start with `#` and lines that contain `//` are skipped.
- The games are played out at the current `level`. In positions that are in the book, the move of the book is played (when `book on`).
- At the end `N/M games learned` is printed (N games learned out of M lines). A line with a move that cannot be played gives a warning and is not learned.
- The result is saved to `<book file name>.store`.

### Learning several games at the same time: book-store-tasks

With the setting `book-store-tasks`, `book store`, `book add` and `book learn` work on **several games (the searches of their positions) at the same time**.

| `book-store-tasks` | Behaviour |
|---|---|
| `auto` (default) | `n-tasks` games are learned at the same time (1 thread per game). The fastest |
| A number n (2 or more) | n games at the same time (`n-tasks ÷ n` threads per game) |
| `1` | The positions are searched one at a time with all the threads (the behaviour of the original Edax; the resulting book is the same too) |

- In the measurement of the README of the package (32 logical CPUs, level 18, 128 games), `auto` was 2.8 to 3.3 times as fast as `1`.
- The book made with `auto` is not exactly the book made with `1` (the move and the score of some leaves may differ by 1 or 2 discs). The difference is of the same size as when the number of search threads is changed. Use `1` only when you need the same book as the original Edax.
- `book learn` plays the games in groups of `book-store-tasks` at the same time. The games of one group are played without using what the others learn (each time a group ends, its games go into the book together).

## 7.3 Automatic expansion: book deviate and its family

### How it works

These commands start **from the position on the board**, follow the links of the book, **choose leaves** that meet a condition **and expand them**. "Expanding" a leaf means: the position after the move of the leaf is searched and added to the book (the leaf becomes a link), and in the original position the next leaf is searched among the remaining moves. Each expansion runs two searches.

One pass (called a "round" here):

1. negamax (bring the scores up to date)
2. choose all the leaves that meet the condition; their number is printed as `todo`
3. expand the chosen leaves, one at a time (or several at the same time)
4. negamax, and save at the chosen interval

The expansions change scores, which makes new leaves meet the condition, so **the rounds are repeated until nothing is added any more**. At the end `...finished` is printed. With wide conditions, there are hundreds of rounds and the time goes to hours and days.

- The starting position is **the current board**. To expand the whole book from the initial position, type `init` first; to dig into one opening, advance with something like `play f5d6c3` and then type the command.
- **If the position on the board is not in the book, the command does nothing** (nothing is printed and the `>` comes back at once). To start from a position that is not in the book, first put the moves up to it into the book with `book store`.
- Nothing is expanded beyond the depth of the book ([6.4](06-book-basics.md)).
- The line `Book deviate2...11/32 done: 11 positions, 11 links` means "11 of the 32 chosen leaves have been expanded; this round added 11 positions and 11 links".

### The conditions of the three commands

Each takes two numbers.

| Command | First number | Second number | Leaves of exactly solved positions |
|---|---|---|---|
| `book deviate X Y` | How many discs "one side" may be off the best move (relative error) | How many discs the score may be away from the score of the starting position (absolute error) | Expanded |
| `book deviate2 X Y` | How many discs one move may lose (either side) | The limit for **the total of discs lost by both sides** from the starting position down to there | **Not expanded** |
| `book deviate3 X Y` | As `deviate2` | As `deviate2` | Expanded |

Without the numbers, all three use `2 4`.

**book deviate X Y** (the method of the original Edax): within one round, choosing and expanding are done twice. The first time follows the lines where "the side to move of the starting position may also choose moves up to X discs worse than the best, and the opponent plays best moves only"; the second time, the other way round. In both, only moves whose score is within "the score of the starting position ± Y" are considered. In short, it expands the lines where "**one side makes small mistakes and the other plays correctly**".

**book deviate2 X Y** (added by this fixed version): each time a move is chosen, "the difference between its score and the best move of that position" is counted as "discs lost". It follows the lines where **one move loses at most X discs and the total from the starting position (Black and White together) is at most Y discs**, and expands the leaves within that range.

An example with `deviate2 5 5`:

| Losses along the line | Within the range? |
|---|---|
| Black 2 discs, White 3 discs (total 5) | Yes |
| 5 discs in one move (total 5) | Yes |
| 6 discs in one move | No (over the limit for one move) |
| Black 3 discs, White 3 discs (total 6) | No (over the limit for the total) |

`deviate2` **does not expand the leaves of exactly solved positions** (positions whose number of empty squares is within the exact range of their level: 21 or fewer at level 18). In such a position the score of the leaf is already exact, so adding what comes after it would not change any score. This avoids expanding the last layers of the book for nothing.

**book deviate3 X Y**: the same conditions as `deviate2`, and the leaves of exactly solved positions are expanded too.

### Which one to use, and choosing the numbers

- **`book deviate2` is recommended in general.** Its range is set by "the total of the mistakes of both sides", so the lines that really occur in games are expanded evenly.
- Larger numbers make the work grow very fast. On the small book above (level 8, depth 12), `deviate 1 2` gave 110 positions and `deviate2 2 4` after it 1,952 (**figures of a small book: they are no forecast for a large one**). Start with small numbers (`1 2`, `2 4`) and widen them while watching the time and the number of positions.
- If the command is stopped on the way, what was saved up to then remains (7.5 below). Typing the same command again goes on from there (leaves already expanded are links now and are not chosen again).

## 7.4 Automatic expansion: the other commands

| Command | Meaning | Intermediate file |
|---|---|---|
| `book enhance X Y` | Uses the lower and upper bounds of the scores to expand "the leaves that could change the score of a position once the error of the search is allowed for (X discs for a midgame search, Y discs for a search that reads to the end but not at 100%)". Repeated until nothing changes. Without the numbers: `2 4` | `.enh` |
| `book fill n` | When n moves from a position of the book lead to another position of the book, the positions and links in between are added. Repeated until nothing is added. `book fill 1` (also when the number is left out) only makes the missing links: no position is added | `.fill` |
| `book play` | Expands the leaf of positions that have no link at all (the tips of the lines), and repeats until no such position remains within the depth of the book. That is, **every line is extended with best moves down to the depth of the book** | `.play` |

These commands come from the original Edax. In this fixed version their behaviour was checked (they build the same book as earlier versions), but there is less experience with them on large books than with the `book deviate` family. Watch what they do on a small book first.

An example of the output on a small book:

```
>book play
Book play...153 todo
Book play...
Book play...1/153 done: 1 positions, 1 links
 …
```

## 7.5 Saving on the way, and stopping

The commands of automatic expansion run for a long time. **When they stop (power cut, restart of the PC, stopped by you), what is lost is what came after the last save.** Two settings decide how often the book is saved.

| Setting | Value in the package | Meaning |
|---|---|---|
| `book-save-interval` | 360 (minutes) | During the expansions, save each time this much time has passed. `0`: no save by time |
| `book-deviate-save-rounds` | 1 | For the `book deviate` family: save each time this many rounds that added positions or links have ended. `0`: only when the command ends |

- The save goes to `<book file name>.dev2` and so on (table of [6.6](06-book-basics.md)). `book.dat` itself is not rewritten while a command runs.
- With a large book, one save takes time and free disk space ([chapter 9](09-large-books.md)). Saving every round when the rounds are short means doing little else than saving: make `book-deviate-save-rounds` larger then (10, for example). When one round takes hours, make `book-save-interval` shorter instead.
- **With `0` and `0` (neither kind of save), nothing is saved until the command ends.** If it stops on the way, everything is lost.

**Stopping**: the book commands accept no input that makes them give up.

- Typing `stop` does not stop them (since v4.5.5-nikque.13; a message says that nothing was stopped. In earlier versions the one position being searched was cut short and its unfinished result went into the book).
- Typing `quit` ends Edax **after the command has ended**.
- To stop at once, stop the Edax program itself (close the window, press Ctrl+C). The book in memory is lost; what remains is the last file saved. Even when Edax is stopped during a save, a half-written file never takes the place of the real one ("How saving works" in [6.6](06-book-basics.md)).

**Going on after a stop**: the last intermediate file (`data/book.dat.dev2`, for example) should be newer than `data/book.dat`. Go on in one of these ways:

- Rename the files: keep `book.dat` under another name, rename `book.dat.dev2` to `book.dat`, then start Edax.
- Start Edax and type `book load data/book.dat.dev2` (in this case too, the automatic save goes to `book-file`: note 2 of [6.6](06-book-basics.md)).

Then type the same command (`book deviate2 5 5`, for example) again.

## 7.6 Expanding several leaves at the same time: book-expand-tasks

Searches at a low level are short, so using many threads for one search does not make it much faster ([5.5](05-search.md)). With the setting `book-expand-tasks`, **several leaves are expanded at the same time** and the CPU is used fully. It applies to `book deviate`, `deviate2`, `deviate3`, `enhance` and `play`.

| `book-expand-tasks` | Behaviour |
|---|---|
| `1` (the value without `config.ini`) | One at a time, each search with all the threads. The behaviour of the original Edax |
| A number n | n leaves at the same time. One search uses `n-tasks ÷ n` threads |
| `auto` (the value in the `config.ini` of the package) | Decided from the level of the book: 2 threads per search up to level 18, 4 up to level 24, 8 above (the number of leaves expanded at the same time is `n-tasks` divided by that). In addition, up to level 18, when a round has at least 32 times `n-tasks` leaves to expand, `n-tasks` searches of 1 thread run at the same time |

- In the measurement of the README of the package (32 logical CPUs, level 18, a book of 656.88 million positions, the first 10,000 leaves of `book deviate3 2 5`): about 805 to 832 seconds with `1`, about 174 seconds with `16` (2 threads each).
- **The resulting book is not the same as with `1`.** The positions added and the moves of the links are the same, but the move and the score of some leaves may differ by 1 or 2 discs. Also, the positions being expanded at the same time do not see each other (when two positions of the same round reach the same new position, the link is not made on the spot; it is made in a later round, or by `book fix`).
- Each of the searches that run at the same time has its own hash table, so more memory is used ([chapter 9](09-large-books.md)).
- A round with a single leaf is the same as expanding one at a time. In a round with few leaves, fewer searches run at the same time.
- For exactly the behaviour of the original Edax, set `book-expand-tasks = 1`.

## 7.7 Examples of growing

**Example 1: digging into an opening you want to study**

```
>play f5d6c3d3c4
>book deviate2 3 6
```

Only what follows the position on the board (the opening called `tiger`) is expanded. If this position is not in the book yet, the command does nothing: type `book store` first to put the moves up to it into the book (in this order: `play f5d6c3d3c4`, `book store`, `book deviate2 3 6`).

**Example 2: adding a file of games, then tidying the whole**

```
>book add mygames.txt
>init
>book deviate2 2 4
```

**Example 3: making Edax play out a list of lines**

```
>book learn openings.txt
```

**Example 4: expanding a little every day (a large book)**

Set the save intervals and the number of simultaneous expansions in `config.ini`, start Edax, type `book deviate2 5 5`, and stop it when time is up. The next day, go on from the last intermediate file (7.5). The cautions for books of hundreds of millions of positions are gathered in [chapter 9](09-large-books.md).

## 7.8 The commands of this chapter

| Command | Meaning |
|---|---|
| `book store` | Add the current game to the book |
| `book add file` | Add the games of a game file to the book |
| `book check file` | Count the moves of the games against the book |
| `book learn file` | Make Edax play out a list of lines against itself and add them to the book |
| `book deviate X Y` | Expand the lines where one side is off by up to X discs and the score is within ±Y |
| `book deviate2 X Y` | Expand the lines with losses of up to X discs per move and Y discs in total (leaves of exactly solved positions excluded) |
| `book deviate3 X Y` | The same (leaves of exactly solved positions included) |
| `book enhance X Y` | Expand the leaves that could change a score once the error of the search is allowed for |
| `book fill n` | Fill in between the positions of the book |
| `book play` | Expand the positions without a link, one move at a time |

Next: [8. Maintaining a book](08-book-maintenance.md)
