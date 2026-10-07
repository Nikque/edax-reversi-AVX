# 5. Search basics

[Contents](README.md) | Previous: [4. Settings](04-settings.md) | Next: [6. Book basics](06-book-basics.md)

Whether it chooses a move, gives a hint or grows a book, Edax *searches* (reads ahead). This chapter explains the **level**, which sets the strength of the search, the **depth** and the **%** that appear on the screen, the **threads** and the **hash table**. For people who grow a book, it is also the chapter that explains what "the level of a book" means.

## 5.1 Reading the search table

`hint` and `go` print a table like this:

```
 depth|score|       time   |  nodes (N)  |   N/s    | principal variation
------+-----+--------------+-------------+----------+----------------------
18@73%  +00        0:00.110      15578834  141625764 c3 D3 c4 F4 c5 B3 c2 E3
18@73%  -01        0:00.000         15137            c4 F4 f6 G5 f3 D3 c3 G6
------+-----+--------------+-------------+----------+----------------------
```

| Column | Meaning |
|---|---|
| `depth` | How deep the search read (how many moves ahead). `18@73%`: "18 moves ahead, read with a confidence of 73%". Without `@`, 100% (5.3) |
| `score` | The score: the expected final disc difference, seen from the side to move. `+` is good for the side to move |
| `time` | Time taken (minutes:seconds) |
| `nodes (N)` | Number of positions examined (nodes) |
| `N/s` | Nodes per second (the speed). Empty when the time is too short |
| `principal variation` | The expected line of play with best moves by both sides. **Black's moves are in lower case, White's in upper case** |

A mark can stand before the number of `score`:

| Mark | Meaning |
|---|---|
| (none) | The value found at that depth |
| `<` | Only known to be "this value or less" |
| `>` | Only known to be "this value or more" |
| `?` | Not certain, for example because the search was stopped (with `stop`) |

## 5.2 What a level is

The **level** is a number from 0 to 60 that sets the strength of the search. Once the level is given, **how deep and with which confidence Edax reads** follows from the number of empty squares of the position.

The idea:

- **Opening and midgame** (many empties): the game cannot be read to the end, so Edax reads **as many moves ahead as the level** (18 moves at level 18). The positions at the end of the reading are scored with the evaluation data (`eval.dat`). The score is therefore an estimate.
- **Endgame** (few empties): Edax reads **everything to the end of the game**. With a confidence of 100% the score is the exact disc difference. This is **exact solving**.
- In between: Edax reads to the end of the game, but faster, with a somewhat lower confidence (the "%" below).

At level 18 (the setting of the package):

| Empty squares | How it reads | Example of display |
|---|---|---|
| 28 or more | 18 moves ahead, 73% | `18@73%` |
| 25 to 27 | To the end, 87% | `26@87%` |
| 22 to 24 | To the end, 98% | `23@98%` |
| 21 or fewer | To the end, 100% (**exact solving**) | `20` |

So at level 18 Edax always knows the exact result once 21 or fewer squares are empty.

### The table of levels

For each level: the number of empties from which the result is exact, and the depth read in the midgame (the table is made from the formula of the source).

| Level | Midgame reading (from that number of empties up) | Read to the end, but below 100% | Exact from |
|---|---|---|---|
| 1 to 10 | level moves ahead, 100% (when there are more than level×2 empties) | none | level×2 or fewer |
| 11, 12 | level moves ahead, 73% (25 empties or more) | 22–24: 98% | 21 or fewer |
| 13 to 18 | level moves ahead, 73% (28 or more) | 25–27: 87%, 22–24: 98% | 21 or fewer |
| 19 to 21 | level moves ahead, 73% (31 or more) | 28–30: 87%, 25–27: 98% | 24 or fewer |
| 22 to 24 | level moves ahead, 73% (34 or more) | 31–33: 73%, 28–30: 95%, 25–27: 99% | 24 or fewer |
| 25 to 27 | level moves ahead, 73% (34 or more) | 31–33: 87%, 28–30: 98% | 27 or fewer |
| 28, 29 | level moves ahead, 73% (37 or more) | 34–36: 73%, 31–33: 95%, 28–30: 99% | 27 or fewer |
| 30, 31 | level moves ahead, 73% (37 or more) | 34–36: 87%, 31–33: 98% | 30 or fewer |
| 32, 33 | level moves ahead, 73% (40 or more) | 37–39: 73%, 34–36: 95%, 31–33: 99% | 30 or fewer |
| 34, 35 | level moves ahead, 73% (40 or more) | 37–39: 87%, 34–36: 98%, 31–33: 99% | 30 or fewer |
| 36 to 59 | level moves ahead, 73% (level+10 empties or more) | rising every 3 squares: 73%, 87%, 95%, 98%, 99% | level−6 or fewer |
| 60 | (none) | none | always (exact from the first move) |

- Level 0 does not read ahead. Every playable move gets the same score, and one of them is chosen at random (it changes from one time to the next).
- Levels 1 to 10 read level moves ahead without lowering the confidence. From level 11 up, the midgame is read at 73%.
- **Time grows fast with the level.** How long a search takes depends very much on the position and on the PC. On the PC used for the checks (Ryzen 9 9950X), a move in the opening took about 0.1 second at level 18 with 32 threads, and about 10 seconds at level 40 with 2 threads. Later in the game, when the position enters the "read to the end" ranges (middle and right columns of the table), a high level takes longer still. Raise the level little by little and see.
- Level 60 tries to read every position to the end of the game. With many empties this takes a very long time (possibly longer than you can wait).

## 5.3 What the "%" (confidence) is

To be faster, Edax cuts off, after a shallow reading only, the branches that it expects not to change the result even if read deeply. The **%** tells how strict that expectation is.

| Display | Meaning |
|---|---|
| `@73%` | The boldest cutting (fast, but something can be overlooked) |
| `@87%`, `@95%`, `@98%`, `@99%` | More and more careful |
| (no `@` = 100%) | No cutting on expectation |

- "73%" does not mean "wrong 27 times out of 100". It tells how strong the expectation is for each single cut, not the probability that the result of the whole search is right.
- Only a search that **reads to the end and at 100%** gives an exact result (exact solving). A value such as `25@87%`, where the depth equals the number of empties but an `@` is shown, is "probably this disc difference".

## 5.4 Thinking by time

With `game-time` (time for the whole game) or `move-time` (time for one move), Edax decides how deep to read from the time, not from the level ([3.8](03-playing.md)). It reads one move deeper at a time, starting shallow, and when the time is up it uses what it has so far. The display then looks like this (20 seconds for the game, a position with 58 empties):

```
32@73%  -02        0:01.250      60118065   48094452 c3 D3 c4 F4 f6 F3 e6 E7
```

## 5.5 Threads (n-tasks)

`n-tasks` is the number of threads of the search. The package is set to `auto` (the number of logical CPUs of the PC).

- More threads make the search faster, **but not in proportion**. The shorter a search, the less it can use many threads. In the measurement of the README of the package (89 positions at level 18, Ryzen 9 9950X with 32 logical CPUs), 32 threads were 2.5 times as fast as one thread (4 threads: 2.1 times). The deeper the search (a high level, exact solving with many empties), the more threads help.
- When you do other things on the PC, a smaller `n-tasks` keeps it more responsive.
- For growing a book there is another way to use many threads: searching several positions at the same time instead of giving all the threads to one search (chapters [7](07-book-learning.md) and [9](09-large-books.md)).

## 5.6 The hash table (hash-table-size)

Edax remembers the results of the positions it examined in a **hash table** and uses them again when the same position comes back. With `hash-table-size = n` the main table has 2 to the power n entries, and the memory used is about 27×2^n bytes in all.

| n | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 28 | 30 |
|---|---|---|---|---|---|---|---|---|---|
| Memory | 57 MB | 113 MB | 226 MB | 453 MB | 906 MB | 1.8 GB | 3.6 GB | 7.2 GB | 29 GB |

- The range is 10 to 30 (10 to 25 for the 32-bit versions).
- **`auto` (the setting of the package) is recommended.** It chooses from the number of threads: 21 for 1 to 3 threads, 22 for 4 to 15, 23 for 16 to 63 (at most 25, and at most 1/32 of the memory of the PC).
- A large table can help long searches (a high level, exact solving with many empties). It does not make short searches faster (measured for the README: with 32 threads at levels 18 and 24, 23 was slightly faster than 21 and larger values were not faster).
- Typing something like `hash-table-size 24` while Edax runs rebuilds the tables at that size (since v4.5.5-nikque.13; earlier versions only used the value at start).

## 5.7 Are the results the same every time?

- With **one thread** (`n-tasks = 1`), the same position, the same level and the same `hash-table-size`, Edax gives the same result every time (score, move, principal variation, node count). The kind of executable (standard, `v3`, `v4`) does not change the result.
- With **several threads**, the result can change a little from one run to the next (the threads do not run in the same order every time). Mostly, what changes is which move is chosen among moves with the same score, and scores of searches below 100% by one or two discs. For exact solving (100%), the check of the README of the package (an endgame problem set solved twice with 32 threads) found the same scores; only the choice among moves of equal score differed.
- Changing `hash-table-size` can slightly change the results of searches below 100%.

To reproduce a result exactly (for a check), set `n-tasks = 1` and give `hash-table-size` as a number.

## 5.8 Things to know about scores

- A score is a **disc difference**. `+4`: "the side to move should win by 4 discs". A draw is `+0`.
- A midgame score changes with the level. Usually deeper is more accurate, but whether the `+2` of level 18 or the `+0` of level 24 is right cannot be known without reading to the end.
- For an accurate score, go on to a number of empties where the search is exact, or raise the level.

Next: [6. Book basics](06-book-basics.md)
