# eval2 tests and measurements

## v4.5.5-eval2.20-pre1

- The tests of regressions (51/51), libedax (290 checks), book analysis (11/11), resuming, and book and base (557 checks) passed with no failure.
- The test that stops `book learn` at each write and resumes it passed with no failure.
- With the default settings, the number of positions searched on 20 midgame positions (level 14) was that of the previous version (7,799,021).
- The exact solves of fforum-20-39 had no wrong move or score (96,356,604 positions searched).
- The new search settings were run one by one and all together: no wrong exact solve and no abnormal end.
- `stop`, and `?` of XBoard, sent just before a search began were never lost in 90 runs each. A `ping` just after `go` in NBoard was never lost in 60 runs.
- `book deviate`, `deviate2` and `book enhance` made the same books with one and four threads and `book-reproducible on`.
- The midgame speedup was 0.925 of the CPU time (AMD Ryzen 9 9950X, AVX-512 build, one thread, level 21, median of 8 runs). In 27 runs of 1,050 positions, depths, scores, best moves, positions searched and lines were unchanged.
- Loading a book of 80,068 positions 80 times took 0.936 of the CPU time for a book saved in order and 0.629 for one that is not (median of 10 runs). The books saved again were byte for byte the same.
- Scores and best moves were the same before and after the endgame speedup. The positions searched on fforum-20-39 fell from 108,368,547 to 96,356,604.

Not checked: playing strength, 32 threads, large production books, Android devices, running the Linux and macOS builds, and use from a GUI.

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
