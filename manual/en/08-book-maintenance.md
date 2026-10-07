# 8. Maintaining a book

[Contents](README.md) | Previous: [7. Growing a book](07-book-learning.md) | Next: [9. Large books](09-large-books.md)

This chapter explains the commands that fix a finished book, merge books, cut a book down, export it, and compute its scores again.

**Every command changes the book in memory.** It reaches a file when you type `book save`, when the command saves by itself (table of [6.6](06-book-basics.md)), and when Edax ends (to `book-file`, if the book was changed). **Nothing can be undone.** Copy the file of a book you care about before maintaining it.

## 8.1 An overview of the maintenance commands

| What you want | Command | Section |
|---|---|---|
| Carry the scores from the bottom to the top again | `book negamax` | 8.2 |
| Make the missing links and repair wrong positions | `book fix` | 8.2 |
| Merge two books into one | `book merge` | 8.3 |
| Make the book shallower, or keep what follows one opening only | `book depth` + `book subtree` | 8.4 |
| Drop the lines where both sides play badly | `book prune` | 8.4 |
| Write to a text file and read back | `book export`, `book import` | 8.5 |
| Compute the scores of the leaves again, after `eval.dat` was replaced for instance | the `book leaf-recalculate` family | 8.6 |
| Verify the values of the exactly solved positions | `book correct` | 8.7 |
| Take games or problems out of the book | `book extract`, `book problem` | 8.8 |

## 8.2 negamax and fix

### book negamax

```
>book negamax
Negamaxing book...done
Sorting book...done>
```

Starting from the initial position and following the links, the score of every position is computed again from the values of the positions below it. Then the moves of each position are sorted, best first.

- The commands that change the book run negamax by themselves, so you seldom need to type it. You do after loading an intermediate file (above all one saved during `book leaf-recalculate`).
- Positions that cannot be reached from the initial position through links (stray positions that no position links to) are not computed again.

### book fix

```
>book fix
Fixing book...
Fixing book...0 done
Linking book...
Searching positions...
Searching positions...6 done
Linking book...2246 done
Negamaxing book...done
Sorting book...done>
```

It checks the whole book and puts it in order. In this order:

1. **Check** (`Fixing book`): positions with wrong content (a recorded move that cannot be played, and the like) are found and rebuilt. `0 done` means that none was found.
2. **Linking** (`Linking book`): for every position, each move that "leads to another position of the book" is made a link. A position whose leaf move became a link gets a new leaf from a search (`Searching positions`).
3. negamax and sorting.

Where `book fix` helps:

- **After growing with `book-expand-tasks` at 2 or more (`auto` included).** Positions expanded at the same time do not see each other, so a link may be missing where "another line reaches the same position" ([7.6](07-book-learning.md)). `book fix` makes it.
- When you start using someone else's book, or a book made by an old version.
- When the book behaves oddly (Edax stops in the middle of the book, for example).

It takes time on a large book ([chapter 9](09-large-books.md)). A position that is rebuilt is searched at the level of the book.

## 8.3 Merging books: book merge

```
>book load data/a.dat
>book merge data/b.dat
Checking book data/b.dat...
Merging book data/b.dat...
Merging book data/b.dat...1 positions added
Linking book...
 …
Negamaxing book...done
Sorting book...done>
Merged book saved to data/my.dat.mrg
```

Another book file (the source) is taken into the current book (the destination).

- **Only the positions that the source alone has** are added. For a position in both, the one of the destination is kept (not overwritten).
- After adding, linking, checking, negamax and sorting are all done (no need to type `book fix` as well).
- With the setting `book-merge-auto-save = on` (default), the result is saved automatically to `<book file name>.mrg`. This save does not overwrite the file of the original book. To use the `.mrg` file, read it with `book load` or rename it.
- When the source file does not exist or is damaged, nothing is taken in and the current book is kept.

  ```
  cannot open data/nofile.dat
  WARNING: Book data/nofile.dat was not merged
  ```
- The source is not loaded into memory (its file is read twice). The memory used is that of the destination book only.

**Cautions**

- The two books should have been grown at **the same level**. With different levels, the reliability of the scores becomes uneven within the merged book, and linking needs more searches.
- When a position is in both, the values of the destination remain. Remember: "the newer, more reliable one is the destination".
- The same book can be grown in parallel on several PCs or in several folders and merged afterwards. In this fixed version, however, one Edax can use many threads fully ([7.2](07-book-learning.md), [7.6](07-book-learning.md)), so there is less need for that.

## 8.4 Cutting a book down

### book depth and book subtree: making it shallower, keeping one part

`book subtree` keeps **only the positions that can be reached from the position on the board by following links** (within the depth of the book) and removes all the others.

**Use 1: making the book shallower** (a book of depth 40 becomes 39, for example)

```
>book depth 39
>init
>book subtree
```

`book depth 39` changes the depth setting; after `init` (back to the initial position), `book subtree` removes the positions beyond depth 39. Stray positions that cannot be reached from the initial position are removed too.

**Use 2: taking out what follows one opening**

```
>play f5d6c3d3
>book subtree
{board:--------------------OX-----XO-----XXO-------O------------------- X; level:8; best: +0 [-2, +2];moves: [F4:+0] <F6:-5>}
Book subtree 13... done
done
 …
>book save data/sub.dat
```

Only the position on the board and what follows it remain. **The positions from the initial position down to it are removed too**, so the resulting book is for studying from that position (`book show` at the initial position then prints nothing). To keep the original book, save with `book save` under **another name** and read the original book again before quitting (see the cautions below).

- When the position on the board is two or more moves beyond the depth of the book, a warning is printed and nothing is done (since v4.5.5-nikque.13; earlier versions emptied the book).

  ```
  WARNING: book subtree: this position is deeper than the book depth; the book is not changed
  ```
- `book subtree` does not make the links again after cutting (since v4.5.5-nikque.12). Type `book fix` before or after if needed.

### book prune: dropping the lines where both sides play badly

```
>book prune
{board:---------------------------OX------XO--------------------------- X; level:8; best: +0 [-2, +2];moves: [D3:+0] [C4:+0] [F5:+0] [E6:+0]}
Book prune 808... done
Book prune 1448... done
done
 …
>book info
Positions: 1269 (moves = 1296 links + 1265 leaves);
```

Only the positions are kept that can be reached from the initial position by lines where "**one side plays any move, the other plays best moves only**" (both cases: Black plays any move, and White plays any move). Positions after both sides played a move that is not best are removed. In the example above, 2,262 positions became 1,269.

Use it to make a book for playing smaller (what remains is the range where you can answer with the best move whatever the opponent plays). On a book grown for study down to "the mistakes of both sides" (expanded with `book deviate2`), most of that is removed.

### Cautions when cutting

**Since v4.5.5-nikque.13, a book cut down with `book subtree` or `book prune` is saved to the file of `book-file` when Edax is then ended** (the book from before the cut is overwritten).

- To keep the book from before the cut, **copy the file first**, or save the cut book with `book save another-name` and then, before quitting, read the original book again with `book load`.
- Changing only the depth setting with `book depth` does not cause a save.

## 8.5 Writing to a text file and reading back

```
>book export data/my.txt
>book import data/my.txt
```

`book export` writes the book to a text file, one position per line.

```
----------X--O----XXOX--XXXOO----OOOO-----O--------------------- X,8,F4,5
```

A line holds the board (64 squares; **the discs of the side to move are `X`, those of the opponent `O`**, and the side-to-move field is always `X`), the level, the move of the leaf and its score. The links are not written (they are made again at the import).

The last line of the file holds the depth of the book (since v4.5.5-nikque.13).

```
% depth 12
```

`book import` reads such a file and **replaces the current book with it**. After reading, it does the linking, the check, negamax and the sorting. The depth is the one of the last line `% depth` (a book of depth 12, exported and imported, gives `Depth: 12`). The imported book is saved to `book-file` when Edax ends; the previous file of `book-file` is kept under a name with `.old` added (note 2 of [6.6](06-book-basics.md)).

- When the file cannot be opened or holds no position at all, the current book is kept (`was not imported; current book retained` is printed). Lines that cannot be read as a position are skipped and the import goes on.
- Because the links are made again, the imported book may differ from the book before the export in some leaves (in a test with a book of 2,246 positions, a second export made after the import differed in 7 lines).
- **A file without the `% depth` line** (a file exported by a version up to v4.5.5-nikque.12, or a file you made yourself): the depth becomes **the depth that just holds the deepest position**. If the book had not been grown down to its chosen depth, it is shallower than the original (a book made with depth 30 that holds only 4 positions down to move 3, exported by an old version and imported, gives `Depth: 3`). After the import, check the depth (`Depth`) with `book info` and, if it is not right, correct it by typing something like `book depth 30`. When what follows `% depth` is not a number from 1 to 60, `WARNING: wrong depth: …` is printed and the result is as without the line.
- **Imported by a version up to v4.5.5-nikque.12**, the depth is one more than that (`Depth: 13` for a book grown down to depth 12). These versions cannot read the last line of a file exported by the new version: they print `WARNING: wrong board: % depth 12` and `WARNING: 1 lines of … hold no position: skipped` and skip that line (all the positions are read).
- For a large book, the text file is much larger than the book file (for the small book above: 170 KB against 99 KB). Use `book save` for everyday saving.

## 8.6 Computing the scores of the leaves again: book leaf-recalculate

The score of a leaf in the book is the value of the search that made that leaf. When `eval.dat` is replaced by another one, the positions added from then on get the values of the new evaluation function, but **the leaves that were there before keep their old values**. The `book leaf-recalculate` family searches the leaves again and replaces their values with those of the current evaluation function. No position is added.

| Command | Range followed | Leaves computed again |
|---|---|---|
| `book leaf-recalculate X Y` | As `book deviate X Y` | The leaves that `book deviate X Y` would choose to expand |
| `book leaf-recalculate2 X Y` | As `book deviate2 X Y` | The leaves that `book deviate2 X Y` would choose to expand |
| `book leaf-recalculate3 X Y` | As `book deviate X Y` | All the leaves of the positions followed |
| `book leaf-recalculate4 X Y` | As `book deviate2 X Y` | All the leaves of the positions followed |

- The meaning of X and Y, and the start from the position on the board, are those of the matching `book deviate` or `book deviate2` ([7.3](07-book-learning.md)).
- **The leaves of exactly solved positions are not computed again** (their value is the same whatever the evaluation function). Positions without a leaf (all moves are links) are not concerned either.
- `3` and `4` also compute again the leaves of the deepest layer (positions with a leaf and no link).
- Each search runs at the level recorded in its position. The number of simultaneous searches follows `book-expand-tasks`.
- The progress and the result are saved to `<book file name>.leaf`, `.leaf2`, `.leaf3`, `.leaf4`. **A file saved on the way is in the state "some leaves are new, negamax not done yet"**: type `book negamax` after loading it. There is no way to resume (typing the command again starts from the beginning).

An example of the output (small book):

```
>book leaf-recalculate4 2 4
Book leaf-recalculate4 2 4:
Book leaf-recalculate4 123 todo
Book leaf-recalculate4...
Book leaf-recalculate4 2 4...finished: 123 leaves, 0 scores changed (0 up, 0 down, largest +0 / -0), 0 moves changed
```

The last line gives the number of leaves computed again, the number of scores that changed (how many went up, how many down, the largest changes), and the number of moves that changed.

### One pass may not be enough: book-leaf-recalculate-rounds

By default the command **follows the range once and ends**. But computing leaves again moves scores, and with them "the range followed": typing the same command again makes leaves that were outside the range the first time come into it.

With the setting `book-leaf-recalculate-rounds = n` (since v4.5.5-nikque.13), "follow → compute again → negamax" is repeated up to n times and **ends with the pass in which no leaf changed**. The default is 1.

- The example of the README of the package (a book of 6.49 million positions, `book leaf-recalculate4 1 1`): repeated one pass at a time, the leaves to compute were 4,066 → 780 → 247 → 485 → 386 and the scores changed 1,419 → 411 → 42 → 46 → 0. With `book-leaf-recalculate-rounds = 10`, it ended after 5 passes.
- The values of a search with several threads vary a little from run to run, so "the pass in which nothing changed" may never come. The command then ends after n passes.
- **Even when `eval.dat` was not replaced, some leaves get another value when computed again** (the search that made the leaf and a search run alone afterwards do not have quite the same conditions). The detailed figures are in the sections on v4.5.5-nikque.12 and 13 of the README.

### An example of the steps after replacing eval.dat

1. Copy the book file.
2. Start Edax with the new `eval.dat` (`-eval-file`, or replace the file).
3. After `init`, type `book leaf-recalculate4 X Y` (X and Y: the numbers you usually give to `book deviate2`).
4. When it has ended, check with `book info` and `book show`, and save with `book save`.

The time is proportional to the number of leaves. In the measurement of the README (a book of 661.62 million positions, level 18, 32 threads), `book leaf-recalculate2 5 5` had 2.75 million leaves to compute and searched about 2,350 of them per minute (the time for the whole was not measured).

## 8.7 Other maintenance

### book correct: verifying the exactly solved positions

```
>book correct
Correcting solved positions...
Correcting solved positions...0 done (0 error found)
 …
```

The leaf of each position within the exact range (positions whose number of empty squares is solved exactly at their level) is searched again and corrected if it differs from what is recorded. Then the same work as `book fix` is done. The number of corrections is shown as `error found`. The progress is saved to `.err`.

On a correctly built book this command should give `0 error found`. Use it when you suspect that faults of an old version, or results of searches that were stopped, went into the book.

**Up to v4.5.5-nikque.12, this command removed the leaves of game-over positions and of pass positions and damaged the scores of the book** (`error found` shows a large number and scores become ±127). This is fixed in v4.5.5-nikque.13. A book damaged by the `book correct` of an old version was repaired by the `book correct` of v4.5.5-nikque.13 (checked on a small test book).

### book deepen: do not use it

The built-in help says "change the level of the book and evaluate everything again", but **it does not work like that** (it searches again the leaves of the positions whose level differs from the level of the book, but the search runs at "the current level of that position" and the level is not rewritten; it is made this way in the original Edax). To raise the level of a book, a new book has to be built at the new level.

### book verbose: how much is printed

`book verbose 0` silences the book commands, `1` (normal) shows the progress, `2` shows the search tables too.

### book feed-hash

It puts the scores of the book, from the position on the board onwards, into the hash table of the search (so that the next search can use the values of the book). Nothing is printed.

## 8.8 Taking things out of the book

### book extract: the best lines as games

```
>book extract data/lines.txt
49 games extracted
```

Starting from the initial position and following, in each position, "the move with the best score" (all of them when several are equal), the lines are written to a game file. The format follows the extension ([chapter 10](10-files.md)).

- **If the output file already exists, the games are added after its end.**
- With a new file, `WARNING: Cannot open file data/lines.txt` is printed, but the file is created (the message means "tried to read an existing file, and there was none").

### book problem: positions as problems

```
>book problem 52 3
Extracting 3 positions at 52 ...
-----------------OOOO----XOXXX----OOO--------------------------- X % bm E2:+1; ba C6:-2;
```

Among the positions of the book with 52 empty squares in which the best move is unique (its score is better than that of the second move), up to 3 are printed on the screen (without the numbers: 24 empty squares and 10 positions; those that come first in the internal order of the book are chosen). A line holds the board, the side to move, `bm` (the best move and its score) and `ba` (the second move and its score).

## 8.9 The commands of this chapter

| Command | Meaning |
|---|---|
| `book negamax` | Compute the scores again and sort the moves |
| `book fix` | Check, linking, negamax, sorting |
| `book merge file` | Take in another book |
| `book depth n` | Change the depth setting |
| `book subtree` | Keep only what follows the position on the board |
| `book prune` | Keep only the lines where one side plays best moves |
| `book export file`, `book import file` | Write to a text file, read back |
| `book leaf-recalculate` (`2`, `3`, `4`) `X Y` | Compute the scores of the leaves again |
| `book correct` | Verify the exactly solved positions |
| `book deepen` | (do not use) |
| `book extract file` | Write the best lines to a game file |
| `book problem empties count` | Show positions as problems |
| `book feed-hash` | Put the values of the book into the hash table |
| `book verbose n` | How much is printed |

Next: [9. Large books](09-large-books.md)
