# 15. Glossary

[Contents](README.md) | Previous: [14. Reference](14-reference.md)

The words of this manual and of the Edax screen.

| Word | On the screen | Meaning |
|---|---|---|
| Book | book | Data that holds positions of the opening and the midgame with their scores ([chapter 6](06-book-basics.md)) |
| Confidence (%) | `@73%` and the like | How careful the search is when it cuts branches off on an estimate. At 100% nothing is cut on an estimate ([5.3](05-search.md)) |
| Depth (of a book) | `Depth` | Up to which move the book goes ([6.4](06-book-basics.md)) |
| Depth (of a search) | depth | How many moves ahead the search reads |
| Disc difference | score | The difference between the numbers of discs at the end of the game |
| Empties | `empties` | The number of empty squares. 60 at the start of a game |
| Evaluation data | `eval.dat` | The data used to give midgame positions a score. Edax does not start without it |
| Exact solving | solve, exact | Reading everything to the end of the game to get the exact disc difference. A search at 100% whose depth equals the number of empties ([5.2](05-search.md)) |
| Expand | expand | Search the position after the move of a leaf of the book and add it to the book. The leaf becomes a link |
| First position | initial position | The position from which the current game started. Usually the initial position; `setboard` and loading a game change it |
| Game | game | The record of the moves of a game |
| Hash table | hash table | The table in which the search remembers the results of the positions it examined. Setting `hash-table-size` ([5.6](05-search.md)) |
| Initial position | | The usual starting position of Othello (4 discs in the centre) |
| Intermediate file | `.dev2`, `.store` and so on | A book that a book command saves, on the way or at its end, under the name of `book-file` plus an extension ([6.6](06-book-basics.md)) |
| Leaf | leaf, `<move:score>` | In a position of the book, the one best move among those that are not links. The position after it is not in the book yet |
| Level | level | The number (0 to 60) that sets the strength of the search. The depth and the confidence follow from it and from the number of empties ([5.2](05-search.md)) |
| libedax | | The library that makes Edax callable as functions from other programs ([12.2](12-integration.md)) |
| Link | link, `[move:score]` | In a position of the book, a move whose resulting position is in the book too |
| Logical CPU | | The number of CPUs that the operating system sees (the number of cores, or twice that) |
| Negamax | negamax | The computation that carries the scores of the book from the lower positions up to the higher ones |
| Node | node | One position examined by the search. `nodes` is their number, `N/s` the number per second |
| OBF | `.obf` | The format of problem files ([10.3](10-files.md)) |
| Opening | opening | A known sequence of moves at the start of the game. The `opening` command shows the name of a named opening |
| Pass | `PS`, `pa` | Handing the turn to the opponent because no square can be played |
| Position | position, board | The discs on the board and the side to move. A book keeps the 8 symmetric forms as one |
| Principal variation | principal variation, PV | The expected line when both sides play their best |
| Protocol | protocol | The fixed words used to talk with another program (GTP, NBoard, XBoard, …; [12.1](12-integration.md)) |
| Round | round | One pass "choose → expand → negamax" of a command that expands the book automatically ([7.3](07-book-learning.md)) |
| Score | score | The expected disc difference when both sides play their best to the end. Seen from the side to move (`+` is good) |
| Search | search | Reading the moves ahead |
| Side to move | `Black's turn`, `White's turn` | The side that plays next |
| Temporary file | `book.dat.tmp.number` | The file into which a book is first written whole when it is saved. Once complete, it takes the place of the real file ([6.6](06-book-basics.md)) |
| Thread | task, thread | A unit of work that the CPU can carry on at the same time as others. Setting `n-tasks` |
| todo | `todo` | The number of leaves that a command expanding the book automatically chose for a round |

[Back to the contents](README.md)
