# eval2 テストと実測の記録

## v4.5.5-eval2.21

Windows x64 で本体と単体 eval_builder を組み立てました。合成入力で、SGF・TXT の BOM、壊れた TXT 行に続く局の読み込み、OBF の正常終了・入力ファイルエラーを確認しました。実 book を使った対局と棋力の比較は未実施です。

2026-11-26 に差し替えた Windows の配布版（PGO）の確かめです。

- 実行ファイル 5 本（`wEdax-x86-64-v4`・`-v3`・`wEdax-x86-64`・`wEdax-x86-sse`・`wEdax-x86`）は、`src/NMakefile` の `vc-pgo-*` でビルドしました。プロファイルは `-n 1 -l 60 -solve problem\fforum-20-39.obf` と `-n 1 -l 18 -auto-store on -auto-start on -repeat 4 -auto-quit on -mode 2` で取りました。
- DLL 3 本（`libedax-x64`・`-v3`・`-v4`）は、`/LTCG:PGI` でビルドし、`tests/libedax_test.c` の `libedax_test <dll> bench 20 1` と API のテスト全体を流してから、`/LTCG:PGO` でビルドしました。
- fforum-20-39 の完全読み（1 スレッド、`-l 60`）：PGO の 5 本と、差し替え前の CI の `wEdax-x86-64.exe` は、どれも着手とスコアの誤りが 0 でした。探索した局面の数は 95,647,576 で、6 本でそろいました。
- v4.5.5-eval2.20 の記録（配布版の Windows 版で 95,647,575）とは 1 局面違います。違いの原因は確かめていません。
- DLL 3 本の libedax の API のテスト（290 項目）は、失敗 0 でした。
- 速さは測っていません。ほかの処理と CPU を分け合っている中だったので、時間の数字は比べていません。
- 差し替えていない 15 本（`wEdax-arm64.exe`、Linux、macOS、Android）は、この確かめに含めていません。

## v4.5.5-eval2.20

- この版と前の版（v4.5.5-eval2.20-pre1 に lazy auto と `pc-slope` を足した物）を、1 スレッドで比べました。
  - この版の既定は、前の版に `-sort-eval-shift 1,0,0 -sort-min-depth 7` を付けた物と、評価値・探索した局面の数・読み筋が局面ごとにすべて同じでした。中盤 20 局面（level 16）で 13,261,057 局面、空き 26 の 25 局面（level 21）で 2,209,833,428 局面です。
  - `-pc-slope on -pc-dev on -pc-mob on` は、前の版に手を読む順番の設定と 3 つの数値を付けた物と、局面ごとにすべて同じでした（12,168,797 局面と 1,532,002,685 局面）。
  - `off` を 3 つ書いた物は、既定と同じでした。
  - 前の版の既定とこの版の既定では、探索した局面の数が中盤 20 局面で 13,494,056 → 13,261,057、空き 26 の 25 局面で 2,225,410,591 → 2,209,833,428 でした。評価値が違ったのは 0 局面、最善手が違ったのは 0 局面です。
- `config.ini` の `pc-slope`・`pc-dev`・`pc-mob` を `on` にした物は、コマンド行で `on` にした物と同じ探索でした（nikque で確かめました）。

- 配布物の Windows 版は、fforum-20-39 の完全読みで、着手とスコアの誤りが 0 でした。探索した局面の数は 95,647,575 で、x64 の 3 種（手元の PGO の組）と、CI の x86 の 2 種でそろいました。
- CI の DLL 3 種の libedax のテスト（290 項目）は、失敗 0 でした。
- GitHub の CI（release binaries）は、7 つの job がすべて合格しました。macOS arm64 の job は、1 回目に機械が割り当てられずに取り消され、流し直して合格しました。

未確認は、3 つの設定のこの版での効きと強さ、対局での強さ、32 スレッド、大きな実物の book、Android 実機、Linux・macOS 版、GUI からの利用。

---

## v4.5.5-eval2.20-pre1

- 回帰（51/51）、libedax（290 項目）、book の解析（11/11）、book の再開、book・base（557 項目）のテストは、失敗 0 でした。
- `book learn` を書き込みのたびに止めて続けるテストは、失敗 0 でした。
- 既定の設定で、中盤 20 局面（level 14）で探索した局面の数が前の版と同じでした（7,799,021）。
- fforum-20-39 の完全読みは、着手とスコアの誤り 0 でした（探索した局面 96,356,604）。
- `-pc-r`、`-pc-dev`、`-pc-mob`、`-pc-gate`、`-corr`、`-lmr`、`-lazy`、`-sort-eval-shift`、`-sort-min-depth` を、それぞれ単独で動かしました。異常終了は 0 でした。
- 次の 2 通りの組み合わせで fforum-20-39 を完全読みしました。着手とスコアの誤りは 0、異常終了は 0 でした。
  - `-pc-r`・`-pc-end`・`-pc-dev`・`-pc-mob`・`-corr`・`-lmr`・`-sort-eval-shift`・`-sort-min-depth`（1 スレッド）
  - `-lazy`・`-split-probe`・`-lmr`・`-corr`（4 スレッド）
- `stop` と XBoard の `?` を探索の開始の直前に送るテストは、各 90 回で取りこぼし 0 でした。NBoard の `go` 直後の `ping` は、60 回で取りこぼし 0 でした。
- `book deviate`・`deviate2`・`book enhance` は、`book-reproducible on` の 1 スレッドと 4 スレッドで同じ book でした。
- 中盤の高速化は、AMD Ryzen 9 9950X の AVX-512 版、1 スレッド、level 21、8 回ずつの中央値で CPU 時間 0.925 倍でした。27 通り・1,050 局面で、深さ・評価値・最善手・局面の数・読み筋が前と同じでした。
- book の読み込みは、80,068 局面の book を 80 回読む CPU 時間（10 回ずつの中央値）で、並びのそろった book が 0.936 倍、そろっていない book が 0.629 倍でした。保存し直した book は全バイト同じでした。
- 終盤の高速化の前後で、評価値と最善手は同じでした。fforum-20-39 で探索した局面は 108,368,547 から 96,356,604 に減りました。

未確認は、時間配分の設定、`-pc-end` と `-split-probe` の単独、対局での強さ、32 スレッド、大きな実物の book、Android 実機、Linux・macOS 版の実行、GUI からの利用。

---

## v4.5.5-eval2.19

- Windows の配布版の5種の実行ファイルで、独立実装が採点した合成終盤24局面を解きました。着手とスコアの誤りは各0で、Cassio のEOFテスト7項目も通りました。
- DLL 3種で API の279項目がそれぞれ通りました。
- x86-64-v4 で対象の7件を再テストしました。棋譜1・2件の完了表示、見積もりの手計算、GTP の終局と合法パス、XBoard・GTP・NBoard の `q`、模擬GGS接続、2種類の対話ヘルプを確認しました。
- `estimate` の合計が2と4の試行では、平均3、誤差0.7071068になりました。
- [配布ビルドの CI](https://github.com/Nikque/edax-reversi-AVX/actions/runs/44573928843) で Linux、Intel Mac、Android、Windows ARM64 をビルドしました。Linux のライブラリテストも通りました。
- [代替ビルド](https://github.com/Nikque/edax-reversi-AVX/actions/runs/44577301101) で、同じ修正コミットから Apple Silicon 用実行ファイルとユニバーサルライブラリを Intel Mac 上でクロスビルドしました。両アーキテクチャを確認し、Intel 側のライブラリテストは通りました。Apple Silicon 上での実行は未確認です。

未確認は、実 book、対局での強さ、実際の GGS サーバーとの通信、Android 実機での動作です。

---

## v4.5.5-eval2.18

- 度外石の再現テストで、正解 +10 に対して修正後の境界が +10 になりました。
- Windows 版の 5 種の実行ファイルは、fforum-20-39 の 20 局面で着手・スコアの誤り 0、既知の 1,205 局面でスコアの誤り 0 でした。Cassio EOF の 7 項目も通りました。
- Windows 版の DLL 3 種で API の 279 項目がそれぞれ通りました。
- BMI2 の最終手は 71,168 局面を独立した実装と照合しました。初期評価・差分評価と確定石の回帰テストも通りました。
- [配布ビルドの CI](https://github.com/Nikque/edax-reversi-AVX/actions/runs/44466216151) で Linux・macOS・Android・Windows ARM64 のバイナリをビルドしました。

未確認は、対局での強さ、実 book、SVE の実機での動作と速度です。
