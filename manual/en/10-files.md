# 10. Game and problem files

[Contents](README.md) | Previous: [9. Large books](09-large-books.md) | Next: [11. Solving problems and measuring speed](11-solve-bench.md)

## 10.1 Game formats

Edax chooses the format of a game file by **the end of its name (the extension)**.

| Extension | Format | One game: save and load (`save`, `load`) | Files of many games (`book add`, `base …`) |
|---|---|---|---|
| `.txt` | Text. One game per line | yes | yes |
| `.ggf` | GGF (the format of the internet game server GGS) | yes | yes |
| `.sgf` | SGF | yes | yes |
| `.pgn` | PGN | yes | yes |
| `.wtb` | WTHOR database | no | yes |
| `.edx` | Edax's own binary format | yes | yes |
| `.eps`, `.svg` | A picture of the board (written by `save` only) | save only | no |

### The text format (.txt)

The simplest one, and well suited to writing by hand. One line is one game, the moves written one after the other.

```
F5D6C3D3C4F4C5B3C2
f5f6e6f4e3c5c4e7c6e2
```

- Upper or lower case. Edax writes upper case.
- Passes are not written (they are inserted where needed).
- When a game that does not start from the usual initial position (a game after `setboard`) is saved as `.txt`, the first position is written before the moves, as `board side;`.

  ```
  -------------------O-------OO------XO--------------------------- X;D2C3
  ```

  **But a line of this form cannot be read back** (a `.txt` file is always read as moves from the usual initial position. Loading this line prints the warning `WARNING: game text: … is not a move that can be played here`, reads no move at all and gives a game without moves at the usual initial position; the game in progress is lost). To keep a game that starts from a set-up position, use the formats of the table below.

### Saving a game that starts from a set-up position

The same game (2 moves from a position made with `setboard`) was saved in the 5 formats and read back. The results:

| Format | Result of reading back |
|---|---|
| `.ggf`, `.pgn`, `.edx` | The first position and the moves, as they were |
| `.txt` | **Cannot be read back.** No move is read; the result is a game without moves at the usual initial position (a warning is printed) |
| `.sgf` | **Cannot be read back.** `WARNING: error while importing a SGF game` is printed and nothing is loaded (the board stays as it was before `load`) |

A game that starts from the usual initial position can be read back in every format.

Up to v4.5.5-nikque.12, loading this `.sgf` printed the same warning and then left a wrong board: the moves put on the usual initial position without turning over any disc.

### Examples of the other formats

The same game of 5 moves (`F5D6C3D3C4`), saved.

SGF:

```
(;FF[4]GM[2]AP[edax:4.5.5]
PC[Edax]DT[2026-10-07]
PB[?]PW[?]
SZ[8]AB[e4][d5]AW[d4][e5]PL[B]
RE[Void]
B[f5];W[d6];
B[c3];W[d3];
B[c4];)
```

GGF:

```
(;GM[othello]PC[Edax]PB[?]PW[?]RE[-127.000]BO[8 -------- -------- -------- ---O*--- ---*O--- -------- -------- -------- *]B[F5]W[D6]B[C3]W[D3]B[C4];)
```

- In SGF a pass is written `B[PA]` or `W[PA]` (since v4.5.5-nikque.13 an SGF file with passes can be read back; earlier versions failed to load it).
- **A game file with a move that cannot be played**: when a `.ggf`, `.sgf`, `.pgn` or `.edx` file holds a move that cannot be played (a move that turns over no disc, a move on an occupied square), the game is not loaded. The board and the game stay as they were before `load`. The move that could not be played is shown, as in `WARNING: Illegal move #2: A1` (`#2` is the number of the move, counted from 0; with `.ggf`, `.sgf` and `.pgn`, a warning such as `WARNING: error while importing a GGF game` is printed before it). `.txt` alone is different: the moves before the unplayable one are loaded, and `WARNING: game text: "A1F4" is not a move that can be played here: the rest of the line is not read` is printed (the same when `book add` reads the file).
  - Up to v4.5.5-nikque.12, a move that turns over no disc was put on the board without turning anything over, and the game was loaded (a board that cannot occur in a real game). With an old version, when a warning is printed during a load, do not use that board: start again with `init`. The old versions do not warn either when a `.txt` line is read only in part.
- Up to v4.5.5-nikque.12, loading a `.pgn` of a game that is not finished prints `WARNING: uncomplete game.` (the game is loaded).
- A PGN file is read up to 60 moves.
- A saved game holds 60 moves at most.

## 10.2 Files of many games: the base commands

The commands that start with `base` handle game files (holding many games) as a whole. They have nothing to do with the current game or with the book.

| Command | Meaning |
|---|---|
| `base convert input output` | Change the format (the extensions give the formats) |
| `base unique input output` | Keep one copy of each game |
| `base compare file1 file2` | Show the number of positions that the two files have in common |
| `base check file n` | Examine the endgame of each game by exact solving (from the positions with n or fewer empty squares; 24 if left out) and show the games that hold bad moves |
| `base correct file n` | Examine in the same way and replace the bad moves with the best ones. **The file is rewritten** |
| `base complete file` | Edax plays the unfinished games on to their end. **The file is rewritten** |
| `base problem file n output` | Write the position with n empty squares of each game to a problem file |
| `base tofen file n output` | Write the same positions in the notation called FEN |

The word after `base` may be written in upper case.

Examples:

```
>base convert games.txt games.sgf
WARNING: Cannot open file games.sgf

>base unique games.txt uniq.txt
WARNING: Cannot open file uniq.txt

>base compare games.txt uniq.txt
games.txt : 24 positions - 0 original positions
uniq.txt : 24 positions - 0 original positions
24 common positions

>base check full.txt 12
F5F4E3F6E6C5C6D6G4D3F3C4E7D7B5E2G5B4F7E8C8D8F8C7B8G6H5A6B6G3A5A4C3D2B7A8A7B3C2D1F2H3H4H6F1B2C1E1G7H8G8H7G1H1A1B1H2G2A2A3
Game #0 contains 3 errors
1/1 100.0 % done.
```

**Good to know**

- **If the output file already exists, the games are added after its end** (it is not overwritten). Doing the same thing twice with the same name doubles the games. To make the file anew, delete the old one first or use another name.
- When the output is a new file, `WARNING: Cannot open file …` is printed, as in the examples above. It means "tried to read the existing file before adding to it, and there was none": **the file is created**. The same message appears after `base correct` and `base complete`.
- `base check` and `base correct` solve exactly for every game. With a large n they take very long.
- `base complete` plays on at the setting `level` (since v4.5.5-nikque.13; earlier versions kept the depth of the last search, so they would try to solve to the end and not finish, or play nonsense moves). The search table is printed for every move.
- `base correct` and `base complete` rewrite the file only when it could be read (a file that cannot be read is left as it is).
- While a book or `base` command runs, `stop` does not stop it ([7.5](07-book-learning.md)).

## 10.3 Problem files (OBF)

These files hand Edax positions to solve ([chapter 11](11-solve-bench.md)). The extension is `.obf`. One line is one position.

```
--XXXXX--OOOXX-O-OOOXXOX-OXOXOXXOXXXOXXX--XOXOXX-XXXOOO--OOOOO-- X; G8:+18; H1:+12; H7:+6; A2:+6;
```

- First the board (64 squares; `X` black, `O` white, `-` empty), then the side to move (`X` or `O`), then `;`.
- The `move:score;` items that follow are known answers (the disc difference after that move). They may be left out. When they are there, Edax compares its own answer with them.
- From `%` to the end of the line is a comment and is skipped.

The `problem` folder of the package holds endgame problem sets (4 files such as `fforum-1-19.obf`: problems published in the bulletin of the French Othello federation).

`base problem` and `book problem` ([8.8](08-book-maintenance.md)) write positions in this form.

Next: [11. Solving problems and measuring speed](11-solve-bench.md)
