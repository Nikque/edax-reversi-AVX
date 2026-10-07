# 3. Commands for playing and analysing

[Contents](README.md) | Previous: [2. Starting and playing a first game](02-quick-start.md) | Next: [4. Settings](04-settings.md)

This chapter explains the commands used to play and to analyse. The book commands (`book …`) are in chapters [6](06-book-basics.md) to [8](08-book-maintenance.md); the commands that handle files of many games (`base …`) are in [chapter 10](10-files.md).

A command is typed after `>` and sent with Enter. Many commands have a short name (`u` for `undo`, and so on). Typing `help` (or `?`) prints the English list built into Edax (up to v4.5.5-nikque.12 some "default" values of that list were those of old versions; the right defaults are in chapters [4](04-settings.md) and [14](14-reference.md)).

## 3.1 Playing moves

| Input | Meaning |
|---|---|
| `f5` (the name of a square) | Play that square. Upper or lower case |
| `ps` (or `pa`, `pass`) | Pass (only when no square can be played) |
| `play f5d6c3d3c4` | Play a sequence of moves. Spaces between the moves are allowed. Passes need not be written: they are inserted where needed |
| `play tiger` | Play the moves of an opening given by its (English) name |
| `go` | Make Edax play one move for the side to move |
| `stop` | Stop Edax while it thinks (it plays the move found so far, and the mode becomes `mode 3`) |

`play` goes on from the current position. To start again from the initial position, type `init` first.

**Opening names**: as in `play tiger`, the (English) name of an opening that Edax knows can be given to `play`. The other way round, `opening` (English) and `ouverture` (French) print the name of the opening of the current position (`?` if the position has no name).

```
>play tiger
>opening
tiger
>ouverture
tigre
```

## 3.2 Who plays: mode

| Input | Black | White |
|---|---|---|
| `mode 0` | You | Edax |
| `mode 1` | Edax | You |
| `mode 2` | Edax | Edax |
| `mode 3` (at start) | You | You |

- `mode 2` plays on from the current position to the end of the game, Edax against Edax.
- With `mode 3` Edax does not play by itself. It suits setting up positions and looking at them with `hint`, or making Edax play one move at a time with `go`.
- `m` is the short name of `mode`. Without a number it is `mode 3`.

## 3.3 Back, forward, again

| Input | Meaning |
|---|---|
| `undo` (`u`) | Take back one move. With `mode 0` or `mode 1`, two moves (yours and Edax's) |
| `redo` (`r`) | Play again a move that was taken back (two moves with `mode 0` or `mode 1`) |
| `init` (`i`) | Set the usual initial position and start a new game |
| `new` (`n`) | Start a new game from the *first position* of the current game. Use it to play again from a position made with `setboard` or from the first position of a loaded game |

## 3.4 Looking at moves: hint and analyze

### hint: the good moves of the current position

```
>hint 3
```

It shows the three best moves of the current position, best first (1 to 60; 1 if left out). How to read the table: [2.5](02-quick-start.md) and [chapter 5](05-search.md). With a number larger than the number of playable moves, all the moves are shown.

`hint` does not change the position. Its strength is the current `level`. When the book is in use and holds the current position, the moves recorded in the book are shown first (lines with `book` in the `depth` column; [6.5](06-book-basics.md)).

### analyze: looking back at a finished game

```
>analyze 6
```

It examines **the last 6 moves** of the current game, one at a time from the end, and compares the move that was played with the best of the other moves (without a number: all the moves). The strength is the current `level`. Many moves take a long time.

```
              N     played        alternative
ply  level   alt. move  score     score   move
---+-------+-----+-----------+--+---------------
 60    0      0    a3    +14
 59    2      1    A2    -14   >   -26     A3
 58    2      0    g2    +14
 57    4      3    H2    -14   >   -24     G2
 56    2      2    b1    +14   >    +5     g2
 55    2      5    A1    -14   >   -16     H2

      | rejections : discs | errors    : discs | error rate |
Black |   0 /   1  :    +0 |   0 /   2 :    +0 |      0.000 |
White |   0 /   1  :    +0 |   0 /   0 :    +0 |          - |
```

| Column | Meaning |
|---|---|
| `ply` | The number of the move |
| `level` | The depth at which that move was examined (as `18@73%`; [chapter 5](05-search.md)) |
| `N alt.` | The number of other moves that could be played |
| `move` and `score` under `played` | The move that was played, and its score (in this column Black's moves are in upper case, White's in lower case) |
| The sign in the middle | `>`: the played move is better; `=`: the same; `<`: another move is better |
| `score` and `move` under `alternative` | The best of the other moves, and its score |

When another move was better, the line ends with one of these:

| Text | Meaning |
|---|---|
| `<- Mistake` | A bad move, confirmed by exact solving |
| `<- Possible mistake` | Judged by a search that read to the end but with a confidence below 100% |
| `<- Edax disagrees` (`strongly` if the difference is 5 discs or more) | Judged by a midgame search (not certain) |

The two lines at the bottom sum up Black and White. `rejections`: how many times "another move is better" was found by a search that was not exact (out of the moves examined that way), and the total of discs; `errors`: the number of bad moves confirmed by exact solving (out of the moves examined exactly), and the total of discs; `error rate`: the discs lost per move examined exactly. For a colour with no move examined exactly, `error rate` is `-`, a dash (up to v4.5.5-nikque.12 it showed something that is not a number, such as `-nan(ind)`).

**Looking back with the book**: `book analyze 6` does the same with the scores of the book instead of a search (only for the positions that are in the book; [chapter 6](06-book-basics.md)).

## 3.5 Setting up a position: setboard

To start from any position, give `setboard` the 64 squares of the board and the side to move.

```
>setboard -------- -------- -------- ---OX--- ---XO--- -------- -------- -------- X
```

- The board is written square by square in the order A1, B1, …, H1, A2, …, H8: 64 squares.
- A black disc is `X` (or `*`, `b`), a white disc `O` (or `w`), an empty square `-` (or `.`). Other characters (spaces, for instance) are skipped, so the board can be written with a space every 8 squares, as above.
- After the 64 squares, write the side to move as one character (`X`: Black to move, `O`: White to move).
- **If fewer than 64 squares are given, the character of the side to move is read as a square.** Edax then prints `WARNING: board_set: bad string input` and chooses the side to move itself (Black if the number of empty squares is even, White if it is odd). Count carefully.

After `setboard`, that position is the *first position* of the game: `new` starts again from it, `init` goes back to the usual initial position.

### Mirroring and rotating the board

These move the current game (with its moves) to a symmetric one.

| Input | Meaning |
|---|---|
| `vmirror` | Swap top and bottom |
| `hmirror` | Swap left and right |
| `rotate 90` (`180`, `270`) | Rotate |
| `symetry n` (`n` from 0 to 15) | Choose one of the 8 symmetries by its number. From 8 up, the side to move is swapped as well |

## 3.6 Saving and loading a game

```
>save mygame.txt
>load mygame.txt
```

`save` (`s`) saves the current game; `load` (`o`, `open`) reads a game and goes to its last position. **The format is chosen by the end of the file name (the extension).**

| Extension | Format | Save | Load |
|---|---|---|---|
| `.txt` | Text: the moves on one line (`F5D6C3D3C4`) | yes | yes |
| `.sgf` | SGF | yes | yes |
| `.ggf` | GGF (the game format of GGS) | yes | yes |
| `.pgn` | PGN | yes | yes |
| `.edx` | Edax's own binary format | yes | yes |
| `.eps`, `.svg` | A picture of the board | yes | no |

- The file name is relative to the current folder (`bin`). A folder can be given, as in `save games/mygame.txt` (create the folder first).
- With an unknown extension, `WARNING: Unknown game format extension: .abc` is printed and nothing is saved.
- When a load fails, the reason is printed and the position does not change (`WARNING: Cannot open file nofile.txt`, `WARNING: Unknown game format extension: .abc`, `WARNING: Illegal move #2: A1`, …). Up to v4.5.5-nikque.12 nothing was printed.
- **A game that started from a set-up position (after `setboard`) should be saved as `.ggf`, `.pgn` or `.edx`.** `.txt` and `.sgf` can be written, but they cannot be read back ([10.1](10-files.md)).
- A game that holds a move that cannot be played is not loaded, and the board stays as it was before the load (with `.ggf`, `.sgf` and `.pgn`, a warning such as `WARNING: error while importing a GGF game` is printed). `.txt` is the exception: a warning is printed and the moves before the unplayable one are loaded. Up to v4.5.5-nikque.12, a move that turns over no disc was put on the board as it was, and the game was loaded ([10.1](10-files.md)).
- The game formats are described in [chapter 10](10-files.md).

## 3.7 Making Edax play a given opening: force

```
>force f5d6c3
```

From then on, as long as the position is on this line (or on a symmetric one), Edax plays the moves of the line when it is its turn. When the line ends, or when the opponent leaves it, Edax thinks and plays as usual. An opening name can be given too (`force tiger`).

- The line is read as starting from the *first position* of the current game.
- `init` sets the forced line back to "the first move is F5" (this is also the state just after starting: the four moves of the initial position are the same by symmetry, so Edax plays F5 as Black's first move without thinking). `setboard` removes the forced line.

## 3.8 Strength and time

| Input | Meaning |
|---|---|
| `level 12` | Set the strength of the search to 12 (0 to 60; [chapter 5](05-search.md)) |
| `game-time 5:00` | Give each side 5 minutes for the game. Edax decides how long to think from the time it has left |
| `move-time 10` | 10 seconds for each move |
| `ponder on` / `ponder off` | Think during the opponent's turn or not (on by default) |

- A time is written as `seconds`, `minutes:seconds` or `hours:minutes:seconds` (`30`, `5:00`, `1:30:00`).
- With `game-time` or `move-time`, Edax thinks by **time**, not by level. If you **gave a level yourself** (the `level` command, or `-l` on the command line), that level caps the reading. Otherwise Edax reads as deep as the time allows (a `level` written in `config.ini` is not a cap).
- Typing `level` goes back to thinking by level instead of by time.
- When `game-time` is changed during a game, the new time applies from the next game (`init`, `new`).
- To set it at start, write `-t 5:00` (the same as `-game-time`) or `-move-time 10` on the command line.

An example started with 20 seconds for the game (part of the output of `options`):

```
	search alloted time: 0:20.000
	search with: fixed time per game
```

## 3.9 Playing many games in a row

These settings decide what happens when a game ends (all are `off` by default; type `auto-start on`, or write `-auto-start on` at start).

| Setting | What happens when a game ends |
|---|---|
| `auto-start on` | A new game starts by itself |
| `auto-swap on` | Edax takes the other colour (`mode 0` ↔ `mode 1`) |
| `auto-store on` | The game is added to the book (as `book store`; [chapter 7](07-book-learning.md)) |
| `auto-quit on` | Edax ends |
| `repeat n` | Play n games (`-repeat n` at start) |

The current values are at the very end of the output of the `options` command (under `Game play`). This is the output after starting with `-auto-store on -auto-swap on` (from top to bottom: `mode`, `auto-start`, `auto-store`, `auto-swap`, `auto-quit`, `repeat`).

```
Game play
	mode: human/human
	start a new game after a game is over: false
	store each played game in the opening book: true
	change computer's side after each game: true
	quit when game is over: false
	repeat 0 games (before exiting)
```

- The `mode:` line is **the value at start** (`-mode`, or `mode` in `config.ini`). Typing `mode 0` and the like while running does not change this line (the current mode shows in whether Edax plays by itself).
- Up to v4.5.5-nikque.12, the lines of `auto-store`, `auto-swap` and `auto-quit` all showed the value of `auto-start` (a display fault only: the behaviour followed the settings).

## 3.10 Changing what is displayed

| Input | Meaning |
|---|---|
| `verbose 0` | Do not show the board and the search table (`You play …` and `Edax plays …` are still shown). Same as `-q` at start |
| `verbose 1` | The usual display (default) |
| `verbose 2` | Also show the progress of the search (one line each time the depth increases). Same as `-vv` at start |
| `noise n` | Do not show the progress lines of the search for depths below n |
| `width n` | Width of the search table in characters (80 by default) |
| `options` | Show all the current settings |
| `version` (`v`) | Show the version |

## 3.11 The commands of this chapter

| Command | Short name | Meaning |
|---|---|---|
| `help` | `?` | The built-in help (English). `help options`, `help commands`, `help book`, `help base`, `help test` show one part |
| `init` | `i` | New game from the initial position |
| `new` | `n` | New game from the current *first position* |
| `mode n` | `m` | Who plays (3.2) |
| (a square), `ps` | | Play a move, pass |
| `play moves` | | Play a sequence of moves |
| `force moves` | | Make Edax play a given opening |
| `go` | | Make Edax play one move |
| `stop` | | Stop thinking |
| `hint n` | | Show the n best moves |
| `undo`, `redo` | `u`, `r` | Back, forward |
| `analyze n` | `a` | Look back at the last n moves |
| `setboard board side` | | Set up a position |
| `vmirror`, `hmirror`, `rotate`, `symetry` | | Move the board to a symmetric one |
| `save file`, `load file` | `s`, `o` (`open`) | Save or load a game |
| `opening`, `ouverture` | | Name of the opening (English, French) |
| `options` | | Show the settings |
| `version` | `v` | Show the version |
| `quit` | `q`, `exit` | End |

Next: [4. Settings](04-settings.md)
