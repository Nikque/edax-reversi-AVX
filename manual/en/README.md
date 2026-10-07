# Edax 4.5.5 (fixed version) — User Manual

[日本語](../ja/README.md)

Version described: **v4.5.5-nikque.13** (`Nikque/edax-reversi-AVX`). Some parts do not apply to earlier versions; those places say "since v4.5.5-nikque.N".

Edax is a program that plays Othello (Reversi), analyses games, solves endgames exactly and builds opening data (a *book*). This manual goes step by step, from "start the program and play one game" to "grow and maintain a book of several hundred million positions".

## Reading order

If you are new, read chapters 1, 2 and 3 in order. If you want to build a book, go on with chapters 6, 7 and 8.

| Chapter | Content |
|---|---|
| [1. Introduction](01-introduction.md) | Edax and this fixed version. What the package contains, which executable to use, the words of this manual |
| [2. Starting and playing a first game](02-quick-start.md) | How to start, how to read the screen, playing a game against Edax, quitting |
| [3. Commands for playing and analysing](03-playing.md) | Entering moves, taking back, hints, setting up a position, saving and loading games, analysing a game, time limits |
| [4. Settings](04-settings.md) | `config.ini`, command-line options, changes while running. The list of settings and how to choose them |
| [5. Search basics](05-search.md) | Level, depth, the "%", exact solving, threads, the hash table, how to read the search output |
| [6. Book basics](06-book-basics.md) | What a book is. Positions, links and leaves, reading `book show`, files and how saving works |
| [7. Growing a book](07-book-learning.md) | Adding games (`book store`, `add`, `learn`), automatic expansion (`book deviate` and its family, `enhance`, …), saving while it runs |
| [8. Maintaining a book](08-book-maintenance.md) | Fixing, merging, cutting, exporting, computing scores again |
| [9. Large books](09-large-books.md) | Memory, time, settings and cautions for books of tens to hundreds of millions of positions |
| [10. Game and problem files](10-files.md) | Game formats, the `base` commands, the problem format (OBF) |
| [11. Solving problems and measuring speed](11-solve-bench.md) | `-solve`, `bench`, counting positions |
| [12. Using Edax from other programs](12-integration.md) | Connecting a GUI (GTP, NBoard, XBoard, …), the library (libedax), edax_runner |
| [13. Troubleshooting](13-troubleshooting.md) | Common messages, what they mean and what to do |
| [14. Reference](14-reference.md) | All commands, all settings, file extensions, the table of levels |
| [15. Glossary](15-glossary.md) | The words used in this manual |

## Conventions

- `Text like this` is what the screen shows, or what you type.
- Examples of input start with `>`. The `>` is the prompt that Edax prints when it waits for input; you do not type it.

  ```
  >hint 3
  ```

  This means "type `hint 3` and press Enter".
- Squares are named with a column `A` to `H` and a row `1` to `8` (`A1` is the top left corner, `H8` the bottom right). Upper and lower case are both accepted as input.
- On the board of the screen, `*` is a black disc, `O` a white disc, `-` an empty square and `.` a square where the side to move can play.
- The examples use the name of the Windows executable (`wEdax-x86-64.exe`). On Linux and macOS only the name of the executable differs (see the table in [chapter 1](01-introduction.md)).

## How reliable this manual is

This manual was written from the source of v4.5.5-nikque.13 and from what the Windows version actually did when it was run. The screen examples are real output (long ones are shortened). Where something was not checked by running it, the text says so. Speed and memory figures come with the PC and the book they were measured on. **A figure from a small book is no forecast for a large one.**
