# Edax 4.5.5 修正版

[English](README-NIKQUE.en.md) · [Releases](https://github.com/Nikque/edax-reversi-AVX/releases) · [18件のバグ修正一覧](RELEASE-NOTES.md)

この公開forkは上流の `v4.5.5`（`4cde6ff588f0eade07fcba0c7f02d5cd0cacd4ee`）を基点としています。修正後のソース、再ビルドしたWindows・Linux・macOS x64・Android用実行ファイル、元のGPL-3.0 [ライセンス](LICENSE)を公開しています。上流の `master` ブランチは残し、修正版の `edax-4.5.5-fixes` を既定ブランチに設定しました。

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

bookを統合するには、統合先のbookを読み込んで `book merge source.dat` を実行し、結果を残す場合は `book save merged.dat` で保存します。mergeは統合元にしかない局面を追加し、統合先に既にある局面は上書きしません。続いてLinkの再構築、不整合な局面（古い `nomove` Leafを含む）の修復、評価値の再計算、着手の並べ替えを行います。Linkの再構築を検査より前に行うよう修正したため、追加された子局面が原因の誤った `nomove is wrong` 判定を避けられます。現在のbookを単独で修復する `book fix` も利用できますが、このmerge手順の前提条件ではありません。統合元のファイルが存在しない、または構造が壊れている場合、mergeを中止し現在のbookを維持します。

book保存時は、一時ファイルへの書き込みを確認してから保存先を置き換えます。保存に失敗しても旧bookを保持しますが、保存中はbookとほぼ同じ容量の追加空き領域が必要です。

## その他の修正

従来の `book deviate` を含む18件のバグを修正しました。全件の一覧は [RELEASE-NOTES.md](RELEASE-NOTES.md) にあります。

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

`v3` 版はAVX2対応のx86-64 CPU、`v4` 版はAVX-512対応のx86-64-v4 CPUが必要です。CPUの対応が不明な場合は標準版を選んでください。`config.ini` は環境に合わせてパス、`book-save-interval`、`book-deviate-save-rounds` を設定してください。Windows v4版を再ビルドする場合はVisual Studio 2022のx64 Developer Command Promptで `build-win-v4.cmd` を実行します。その他の環境向けには[release-binariesワークフロー](.github/workflows/release-binaries.yaml)を用意し、`package-release.py` で配布ZIPを作成します。

元配布物の旧32ビットmacOS用 `mEdax-x86` は除外しました。現在のXcode SDKにはi386用のリンクライブラリがなく修正版をビルドできません。元の実行ファイルをそのまま同梱しても、今回の修正は反映されません。

通常のWindowsビルドで従来の回帰試験24ケースと、新しい保存間隔の試験を通過しました。合法棋譜300局、石の反転549,161件、独立した完全読み96局面（1・4スレッド）、Cassio API、イベントキューも照合しました。ビルドと配布ZIPの確認結果は[検証報告](https://github.com/Nikque/edax-reversi-AVX/releases/tag/v4.5.5-nikque.2)に記載します。

このforkの作業には **ChatGPT-6 Astra** と **ChatGPT-6 Sol** を使用しました。Edaxと原著作者の表記を維持し、元のGPL-3.0ライセンスに基づいて配布します。実行ファイルを再配布する場合も、対応するソースとライセンスを入手可能にしてください。
