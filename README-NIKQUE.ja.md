# Edax 4.5.5 修正版

[English](README-NIKQUE.en.md) · [Releases](https://github.com/Nikque/edax-reversi-AVX/releases) · [17項目の修正一覧](RELEASE-NOTES.md)

この公開forkは上流の `v4.5.5`（`4cde6ff588f0eade07fcba0c7f02d5cd0cacd4ee`）を基点としています。修正後のソース、再ビルドしたWindows・Linux・macOS x64・Android用実行ファイル、元のGPL-3.0 [ライセンス](LICENSE)を公開しています。上流の `master` ブランチは残し、修正版の `edax-4.5.5-fixes` を既定ブランチに設定しました。

## bookの学習と保守

`book deviate2 <一手の許容損失> <双方の累積許容損失>` は登録済みLinkをたどり、一手ごとの評価損失と黒白合計の評価損失を制限します。例えば `book deviate2 5 5` は黒2石・白3石の損失や一手で5石の損失を許容し、6石の損失や合計5石を超える進行を除外します。条件内の未展開Leafを候補にしますが、bookの学習レベルで完全読み済みのLeafは除外します。`book deviate3` は同じ損失条件で完全読み済みLeafも含め、以前の `deviate2` と同じ動作にします。従来の `book deviate` の評価条件は維持しています。

bookの自動保存間隔は `config.ini` の `book-save-interval` から分単位で読み込み、`0` で定期保存を無効にできます。従来の `book deviate` では、position追加によりハッシュバケットが移動する場合に備え、展開後にrootを再取得します。book merge/fixは不整合な `nomove` を扱う際に発生していた強制終了を修正しました。破損したbookレコードは読み込み時に拒否し、保存では一時ファイルを完全に書き込んだ後に保存先を置き換えます。保存失敗時は旧bookを保持しますが、保存時にはbookとほぼ同じ容量の追加空き領域が必要です。

## その他の修正

監査で指摘された17項目の対応表は [RELEASE-NOTES.md](RELEASE-NOTES.md) にあります。

| 分野 | 修正内容 |
|---|---|
| ファイルとbook | Windowsでの`.edx`バイナリ入出力、未対応の保存拡張子を開く前に拒否、一時ファイル経由の安全なbook保存、破損レコードの拒否と読み込み中のbook・起動時bookの保護、`book enhance` の収束までの反復。 |
| 対局と持ち時間 | XBoardの `level 0` でのゼロ除算防止、着手後のFischer加算、棋譜解析履歴の境界確認とパス履歴の初期化。 |
| PGNとGGF | 密なFEN・最大長GGFフィールド用の領域確保、PGNのFENタグと時刻タグの往復、PGN読込を最大60着手に制限。 |
| コマンドと通信 | パーサーの出力先サイズと短いファイル名の処理、GTP `reg_genmove` の結果返却、`time_left` の色検証と応答IDの初期化、NBoardの数値型に合った表示。 |

## ビルドと利用

Releaseの配布一式には[元forkのv4.5.5配布物](https://github.com/okuhara/edax-reversi-AVX/releases/tag/v4.5.5)からそのまま取り出した評価データ `bin/data/eval.dat`（SHA-256 `f8b2299612d9fa4414157e70e932636e33111c2602d0c2fc382a7d90ef21b792`）、初期book `bin/data/book.dat`、問題集を同梱します。既定の `data/eval.dat` を参照できるよう、実行ファイルは `bin/` から起動するか、`-eval-file` でファイルを指定してください。OSとCPUに合う実行ファイルを選んでください。`wEdax-x86-64-v4.exe` はx86-64-v4（AVX-512）対応CPUを必要とします。`config.ini` は環境に合わせてパスと保存間隔を設定してください。Windows v4版を再ビルドする場合はVisual Studio 2022のx64 Developer Command Promptで `build-win-v4.cmd` を実行します。その他の環境向けには[release-binariesワークフロー](.github/workflows/release-binaries.yaml)を用意し、`package-release.py` で配布ZIPを作成します。

元配布物の旧32ビットmacOS用 `mEdax-x86` は除外しました。現在のXcode SDKにはi386用のリンクライブラリがなく修正版をビルドできません。元の実行ファイルをそのまま同梱しても、今回の修正は反映されません。

通常のWindowsビルドで回帰試験24ケースを通過しました。合法棋譜300局、石の反転549,161件、独立した完全読み96局面（1・4スレッド）、Cassio API、イベントキューも照合しました。ユーザーの28GBのbookは変更・再集計していません。最終環境ではAddressSanitizer版を正常実行できなかったため、報告した結果は通常ビルドの試験とコード確認に基づきます。詳細と残る検証上の制約は[検証報告](https://github.com/Nikque/edax-reversi-AVX/releases)に記載します。

このforkの作業には **ChatGPT-6 Astra** と **ChatGPT-6 Sol** を使用しました。Edaxと原著作者の表記を維持し、元のGPL-3.0ライセンスに基づいて配布します。実行ファイルを再配布する場合も、対応するソースとライセンスを入手可能にしてください。
