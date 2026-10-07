# 11. Solving problems and measuring speed

[Contents](README.md) | Previous: [10. Game and problem files](10-files.md) | Next: [12. Using Edax from other programs](12-integration.md)

## 11.1 Solving a problem set: -solve

The positions of a problem file (OBF; [10.3](10-files.md)) are searched in order and the results are shown as a table. Given as an option at start, Edax ends when the problems are solved.

```
wEdax-x86-64.exe -solve ..\problem\fforum-1-19.obf
```

This is the output for the first 3 problems of the set of the package.

```
 # | depth|score|       time   |  nodes (N)  |   N/s    | principal variation
---+------+-----+--------------+-------------+----------+---------------------
  1|   14   +18        0:00.000         76954            g8 H7 a8 A6 a4 A7 b6
  2|   14   +10        0:00.000         33373            a4 B7 a3 A2 b8 A7 g7
  3|   14   +02        0:00.000        151540            d1 G1 b8 C1 g3 A8 g2
---+------+-----+--------------+-------------+----------+---------------------
three.obf: 261867 nodes in  0:00.000
```

- One line is one problem. The columns are those of the search table ([5.1](05-search.md)). When `depth` equals the number of empty squares and has no `@`, the result is an exact solution (the 3 problems above have 14 empty squares).
- The last line gives the total of the nodes and of the time for all the problems.
- **The strength of the search is the setting `level`** (18 in the package: positions with 21 or fewer empty squares are solved exactly). To solve problems with more empty squares exactly, add `-l 60`.
- Options often used with it: `-n 1` (1 thread: the result and the node count are the same every time), `-l 60`, `-h 24` (a larger hash table), `-q` (no table, the last line only), `-vv` (one line each time the depth increases).

  ```
  wEdax-x86-64.exe -solve ..\problem\fforum-40-59.obf -l 60 -n 8 -h 24
  ```
- When the problem file holds answers (`move:score;`), Edax prints `Erroneous move: D1 expected, with score +2, …` at the right of the line **when the move it found is not the best move of the answers**. This can happen when solving at a level that is not exact (the example below is a problem with 14 empty squares solved at level 6).

  ```
    3|    6   +08        0:00.000         13193            g3 G1 b8 G2          Erroneous move: D1 expected, with score +2, error = …
  ```

From a running Edax, `solve file` does the same.

```
>solve ..\problem\fforum-1-19.obf
```

The problem sets of the package (folder `problem`):

| File | Content |
|---|---|
| `fforum-1-19.obf` | Endgame problems 1 to 19 |
| `fforum-20-39.obf` | Problems 20 to 39 |
| `fforum-40-59.obf` | Problems 40 to 59 |
| `fforum-60-79.obf` | Problems 60 to 79 |

The numbers of empty squares are 14 to 16 in `fforum-1-19`, up to 26 in `fforum-20-39`, 20 to 34 in `fforum-40-59` and up to 36 in `fforum-60-79`. The more empty squares, the longer exact solving takes (solving the last two files with `-l 60` needs a long time; the time was not measured).

## 11.2 Measuring speed: bench

Edax solves endgame positions that are built into it and shows the speed. This gives an idea of the speed of a PC, and lets you compare the kinds of executables (standard, `v3`, `v4`).

```
wEdax-x86-64.exe -bench 2 -n 2
```

```
 # | depth|score|       time   |  nodes (N)  |   N/s    | principal variation
---+------+-----+--------------+-------------+----------+---------------------
  1|   20   +20        0:00.047       3563782   75825149 a4 C8 a8 D7 c7 G2 a2
  2|   20   -02        0:00.078       8479888  108716513 h7 E6 f8 F7 h6 H3 e8
---+------+-----+--------------+-------------+----------+---------------------
2 positions solved: 12043670 nodes in  0:00.125 (96349360 nodes/s).
```

- The n of `-bench n` is the number of positions to solve. From a running Edax: `bench n` (all the positions if n is left out).
- `nodes/s` in the last line is the number of nodes per second.
- With few positions the time is too short and the figures are not stable. To compare, measure several times with the same n and the same `-n` (number of threads). Other running programs make it slower.

## 11.3 Counting positions and lines

These commands count the lines or the positions a few moves ahead of the position on the board. They are for testing the program and are not needed in normal use.

| Command | Meaning |
|---|---|
| `count games d` | The number of lines down to d moves ahead (counted fast, with a hash table) |
| `perft d` | The same count, without the hash table |
| `count positions d` | The number of different positions down to d moves ahead |
| `count shapes d` | The number of shapes of discs (colours not distinguished) down to d moves ahead |
| `estimate n` | Estimate the total number of lines by playing on at random n times |

An example down to 6 moves from the initial position (4 lines after the first move, 12 after the second, …):

```
>count games 6
  ply           moves        passes          wins         draws        losses    mobility        time   speed
   1,               4,            0,            0,            0,            0,    4 -  4,        0:00.000,  …
   2,              12,            0,            0,            0,            0,    3 -  3,        0:00.000,  …
   3,              56,            0,            0,            0,            0,    4 -  5,        0:00.000,  …
   4,             244,            0,            0,            0,            0,    2 -  6,        0:00.000,  …
   5,            1396,            0,            0,            0,            0,    3 -  9,        0:00.000,  …
   6,            8200,            0,            0,            0,            0,    1 - 11,        0:00.000,  …
```

A larger d makes the time grow very fast. The option `-count games 10` at start counts as well.

## 11.4 Other testing commands

The following commands exist too, but they were not run and checked for this manual (they are listed for completeness).

| Command | The built-in help (summary) |
|---|---|
| `microbench` | Measure the speed of the main functions, in CPU cycles |
| `script-to-obf input output` | Convert a problem file of the "script" format to OBF |
| `select-hard input output` | Select the hard problems of a problem set |
| `wtest file` (`-wtest`) | Check the theoretical values of a WTHOR database |
| `weval file`, `edaxify file` | Tests and conversions that use a WTHOR database |
| `seek board`, `mobility n`, `debug-pv move` | For development |

Next: [12. Using Edax from other programs](12-integration.md)
