# eval2 tests and measurements

## v4.5.5-eval2.18

- The solid-disc regression returned the correct bound of +10 for the position with exact score +10.
- The five executables in the Windows distribution gave the expected moves and scores on 20 FForum positions and the expected scores on 1,205 exact-endgame positions. All seven Cassio EOF cases passed for each executable.
- All 279 API checks passed for each of the three DLLs in the Windows distribution.
- The BMI2 final-move kernel matched an independent implementation on 71,168 positions. The evaluation setup, incremental evaluation and paired stability regression cases passed.
- [The distribution CI](https://github.com/Nikque/edax-reversi-AVX/actions/runs/44466216151) built the Linux, macOS, Android and Windows ARM64 binaries.

Playing strength, a production book, and SVE behavior and speed on hardware remain untested.
