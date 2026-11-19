# eval2 tests and measurements

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
