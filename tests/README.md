# Regression harness

The harness covers the 17 audit cases, legal-move and exact endgame checks, Cassio, and event handling. Build it with `tests\build-regression.cmd` from a Visual Studio 2022 x64 Developer Command Prompt. Set `EDAX_EVAL_FILE` to the path of your `eval.dat` before running cases that search positions. Run commands from the repository root.

Examples:

```cmd
set EDAX_EVAL_FILE=C:\path\to\eval.dat
tests\regression.exe edx
tests\regression.exe truncated
tests\regression.exe enhance
tests\regression.exe deviatesave
tests\regression.exe formats
tests\regression.exe gtp
tests\regression.exe core 1
tests\regression.exe core 4
```

v4.5.5-nikque.3 adds `missinglink` (a link to a missing position is skipped by negamax and removed by `book fix`), `mergefile` (a truncated merge source leaves the destination unchanged; a complete one is merged), and `linkstore` (links move between in-place and heap storage without loss).

The final audit of v4.5.5-nikque.8 adds `mergetwice` (a second merge does not take the positions of the first one for duplicated positions; a merge that fails only removes what it added), `negamaxloop` (a link of a damaged book that leads back to its own position does not make the negamax with threads endless) and `learnline` (the first moves of a game to learn are copied without their spaces: a long line is not cut).

Additional case names are listed in `tests/regression.c`. Some cases write small `audit-*` files in the current directory. Run them from a disposable checkout if you want to keep the source directory clean.
