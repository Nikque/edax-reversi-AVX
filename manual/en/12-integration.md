# 12. Using Edax from other programs

[Contents](README.md) | Previous: [11. Solving problems and measuring speed](11-solve-bench.md) | Next: [13. Troubleshooting](13-troubleshooting.md)

Edax is operated with text, but it can also serve as the "brain" (the engine) of another program. There are three ways.

| Way | When it fits |
|---|---|
| **Connecting a GUI program** (12.1) | You want to see the board as a picture and play or analyse with the mouse |
| **Calling the library (libedax)** (12.2) | You write your own program and want to use what Edax can do |
| **Using edax_runner** (12.3) | You want to write the lines to learn in a list and have the book grown automatically |
| **The book analysis commands** (12.4) | You want a GUI program to have Edax count the precision stages, umigame numbers, positions… of the book |

## 12.1 Connecting a GUI program

A GUI program starts Edax behind the scenes and exchanges messages such as "play in this position" or "this move was played" in fixed words (a **protocol**). Edax speaks these protocols:

| Option at start | Protocol | The other side |
|---|---|---|
| (none) | Edax's own words (the commands explained in this manual) | A person, or a program that sends Edax commands directly |
| `-gtp` | GTP (Go Text Protocol) | A GUI that supports GTP |
| `-nboard` | NBoard | NBoard |
| `-xboard` | XBoard (WinBoard) | XBoard or WinBoard with Reversi support |
| `-cassio` | The engine words of Cassio | Cassio (macOS) |
| `-ggs` | Edax itself connects to GGS (the internet game server) | GGS |

Even without an option, Edax switches to a protocol when the first words it receives are one of these: `nboard 1` (NBoard), `xboard` (XBoard), `protocol_version` (GTP), `engine-protocol init` (Cassio).

**The basics of connecting** (settings on the GUI side)

1. In the GUI's "register an engine" screen, name the Edax executable (`wEdax-x86-64.exe` or another one in `bin`).
2. **Set the working folder (the folder in which it runs) to `bin`.** Edax looks for `data/eval.dat` from there ([2.1](02-quick-start.md)). If the GUI cannot set a working folder, write `-eval-file` and `-book-file` in the start options, with names that include the folder.
3. If needed, write the protocol (`-gtp` and so on), the strength (`-l 18`) and the number of threads (`-n 4`) in the start options. The settings of `config.ini` are read when Edax is started by a GUI too.

**What was checked for this manual**: the connection with the GUI programs themselves was not tried. What was checked is only that Edax starts with the three protocols (GTP, NBoard, XBoard) and answers their basic words.

A GTP example (the lines with `>>>` are what was sent, the lines below are Edax's answers):

```
>>> protocol_version
= 2

>>> name
= Edax

>>> boardsize 8
=

>>> play black f5
=

>>> genmove white
= d6
```

The words available in GTP are shown by `list_commands`: `protocol_version`, `name`, `version`, `known_command`, `list_commands`, `quit`, `boardsize`, `clear_board`, `komi`, `play`, `genmove`, `undo`, `time_settings`, `time_left`, `set_game`, `list_games`, `loadsgf`, `reg_genmove`, `showboard`.

An NBoard example:

```
>>> nboard 1
>>> set depth 4
set myname Edax4
>>> go
status Edax is thinking
nodestats 257 0.00
=== d6 2.00 0.0
status Edax is waiting
```

An XBoard example (the answer to `protover 2` lists the supported features):

```
>>> protover 2
feature setboard=1 playother=1 ping=1 draw=0 sigint=0 sigterm=0 analyze=1 myname="Edax 4.5.5" variants="reversi" colors=0 nps=1 memory=1 smp=1 done=1
```

- Under a protocol too, the strength of play is decided by `level` (which NBoard's `set depth` and XBoard's `sd` can set as well) and by the time limits, and the book is used.
- `bin/README.MS-Windows.txt` of the package (the notes of the original Edax, in English) explains how to connect NBoard and WinBoard. It is old: file names and the like differ from the current package.
- GGS (`-ggs`) takes its server from the settings `ggs-host`, `ggs-port`, `ggs-login` and `ggs-password` (not tried for this manual).

## 12.2 The library (libedax)

**libedax** makes the functions of Edax callable from other programs (it has the same functions as the library that lavox made for Edax 4.4). A program written for libedax can use this fixed version of Edax by just replacing the library file.

The libraries in `bin/` of the package:

| File | Environment |
|---|---|
| `libedax-x64.dll` | Windows (64-bit). Runs on any CPU |
| `libedax-x64-v3.dll`, `libedax-x64-v4.dll` | Windows. For CPUs with AVX2, with AVX-512 |
| `libedax-x86-64.so`, `-v3.so`, `-v4.so` | Linux |
| `libedax.universal.dylib` | macOS (for both Apple silicon and Intel) |
| `libedax-arm64-v8a.so`, `libedax-armeabi-v7a.so` | Android (only the build was checked; not run on a device) |

The steps:

1. Put the library and `data/eval.dat` (required) in the folder of your program (`data/book.dat` and `config.ini` if needed).
2. From C or C++, include `src/libedax.h` of the source. From another language, load the library with that language's way of calling foreign functions (for Dart there is a package named libedax4dart).
3. Call `libedax_initialize` first (the settings are read there), start a game with `edax_init`, and call `libedax_terminate` at the end.
4. The functions match the commands of Edax (`edax_play`, `edax_go`, `edax_hint`, `edax_book_deviate`, …).

A short example (shortened from `tests/libedax_example.c` of the source):

```c
#include <stdio.h>
#include "libedax.h"

int main(void)
{
	char *args[] = {"", "-eval-file", "data/eval.dat", "-book-file", "data/book.dat", "-level", "12", "-n-tasks", "2"};
	char moves[] = "f5d6c3";
	static LibedaxHintList hints;
	LibedaxMove last;
	int i;

	libedax_initialize(9, args);  /* the settings are read from edax.ini, config.ini, then these arguments */
	edax_init();                  /* new game */
	edax_play(moves);
	edax_hint(2, &hints);         /* the 2 best moves: hint[1] to hint[n_hints] */
	for (i = 1; i <= hints.n_hints; ++i)
		printf("%c%c %+d\n", 'a' + hints.hint[i].move % 8, '1' + hints.hint[i].move / 8, hints.hint[i].score);
	edax_go();                    /* Edax plays a move */
	edax_get_last_move(&last);    /* squares: A1 = 0, B1 = 1, ..., H8 = 63 */
	libedax_terminate();
	return 0;
}
```

Good to know:

- One process can use one Edax. The functions are called one after the other from one thread (only `edax_stop` and `edax_book_stop_count_bestpath`, which stop what is running, may be called from another thread).
- The book is loaded into memory, and a save replaces the file. If two processes learn into the same book file and save it, only the last save remains: let one process write a book file.
- The settings have the same names and the same syntax as in Edax itself.
- **While a function that changes the book, or one that writes a game file (`edax_base_complete`, `edax_base_correct`), is running, `edax_stop` does not stop the search** (so that the result of a search cut short cannot go into the book or the file).
- The line returned by `edax_get_moves` holds 80 plies at most (moves and passes together).
- On Windows, file names are read as UTF-8 (as the code page of the PC when they are not valid UTF-8). A Japanese name passed in the code page of the PC (CP932) may, in rare cases, be read as another name. Keeping file names to ASCII letters and digits is safe.
- The details (differences from the original libedax, added functions, how to build) are in "libedax: Edax as a library" of `README-NIKQUE.en.md` in the package. The list of functions is `src/libedax.h`; an example that calls every function is `tests/libedax_test.c`.

## 12.3 edax_runner

**edax_runner** (`Nikque/edax_runner`: the edax_runner of sensuikan1973, made to run with the libedax of this fixed version) is a small program that **takes a list of the lines to learn and makes the book learn them from the top down**. Instead of typing commands on the Edax screen, you just write a text file.

How to use it (from the README of edax_runner):

1. Download the package from the Releases of edax_runner and unpack it.
2. Write what is to be learned in `learning_list.txt`.
3. Edit `config.ini` to your liking (same syntax as Edax; [chapter 4](04-settings.md)).
4. If you have a book to grow, put it at `data/book.dat`.
5. Start edax_runner.
6. The lines that have been learned are recorded in `learned_log.txt`.

How to write `learning_list.txt`:

| Purpose | Syntax | Example |
|---|---|---|
| From the given moves, play one game Edax against Edax and learn it (as one line of `book learn`) | `value of book-randomness,moves` | `2,F5F6F7F8` |
| Run `book deviate` from the position after the moves | `[relative-error absolute-error] moves` | `[1 1] F5F6F7F8` |
| Run `book fix` | `fix` | `fix` |
| A comment | `// comment` | `// the tiger opening starts here` |
| End edax_runner | `exit` | `exit` |

- Leaving `2,` out and writing the moves alone is the same as `0,moves`.
- Each time a line (or, when several are learned at the same time, a group) is done, edax_runner saves the book to `data/book.dat` and reads the list again. The list may be edited while it runs.
- A second edax_runner started in the same folder stops without doing anything (since 5.3.0+nikque.10).
- When the setting `book-store-tasks` is not 1 (the default `auto` included), the "one game Edax against Edax" lines are grouped by that number and learned at the same time.
- The rest of this section, and the behaviour of edax_runner itself, were not checked again for this manual (edax_runner is tested separately). For details see the `README.md` of edax_runner.

## 12.4 The book analysis commands for a GUI

These commands let a GUI program leave to Edax the counts that walk the book (since v4.5.5-nikque.19).

- They all run in the background, beside the other commands: during an analysis, Edax still reads `hint`, the moves and the other commands.
- They only read the book. Before a command that may change the book (`book store`, …), Edax stops them.
- Each one is sent with a number `id=…`. The answers are lines that start with `@`, each one written whole, up to its end, at once; written while another output (a warning, …) is half written, one may come in the middle of that line (as in `WARNING: @end id=5 ms=0`, the rest of the warning on the next line). The values are disc differences seen by the side to move at the current position.

| Command | Answer |
|---|---|
| `book moveinfo id=N` | For every legal move: the book value and level, the precision stage, the leaves missing for the next stage, the umigame numbers, the book positions reached after the move |
| `book missing id=N` [`sq=move`] | The lines to the leaves missing for the next stage (with `sq=`, after that move) |
| `book levels id=N` | The positions after the current position, per level (`scope=all`: the whole book) |
| `book bestlines id=N` | The number of lines where both sides play best moves only (or moves losing at most `loss=` discs); with `file=`, the moves to their ends are written |
| `book findline id=N target=position` [`n=`] | The lines from the current position to the `target` position through book positions only (the `n` lines where the value changes least, 1 by default) |
| `stop id=N` | Stop the analysis N (the other analyses and the commands waiting are kept) |

**Precision stage** (`stage=`)

| Stage | Meaning |
|---|---|
| 0 | The position after the move is not in the book |
| 1 | It is, but the lines of best moves of both sides do not reach a solved position |
| 2 | One does, but the range of stage 3 is not complete |
| 3 to 7 | `book deviate 1 1` to `book deviate 5 5` from the position after the move would add nothing |

- The command of reference can be changed to `base=deviate2` or `base=deviate3`, and the ranges with `stages=1/1,2/2,3/3,4/4,5/5`.

**Umigame numbers** (`umi=black:white`)

- For black and for white: the smallest number of line ends to learn to know every line of the condition after the move. At its own turn, the side that learns chooses one move; at the other side's turn, it has to know every move.
- A position reached by different lines is counted once, with what follows it.
- By default the condition is the best moves of both sides; `umi=loss x=… y=…` (loss of one move, total loss) and `umi=range lo=… hi=…` (range of values) can be used too. With a wide range, a number that may not be the smallest gets `exact=0`.

Example (a test book):

```
>book moveinfo id=1
@begin id=1 ply=0 turn=black moves=4 book=1
@move id=1 sq=d3 book=1 value=-2 level=4 lines=505 stage=1 missing=1
...
@move id=1 sq=d3 part=umi umi=1:1
...
@move id=1 sq=d3 part=count positions=1546
...
@end id=1 ms=0
```

- `book moveinfo` answers the moves out of the book, then the precision stages, the umigame numbers (`part=umi`) and the numbers of positions (`part=count`). Near the start of a large book, the numbers of positions take time; a part can be asked alone, as `parts=stage,umi`. `lines=` is the number of unterminated lines written in the book (at once; a transposition is counted by each line).
- `book bestlines` answers one line `@end id=N lines=… ends=… incomplete=… incomplete_ends=… positions=… written=…`: `lines` counts the lines, `ends` the end positions, `incomplete` the ends that do not reach a solved position. The file of `file=` gets one line per end position: the moves of the game up to the current position, then the moves to the end (`which=incomplete`: only the ends that do not reach a solved position).
- The `target=` of `book findline` has 65 characters: the 64 squares from a1 to h8 (`X` black, `O` white, `-` empty, as for `setboard`), then the side to move (`X` or `O`). The lines have the current position, the target and every position between them in the book (they only play on the squares that hold a disc in the target, and pass, `pa`, when the side to move has no move and the other side has one). They come in the order of the sum of the changes of the book value (seen by black) at each move, the smallest first. The answer is one `@line id=N moves=f5d6c3… cost=…` per line, then `@end id=N count=… paths=… positions=…` (`paths`: the number of all the lines, `positions`: the positions looked at). When the target is not in the book, `notinbook=1`; when the current position is not, `startnotinbook=1`; both with `count=0`. `n=` is 1 to 100.
- A wrong word is answered by `@error id=N word=…` and `@end id=N error=1`, a stopped analysis by `@cancel id=N`. A number `id=` that is not an int is answered by `@error word=id` alone, and nothing is analysed or stopped. The words of a command are cut at spaces; a value between `…` is one value, with its spaces (an `id=` in it is not the number). The names (`id=`, …) may be in any case. A `stop id=N` that comes after the analysis N has ended, or before it is taken, is answered by `@cancel id=N state=none` (an analysis that has ended has answered `@end id=N` before; one not taken yet is then done to its end and answers `@end id=N`).
- **What was checked for this manual**: on small test books (made for the tests, up to 19 million positions), the answers were compared with computations written apart, and the precision stages by running `book deviate`. A connection to a GUI program itself was not tried.

Next: [13. Troubleshooting](13-troubleshooting.md)
