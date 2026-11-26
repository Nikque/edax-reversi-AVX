# eval2 tests and measurements

## v4.5.5-eval2.21

The engine and standalone eval_builder built on Windows x64. Synthetic inputs covered SGF/TXT BOM handling, games after a malformed TXT line, successful OBF solving and a missing OBF input. Playing strength and games with a production book were not measured.

## v4.5.5-eval2.20

- This version and the version before (v4.5.5-eval2.20-pre1 with lazy auto and `pc-slope`) were compared with one thread.
  - The default of this version and the version before with `-sort-eval-shift 1,0,0 -sort-min-depth 7` had the same scores, nodes and principal variations, position by position: 13,261,057 nodes for 20 positions of the midgame (level 16), 2,209,833,428 for 25 positions of 26 empties (level 21).
  - `-pc-slope on -pc-dev on -pc-mob on` and the version before with the options of the order of the moves and the three numbers were the same, position by position (12,168,797 and 1,532,002,685 nodes).
  - The three set to `off` were the same as the default.
  - From the default of the version before to the default of this version, the nodes went from 13,494,056 to 13,261,057 (midgame) and from 2,225,410,591 to 2,209,833,428 (26 empties). The score differed in 0 positions, the best move in none.
- `pc-slope`, `pc-dev` and `pc-mob` set to `on` in `config.ini` gave the same search as `on` on the command line (checked with nikque).

- The Windows programs of the distribution solved fforum-20-39 with no wrong move or score. They searched 95,647,575 positions, the same for the three x64 programs (local PGO builds) and the two x86 programs of the CI.
- The libedax test (290 checks) of the three DLLs of the CI passed with no failure.
- The 7 jobs of the GitHub CI (release binaries) passed. The macOS arm64 job was cancelled the first time, as no machine was given to it, and passed when run again.

Not checked: the effect of the three settings and the strength in this version, the strength in games, 32 threads, a large real book, an Android device, the Linux and macOS versions, the use from a GUI.

---

## v4.5.5-eval2.20-pre1

- The tests of regressions (51/51), libedax (290 checks), book analysis (11/11), resuming, and book and base (557 checks) passed with no failure.
- The test that stops `book learn` at each write and resumes it passed with no failure.
- With the default settings, the number of positions searched on 20 midgame positions (level 14) was that of the previous version (7,799,021).
- The exact solves of fforum-20-39 had no wrong move or score (96,356,604 positions searched).
- `-pc-r`, `-pc-dev`, `-pc-mob`, `-pc-gate`, `-corr`, `-lmr`, `-lazy`, `-sort-eval-shift` and `-sort-min-depth` were each run alone, with no abnormal end.
- fforum-20-39 was solved exactly with the following two combinations, with no wrong move or score and no abnormal end.
  - `-pc-r`, `-pc-end`, `-pc-dev`, `-pc-mob`, `-corr`, `-lmr`, `-sort-eval-shift` and `-sort-min-depth` (one thread)
  - `-lazy`, `-split-probe`, `-lmr` and `-corr` (four threads)
- `stop`, and `?` of XBoard, sent just before a search began were never lost in 90 runs each. A `ping` just after `go` in NBoard was never lost in 60 runs.
- `book deviate`, `deviate2` and `book enhance` made the same books with one and four threads and `book-reproducible on`.
- The midgame speedup was 0.925 of the CPU time (AMD Ryzen 9 9950X, AVX-512 build, one thread, level 21, median of 8 runs). In 27 runs of 1,050 positions, depths, scores, best moves, positions searched and lines were unchanged.
- Loading a book of 80,068 positions 80 times took 0.936 of the CPU time for a book saved in order and 0.629 for one that is not (median of 10 runs). The books saved again were byte for byte the same.
- Scores and best moves were the same before and after the endgame speedup. The positions searched on fforum-20-39 fell from 108,368,547 to 96,356,604.

Not checked: the time settings, `-pc-end` and `-split-probe` alone, playing strength, 32 threads, large production books, Android devices, running the Linux and macOS builds, and use from a GUI.

---

## v4.5.5-eval2.19

- The five executables in the Windows distribution solved 24 synthetic endgame positions scored by an independent implementation with zero move or score errors. All seven Cassio EOF cases passed.
- All 279 API checks passed for each of the three DLLs.
- The x86-64-v4 executable was retested for all seven issues. The checks covered database summaries, an estimate with known error, GTP at game end and a legal pass, `q` in XBoard, GTP, and NBoard, a local GGS mock, and both help entries.
- For trials with totals of 2 and 4, `estimate` returned a mean of 3 and an error of 0.7071068.
- The [distribution CI](https://github.com/Nikque/edax-reversi-AVX/actions/runs/44573928843) built Linux, Intel Mac, Android, and Windows ARM64 binaries. The Linux library tests passed.
- The [fallback build](https://github.com/Nikque/edax-reversi-AVX/actions/runs/44577301101) cross-compiled the Apple Silicon executable and the universal library on Intel Mac from the same fix commit. Both architectures were verified and the Intel library tests passed. Native execution on Apple Silicon was not tested.

Production books, playing strength, communication with a live GGS server, and operation on Android devices remain untested.

---

## v4.5.5-eval2.18

- The solid-disc regression returned the correct bound of +10 for the position with exact score +10.
- The five executables in the Windows distribution gave the expected moves and scores on 20 FForum positions and the expected scores on 1,205 exact-endgame positions. All seven Cassio EOF cases passed for each executable.
- All 279 API checks passed for each of the three DLLs in the Windows distribution.
- The BMI2 final-move kernel matched an independent implementation on 71,168 positions. The evaluation setup, incremental evaluation and paired stability regression cases passed.
- [The distribution CI](https://github.com/Nikque/edax-reversi-AVX/actions/runs/44466216151) built the Linux, macOS, Android and Windows ARM64 binaries.

Playing strength, a production book, and SVE behavior and speed on hardware remain untested.
