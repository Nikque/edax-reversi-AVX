# 9. Large books

[Contents](README.md) | Previous: [8. Maintaining a book](08-book-maintenance.md) | Next: [10. Game and problem files](10-files.md)

This fixed version is made so that a book of tens to hundreds of millions of positions can be grown and maintained on one PC. This chapter gathers the orders of magnitude for memory, disk and time, the settings, and the cautions for such books.

The figures are measurements recorded in the README of the package. **Each of them is the value of one PC and one book, not a forecast for another environment.** Unless stated otherwise, they were measured on a Ryzen 9 9950X (32 logical CPUs) under Windows, with a book of level 18 and depth 40 (about 660 million positions, a file of about 29 GB).

## 9.1 Memory

| What | Size |
|---|---|
| The book (in memory) | About 50 bytes per position, links included. 30.6 GiB for 657 million positions |
| The book file | About 44 bytes per position (29.16 GB for 661.62 million positions) |
| The hash table of the search | The table of [5.6](05-search.md). 226 MB with `auto` and 32 threads |
| The hash tables of the simultaneous searches (`book-expand-tasks`, `book-store-tasks`) | One set per search. 57 MB each for the 2-thread searches of `auto` (about 0.9 GB for 16 of them). 14 MB each for 1-thread searches up to level 18 |
| The `book deviate` family, only while it chooses positions | 1 byte per position (about 0.9 GB for 657 million positions) |

- With the book of about 29 GB, the peak memory of `book deviate2 5 5` was 33.4 GB and that of `book subtree` 31.8 GB. The rule of thumb from these two values: **the size of the book file + 4 to 5 GB**.
- **`book load` uses the memory of two books while it reads** (so that the current book can be kept if the load fails). For a large book, do not use `book load`: **name it with `book-file` and let Edax read it at start** (reading at start, and `book merge`, do not need this extra memory).
- When memory runs short: if the simultaneous searches cannot be prepared, a warning is printed and the work goes on with fewer of them (`not enough memory to search the positions at the same time` and the like). When no position can be added to the book any more, the growing command stops.
- The 32-bit version cannot read a book of about 89.47 million positions or more (it prints an error and keeps the current book). Use a 64-bit version for a large book.
- A book can hold at most 4,294,967,295 positions. A book with more than 2,147,483,647 positions cannot be read by the versions before v4.5.5-nikque.4, nor by the original Edax.

## 9.2 Disk

- **Each save needs free space of the size of the book.** Edax first writes everything under another name and then swaps it with the original file ([6.6](06-book-basics.md)). For a book of 29 GB, 29 GB must be free during the save.
- **Every intermediate file is a copy of the whole book.** As `book.dat.dev2`, `book.dat.store`, `book.dat.mrg`, … appear next to `book.dat`, each of them is as large as the book. Delete those you no longer need yourself (Edax does not).
- Rule of thumb: free space of 3 times the book file or more (the book + one intermediate file + the temporary file of a save).
- A temporary file left by a forced stop (`book.dat.tmp.number`) is deleted by Edax at the next save, on Windows.
- **On Windows, keep the length of `book-file`, folder included, within 240 characters** ([chapter 13](13-troubleshooting.md)).
- Time of a save: 18 to 28 seconds for the book of about 29 GB (it depends on the speed of the disk).

## 9.3 Examples of times

| Operation | Time (book of about 660 million positions) | Version of the record |
|---|---|---|
| Reading at start | 28 to 49 seconds | v4.5.5-nikque.3 to 12 |
| Saving | 18 to 28 seconds | v4.5.5-nikque.3 |
| One negamax | 18 to 34 seconds | v4.5.5-nikque.10, 12 |
| Choosing what to expand, for `book deviate2` and `deviate3` | 21 to 23 seconds | v4.5.5-nikque.3 |
| A whole `book fix` (from reading to saving) | 222 to 242 seconds | v4.5.5-nikque.10 |
| `book merge` (taking in a book of 6.49 million positions; the whole) | 218 to 224 seconds | v4.5.5-nikque.10 |
| `book merge` (two books of 657 million positions; 1.736 million positions added) | 263 seconds, peak memory 37.0 GiB | v4.5.5-nikque.3 |
| `book depth 39` + `book subtree` (depth 40 → 39; from reading to the end) | 177 seconds | v4.5.5-nikque.12 |

- The same operation varied by 10 to 20% with the day of the measurement and the other programs that were running.
- With v4.5.5-nikque.2 (the original Edax with bug fixes only), the same book took 446 seconds to read, 657 seconds to save, 937 seconds for a negamax, and four and a half hours to merge two books of 600 million positions (section on v4.5.5-nikque.3 of the README).

**The time of growing is almost all search time.** `book deviate2 5 5` on the book of about 660 million positions had 2.75 million leaves to expand in one round (`todo`). With 32 threads and `book-expand-tasks = auto`, 32 to 40 leaves were expanded per second (it varied with the time of day). If that speed held, one round would take about 19 to 24 hours (a computation, not a value measured over a whole round).

## 9.4 Settings

The `config.ini` of the package has values chosen with the growing of a large book in mind. An example for growing a book of level 18 on a PC with 32 logical CPUs:

```
level = 18
n-tasks = 32
hash-table-size = auto
book-file = data/book.dat
book-expand-tasks = auto
book-store-tasks = auto
book-save-interval = 360
book-deviate-save-rounds = 1
book-merge-auto-save = on
```

| Setting | How to think about it |
|---|---|
| `n-tasks` | `auto` to use all the logical CPUs. **Lower it if you do other work as well** (half of them, for example). Negamax, the choice of what to expand and the linking run in parallel on this number of threads too |
| `book-expand-tasks` | `auto`. Up to level 18 this gives "`n-tasks ÷ 2` searches of 2 threads", and in rounds with many leaves (32 times `n-tasks` or more) "`n-tasks` searches of 1 thread". At levels 19 to 24 the searches have 4 threads each, above that 8 each. At high levels, set a number and compare the speeds (the README suggests starting from 4 or 8 with 32 threads) |
| `book-store-tasks` | `auto` (`n-tasks` games learned at the same time) |
| `hash-table-size` | `auto`. A larger table does not make short searches faster |
| `book-save-interval` | Decide it knowing that one save takes about 30 seconds and writes a file as large as the book. The package has 360 minutes (6 hours) |
| `book-deviate-save-rounds` | `1` if one round is long (hours). Writing 29 GB every round when a round is short (minutes) is a waste: make it larger then |

**Settings that give the behaviour of earlier versions and of the original Edax**: `book-expand-tasks = 1`, `book-store-tasks = 1`. The book made with `auto` differs from the book made with `1` in the move and the score of some leaves ([7.2](07-book-learning.md), [7.6](07-book-learning.md); the README has measured counts of the positions that differed).

## 9.5 Cautions for the steps on a large book

1. **Copy the file before maintenance.** `book subtree`, `book prune`, `book merge`, `book correct` and `book leaf-recalculate` change the book a lot. The save at the end overwrites `book-file` ([8.4](08-book-maintenance.md)).
2. **Do not type `book new` while a large book is in use.** The book in memory is dropped, and getting it back costs the time of reading the file again. If Edax is then ended as it is, the file of `book-file` becomes the new empty book and the previous file is kept under a name with `.old` added (up to v4.5.5-nikque.12 it is not kept but overwritten; [6.7](06-book-basics.md)).
3. **`book export` (the text file) gives a very large file for a large book.** Use the files of `book save` for everyday saving and for passing a book on.
4. **When you stop, remember the save interval.** Stopping a growing command loses what came after the last save ([7.5](07-book-learning.md)). `quit` waits until the command ends.
5. **Before quitting, check which file is the latest.** If you `quit` after a growing command has ended, `book.dat` is the latest. If you stopped it on the way, `book.dat.dev2` or the like should be newer. Check the dates and sizes before renaming anything.
6. **Do not open and save the same book file with two Edax at the same time.** The one that saves later overwrites the other. To grow in parallel, use separate files and bring them together with `book merge` afterwards.
7. **Try the steps on a small book before doing them on a large one.** Checking the order of the commands, the file names and where things are saved on a book that takes seconds avoids failures.

## 9.6 The size of a book, its depth and its level

- A higher level makes adding one position take longer. In the measurement of the README (level 18, `book deviate2 5 5` on the book of about 660 million positions), expanding one leaf took a search of about 33 million nodes.
- One more move of depth adds a great many positions. In the example of the README, reducing the depth of the same book from 40 to 39 turned 661.62 million positions into 524.08 million (the deepest layer alone held 137.54 million positions).
- Raising a number of `book deviate2 X Y` by one multiplies the work several times. Always try small numbers first.

Next: [10. Game and problem files](10-files.md)
