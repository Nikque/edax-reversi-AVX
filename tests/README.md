# Regression harness

The harness covers the 17 audit cases, legal-move and exact endgame checks, Cassio, and event handling. Build it with `tests\build-regression.cmd` from a Visual Studio 2022 x64 Developer Command Prompt. Set `EDAX_EVAL_FILE` to the path of your `eval.dat` before running cases that search positions. Run commands from the repository root.

Examples:

```cmd
set EDAX_EVAL_FILE=C:\path\to\eval.dat
tests\regression.exe edx
tests\regression.exe truncated
tests\regression.exe enhance
tests\regression.exe formats
tests\regression.exe gtp
tests\regression.exe core 1
tests\regression.exe core 4
```

Additional case names are listed in `tests/regression.c`. Some cases write small `audit-*` files in the current directory. Run them from a disposable checkout if you want to keep the source directory clean.
