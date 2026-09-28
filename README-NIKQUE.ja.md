# Edax 4.5.5 修正版

[English](README-NIKQUE.en.md) · [Releases](https://github.com/Nikque/edax-reversi-AVX/releases) · [修正一覧](RELEASE-NOTES.ja.md)

この公開forkは上流の `v4.5.5`（`4cde6ff588f0eade07fcba0c7f02d5cd0cacd4ee`）を基点としています。修正後のソース、再ビルドしたWindows・Linux・macOS x64・Android用実行ファイル、元のGPL-3.0 [ライセンス](LICENSE)を公開しています。上流の `master` ブランチは残し、修正版の `edax-4.5.5-fixes` を既定ブランチに設定しました。

## v4.5.5-nikque.3 の変更点

大規模book（数億局面・数十GB）で `book deviate`・`book deviate2`・`book deviate3`・`book merge` を使うときの処理時間とメモリ使用量を改善し、11件の不具合を修正しました。また、`book merge` の後にbookを自動で保存する設定を追加しました。bookのファイル形式は変わっていません。以前のbookをそのまま読み書きできます。

### 処理時間とメモリ

6.57億局面・28.95GBの実bookで計測しました（Ryzen 9 9950X、32論理CPU、別のEdaxプロセスが並行して稼働中の環境）。

| 処理（全量book） | v4.5.5-nikque.2 | v4.5.5-nikque.3 |
|---|---|---|
| bookのロード | 445.6秒 | 48.9秒 |
| ロード後のメモリ | 58.4GiB | 35.5GiB |
| bookの保存 | 657.2秒 | 28.3秒 |
| negamax（deviate系は開始時と毎周に実行） | 936.5秒 | 約33秒 |
| `book deviate 2 4` の1周（展開対象0件） | 2,909.6秒 | 142.0秒 |
| deviate2 / deviate3 の展開対象の選択 | 446.8秒 / 479.2秒 | 21.4秒 / 22.9秒 |
| 6.6億局面どうしの `book merge`（下記） | 16,408秒（4時間33分）、117.2GiB | 263秒、37.0GiB |

展開対象が多い場合、deviate2・deviate3 の1周は1件ずつの探索（1件あたり子局面と親局面の2回）に大半の時間を使います。この部分の速さは変わりません。

### `book merge` の実測

6.57億局面・28.95GBの実bookに、別の実book（6.57億局面・29.03GB）をmergeしました（173.6万局面を追加）。公開された両版の `wEdax-x86-64-v4.exe` で、同じPC・同じbookを使い、1つずつ実行しています。

| 処理 | v4.5.5-nikque.2 | v4.5.5-nikque.3 | nikque.3 で再利用を無効にした場合（参考） |
|---|---|---|---|
| 起動時のbookのロード | 222秒 | 28秒 | 30秒 |
| `book merge` コマンド全体 | 16,408秒（4時間33分） | 263秒 | 10,934秒（3時間2分） |
| うちLinkの再構築 | 15,332秒 | 177秒 | 10,845秒 |
| merge後の `book save` | 251秒 | 18秒 | 62秒 |
| 起動から終了まで | 17,364秒（4時間49分） | 315秒 | 11,027秒 |
| ピークメモリ | 117.2GiB | 37.0GiB | 37.0GiB |

mergeにかかる時間の大半は、Linkの張り直しで最善の未登録手（Leaf）がLinkになった局面の探索し直しです（今回は約18.4万局面）。v4.5.5-nikque.3 は、統合元のbookが同じ探索レベルですでに探索している局面では、その結果を再利用して探索を省きます。これだけでmergeは約42倍速くなります（10,934秒→263秒）。再利用以外の改良（統合元を読み込まない処理、並列化など）による短縮は約1.5倍で、メモリはこちらの改良で117.2GiBから37.0GiBに減りました。

**再利用により、mergeの結果の評価値がわずかに変わることがあります。** 再利用するLeafの手は、探索し直した場合と同じ手になるはずですが、評価値は探索時のハッシュ表の状態や並列探索の進み方で変わることがあるためです。今回のmergeでは、6.59億局面のうち27,897局面（0.004%）で nikque.2 と評価値が異なりました。直接の差は14,200局面で、残りはその差が親局面へ伝わったものです。評価値の差はほとんどが1〜2です。局面の集合、Linkの手、勝敗の集計、探索レベルは完全に一致しました。なお、nikque.2 自身も探索し直す局面の評価値が実行ごとに揺れます。縮小した実book（3,484万局面）どうしのmergeでは、nikque.2 を2回実行しただけで1,655局面の評価値が異なり、nikque.2 と nikque.3 の差（約2,500局面）と同じ程度でした。

主な変更内容は次のとおりです。

- bookの読み書きを16MBのバッファ経由で行います（以前は1局面あたり約15回の小さな読み書き）。
- メモリ上の局面を64バイトから56バイトにしました。Linkが4個以下の局面（大規模bookの99%）は、Linkを別の領域に確保せず局面の中に置きます。
- Edaxが保存したbookは、ロード時に1つの領域へ隙間なく配置します（以前は配列の予備領域が約20%ありました）。
- negamax、deviate系の展開対象の選択、merge時のLinkの再構築・検査・並べ替えを、`n-tasks`（既定はCPU数）のスレッドで並列に処理します。並列処理でも結果は同じです。
- 各周の「全局面の印を消す」処理と「展開対象を探す全局面走査」をなくしました。展開の順序は以前と同じです。
- `book merge` は統合元のbookを読み込まず、ファイルを2回読みます（1回目で全体を検査、2回目で局面を追加）。統合元が壊れている場合は統合先を変更しません。統合元がすでに探索済みの局面については、その結果（最善の未登録手）を再利用し、探索をやり直しません（評価値がわずかに変わることがあります。上の「`book merge` の実測」を参照）。

book学習と同時に対局や解析を行う場合は、`n-tasks`（`-n`）でスレッド数を指定できます。

### merge後の自動保存（新しい設定）

`bin/config.ini` の `book-merge-auto-save` が `on`（既定）のとき、`book merge` が成功すると、統合後のbookを `<bookファイル名>.mrg`（既定では `data/book.dat.mrg`）に自動で保存し、`Merged book saved to data/book.dat.mrg` と表示します。`book save` を別に実行する必要はありません。保存先のファイル名は、deviate系の進捗ファイル（`.dev`・`.dev2`・`.dev3`）と同じく、設定されたbookファイル名に拡張子を付けたものです。`book load` で別のbookを開いている場合も同じ名前になります。

- `.mrg` を使うには、`book load data/book.dat.mrg` で読み込むか、ファイル名を変更してください。元のbook（`data/book.dat`）は自動保存では上書きしません。
- mergeが失敗した場合（統合元が存在しない・壊れているなど）は保存しません。
- 終了時の保存は以前と同じです。merge後に `book save` をせずにEdaxを終了すると、以前と同様に設定されたbookファイル（`data/book.dat`）へ保存されます。
- `off` にすると、`.mrg` の保存と表示を行わず、以前とまったく同じ動作になります。コマンドラインの `-book-merge-auto-save off` でも指定できます。

### 同一性の確認

変更前後で次のコマンドを同じ条件で実行し、出力が一致することを確認しました。一致しないのは、変更前どうしでも実行ごとに変わる箇所（bookの同点の手からの乱数選択、`.edx` に含まれる値、画面の時間・速度表示）だけです。新しい自動保存で増える `.mrg` ファイルと表示は、この比較の対象外です（`book-merge-auto-save = off` では変更前と完全に一致します）。

- book：`new`・`load`・`save`・`import`・`export`・`merge`・`info`・`stats`・`show`・`analyze`・`fix`・`negamax`・`correct`・`prune`・`subtree`・`add`・`check`・`problem`・`extract`・`deviate`・`deviate2`・`deviate3`・`enhance`・`play`・`deepen`・`feed-hash`・`store`・`depth`・`randomness`・`on`・`off`
- 対局：`play`・`go`・`hint`・`save`・`load`・`vmirror`・`hmirror`・`rotate`・`undo`・`redo`・`setboard`

1スレッドと8スレッド、生成した小規模bookと実bookから切り出した649万局面のbook、Windows版5種（x86-64 / v3 / v4、x86、x86-sse）で確認しています。探索ハッシュの扱いは変更していません。

以下の3点は、変更の結果として出力が変わります。

- `book merge` で、Linkの張り直し後に統合元の探索結果を再利用した局面と、その親局面の評価値（わずかに変わることがあります。「`book merge` の実測」を参照）。
- `book merge` で追加した局面のうち、初期局面から辿れない局面の評価値（以前は ±127 などが残っていた）。
- 統合後の局面数が統合先のバケット数の32倍を超える `book merge`（例：`book new` 直後のbookへの大規模merge）では、バケット数を増やすため、保存される局面の順序が変わります。局面の内容は同じです。

### fork元（上流 v4.5.5）との比較

上流の配布物 `edax-4.5.5.zip`（SHA-256 `6f446149092b37cbaa57f68a5610c6e960d45c4f0cbb9a3c8e7a563b503fa9a4`）の実行ファイルと v4.5.5-nikque.3 を同じ条件で実行し、出力を比較しました（Windows版の x86-64-v4・x86-64・x86）。上流にない `deviate2`・`deviate3` と保存間隔の設定は比較から除きました。

次のコマンドでは、出力されたbook・テキストと画面表示が上流と一致しました。

- 実bookから切り出した649万局面のbook：`load`・`stats`・`negamax`・`export`・`show`・`subtree`・`prune`
- 生成したbook：`new`・`deviate`・`enhance`・`negamax`・`fix`・`correct`・`export`・`import`・`subtree`・`prune`・`problem`・`extract`・`check`・`add`・`depth`・`store`・`play`・`deepen`、対局コマンド（`play`・`go`・`hint`・`save`・`undo`・`redo` など）

上流と異なるのは次の点で、いずれも修正または表示の変更によるものです。

- 上流版の異常終了（修正済み）：着手が1つもない局面（パスのみ）を含むbookを表示すると異常終了します（実bookでは `book info` で発生）。統合先に古い `nomove` Leafがあると、merge後の `book fix` で異常終了します。
- `book merge`：上流はLinkを張り直さず、「Book needs to be fixed before usage」と表示して `book fix` の実行を求めます。merge直後に `book fix` を実行すれば、保存されるbookは上流と一致します。本版はmergeの中でLinkの再構築・修復・negamax・並べ替えまで行います。存在しないファイルや途中で切れたファイルのmergeは、上流では空のbookや途中までの局面を統合していましたが、本版は拒否します。
- `.edx` の保存：上流はWindowsでテキストモードで書き出すため、保存した `.edx` を正しく読み込めません。
- 終了時の保存：上流は `book save` で保存した後も、終了時に設定されたbookファイル（`data/book.dat`）へ再び保存します。本版は、明示的に保存した後の変更がなければ終了時に保存しません。
- 表示：`book deviate` の `todo` 件数は、上流では10件ごとの途中経過も表示していましたが、本版は最終値だけを表示します。`book info` の「Memory occupation」の値は、局面のメモリ上の大きさの変更に合わせて変わります。

`book load` で別のbookを読み込む間は、失敗したときに現在のbookを残すため（v4.5.5-nikque.2 からの動作）、一時的に2冊分のメモリを使います。起動時の読み込みと `book merge` にはこの追加はありません。

### 不具合の修正

| 内容 | 以前の動作 |
|---|---|
| 存在しない局面を指すLinkの扱い | negamaxなどで異常終了していました。`book fix` がそのLinkを取り除きます。 |
| パイプで入力したコマンドの処理 | 起動中（bookのロード中）に `quit` が届くと異常終了していました。 |
| `book fill` | 局面の追加中に解放済みのメモリへ書き込み、異常終了することがありました。 |
| メモリ不足時の局面追加 | 局面を黙って失っていました。学習を停止するようにしました。 |
| 保存時のディスク書き出し | ファイル内容をディスクに確定させる前に置き換えていました。 |
| mergeで追加された局面の評価値 | 初期局面から辿れない局面に ±127 が残っていました。 |
| 空・途中切れ・不正な手を含む `.edx` の読み込み | 現在の棋譜が消えていました。読み込みを中止し、棋譜を残します。 |
| Windowsの時計 | OS起動から約49.7日で値が巻き戻っていました。 |
| bookヘッダーの日時 | 未初期化の1バイトを書き出していました。 |
| 局面数の上限 | 約21億局面を超えると桁あふれしていました。追加を拒否します。 |
| バケット数 | 上限が2^26で、10億局面を超えると検索が遅くなっていました。局面数に応じて決めます。 |

一覧は [RELEASE-NOTES.ja.md](RELEASE-NOTES.ja.md) にあります。

## bookの学習と保守

bookを読み込んだEdaxのコマンド入力画面で、次のコマンドを使います。いずれも現在の局面から登録済みLinkをたどり、条件に合うLeafを展開します。Leafは次の局面へのLinkがない候補手であり、bookに存在しない任意の手を列挙するわけではありません。設定されたbook深さで止まり、新しい局面やLinkがなくなるまで展開を繰り返します。

| コマンド | 対象とする条件 | 主な用途 |
|---|---|---|
| `book deviate 2 4` | 従来の相対誤差と開始局面からの絶対誤差。従来の選択条件を維持。 | 従来の学習方法を続ける場合。 |
| `book deviate2 5 5` | 一手の評価損失は最大5、黒白合計も最大5。bookの学習レベルで完全読み済みのLeafを除外。 | 完全読み済みLeafの再学習を避ける場合。 |
| `book deviate3 5 5` | 一手・累積の制限はdeviate2と同じで、完全読み済みLeafも含む。 | 以前のdeviate2と同じ対象を選ぶ場合。 |

`deviate2` と `deviate3` では、黒2石・白3石の損失や一手で5石の損失は範囲内、一手で6石または合計6石の損失は範囲外です。表示される `todo` はその回に選ばれたLeaf局面の件数であり、異なる棋譜の本数ではありません。進捗bookには、設定されたbookファイル名に応じて `.dev`、`.dev2`、`.dev3` が付きます。

`bin/config.ini` には、3種類のdeviateコマンドに適用する独立した保存条件が2つあります。`book-save-interval` は展開中の時間間隔による保存（分）で、`0` はこれを無効にします。`book-deviate-save-rounds` は増分があった完了済みの周回数で、`1` は従来どおり毎周、`10` は10周ごとと学習完了時、`0` は時間間隔による保存を除き学習完了時だけ保存します。従来の `book deviate` は黒白両方の展開で1周、`deviate2/3` は1回の展開で1周です。失敗や中断時には最後の保存以降の学習内容が失われる可能性があり、特に `0` と時間保存の無効化を組み合わせた場合は注意が必要です。互換性のため初期値は `1` です。

### 従来の `book deviate` の修正

従来の `book deviate` には、ポインタの寿命に関するバグがありました。最初の `book_expand` 中に呼ばれる `book_add` は、ハッシュバケットの `Position` 配列を `realloc` で移動させることがあります。従来はその後の2回目の `position_deviate` に古い `root` ポインタを渡しており、解放済み領域を読んで学習が中断したり、局面が欠落したりする可能性がありました。現在は展開後に開始局面からrootを取り直します。従来コマンドの選択条件は変更していません。既存bookの欠落局面を自動で補う修正ではありません。

### `book merge` と `book fix`

bookを統合するには、統合先のbookを読み込んで `book merge source.dat` を実行します。`book-merge-auto-save = on`（既定）なら結果は `<bookファイル名>.mrg` に自動で保存されます。別の名前で残す場合や `off` の場合は `book save merged.dat` で保存します。mergeは統合元にしかない局面を追加し、統合先に既にある局面は上書きしません。続いてLinkの再構築、不整合な局面（古い `nomove` Leafを含む）の修復、評価値の再計算、着手の並べ替えを行います。Linkの張り直しで統合先の局面のLeafが空になった場合、統合元の同じ局面のLeafがまだLinkになっていなければそれを使い、そうでなければ探索します。現在のbookを単独で修復する `book fix` も利用できますが、このmerge手順の前提条件ではありません。統合元のファイルが存在しない、または構造が壊れている場合、mergeを中止し現在のbookを維持します。merge中のメモリ使用量は統合先のbookの分だけです。

book保存時は、一時ファイルへの書き込みをディスクに確定させてから保存先を置き換えます。保存に失敗しても旧bookを保持しますが、保存中はbookとほぼ同じ容量の追加空き領域が必要です。

## その他の修正（v4.5.5-nikque.2 まで）

従来の `book deviate` を含む18件のバグを修正しました。全件の一覧は [RELEASE-NOTES.ja.md](RELEASE-NOTES.ja.md) にあります。

| 分野 | 修正内容 |
|---|---|
| ファイルとbook | Windowsでの`.edx`バイナリ入出力、未対応の保存拡張子を開く前に拒否、一時ファイル経由の安全なbook保存、破損レコードの拒否と読み込み中のbook・起動時bookの保護、`book enhance` の収束までの反復。 |
| 対局と持ち時間 | XBoardの `level 0` でのゼロ除算防止、着手後のFischer加算、棋譜解析履歴の境界確認とパス履歴の初期化。 |
| PGNとGGF | 密なFEN・最大長GGFフィールド用の領域確保、PGNのFENタグと時刻タグの往復、PGN読込を最大60着手に制限。 |
| コマンドと通信 | パーサーの出力先サイズと短いファイル名の処理、GTP `reg_genmove` の結果返却、`time_left` の色検証と応答IDの初期化、NBoardの数値型に合った表示。 |

## ビルドと利用

Releaseの配布一式には[元forkのv4.5.5配布物](https://github.com/okuhara/edax-reversi-AVX/releases/tag/v4.5.5)からそのまま取り出した評価データ `bin/data/eval.dat`（SHA-256 `f8b2299612d9fa4414157e70e932636e33111c2602d0c2fc382a7d90ef21b792`）、初期book `bin/data/book.dat`、問題集を同梱します。既定の `data/eval.dat` を参照できるよう、実行ファイルは `bin/` から起動するか、`-eval-file` でファイルを指定してください。配布ZIPには次の実行ファイルが入っています。

| 実行環境 | `bin/` 内のファイル |
|---|---|
| Windows x86-64：標準 / AVX2 / AVX-512 | `wEdax-x86-64.exe` / `wEdax-x86-64-v3.exe` / `wEdax-x86-64-v4.exe` |
| Windows 32-bit x86：標準 / SSE2 | `wEdax-x86.exe` / `wEdax-x86-sse.exe` |
| Windows ARM64 | `wEdax-arm64.exe` |
| Linux x86-64：標準 / AVX2 / AVX-512 | `lEdax-x86-64` / `lEdax-x86-64-v3` / `lEdax-x86-64-v4` |
| Linux 32-bit x86 | `lEdax-x86` |
| macOS Intel x86-64 | `mEdax-x64-modern` |
| Android ARM64 / 32-bit ARMv7 | `aEdax-arm64-v8a` / `aEdax-armeabi-v7a` |

`v3` 版はAVX2対応のx86-64 CPU、`v4` 版はAVX-512対応のx86-64-v4 CPUが必要です。CPUの対応が不明な場合は標準版を選んでください。`config.ini` は環境に合わせてパス、`book-save-interval`、`book-deviate-save-rounds`、`book-merge-auto-save` を設定してください。Windows版は Visual Studio 2022 の Developer Command Prompt で `src` に移動し、`nmake -f NMakefile vc-x64-v4` などのターゲットでビルドします（v4版は `build-win-v4.cmd` でも作れます）。その他の環境向けには[release-binariesワークフロー](.github/workflows/release-binaries.yaml)を用意し、`package-release.py` で配布ZIPを作成します。v4.5.5-nikque.3 の実行ファイルは、Windows版を Visual Studio 2022（MSVC 19.44）、Linux版を Ubuntu 22.04（WSL）の gcc 11.4、Android版を NDK r27d でビルドし、macOS版は release-binaries ワークフローでビルドしました。32ビットLinux版（`lEdax-x86`）は、bookの並列処理に必要なアトミック命令のため libatomic を静的にリンクしています（i486以降のCPUが必要です）。

元配布物の旧32ビットmacOS用 `mEdax-x86` は除外しました。現在のXcode SDKにはi386用のリンクライブラリがなく修正版をビルドできません。元の実行ファイルをそのまま同梱しても、今回の修正は反映されません。

このforkの作業には **ChatGPT-6 Astra** と **ChatGPT-6 Sol** を使用し、v4.5.5-nikque.3 の性能改善と不具合修正には **Claude Opus 5.5**（Claude Code）を使用しました。Edaxと原著作者の表記を維持し、元のGPL-3.0ライセンスに基づいて配布します。実行ファイルを再配布する場合も、対応するソースとライセンスを入手可能にしてください。
