# 14. コマンドと設定の一覧

[目次](README.md) ｜ 前：[13. 困ったときは](13-troubleshooting.md) ｜ 次：[15. 用語集](15-glossary.md)

v4.5.5-nikque.13 の、全部のコマンドと設定です（ソースの一覧から作りました）。「章」の欄が空のものは、このマニュアルでは説明していない、開発・検査用のものです。

## 14.1 対局・解析のコマンド

| コマンド | 短い名前 | 引数（［ ］は省ける） | 意味 | 章 |
|---|---|---|---|---|
| `help` | `?` | ［`options`・`commands`・`book`・`base`・`test`］ | 組み込みの説明（英語） | [3](03-playing.md) |
| `init` | `i` | | ふつうの初期局面から新しい対局 | [3.3](03-playing.md) |
| `new` | `n` | | 今の「最初の局面」から新しい対局 | [3.3](03-playing.md) |
| `load` | `o`、`open` | ファイル | 棋譜を読み込む | [3.6](03-playing.md) |
| `save` | `s` | ファイル | 棋譜を保存する | [3.6](03-playing.md) |
| `quit` | `q`、`exit` | | 終了 | [2.7](02-quick-start.md) |
| `undo` | `u` | | 手を戻す | [3.3](03-playing.md) |
| `redo` | `r` | | 戻した手を進める | [3.3](03-playing.md) |
| `mode` | `m` | ［0〜3］（省くと 3） | だれが打つか | [3.2](03-playing.md) |
| （マスの名前） | | | その手を打つ。`ps`（`pa`）はパス | [3.1](03-playing.md) |
| `play` | | 手順、または定石の名前 | 手順を打つ | [3.1](03-playing.md) |
| `force` | | 手順、または定石の名前 | Edax に決まった序盤を打たせる | [3.7](03-playing.md) |
| `go` | | | Edax に 1 手打たせる | [3.1](03-playing.md) |
| `hint` | | ［1〜60］（省くと 1） | 良い手を表示 | [3.4](03-playing.md) |
| `stop` | | | 考えるのを止める（`mode 3` になる） | [3.1](03-playing.md) |
| `analyze` | `a`、`analyse` | ［手数］（省くと全部） | 棋譜の最後の n 手を振り返る | [3.4](03-playing.md) |
| `setboard` | | 盤 手番 | 局面を作る | [3.5](03-playing.md) |
| `vmirror`・`hmirror` | | | 上下・左右を入れ替える | [3.5](03-playing.md) |
| `rotate` | | ［90・180・270］（省くと 90） | 回す | [3.5](03-playing.md) |
| `symetry` | | 0〜15 | 対称を番号で指定 | [3.5](03-playing.md) |
| `opening`・`ouverture` | | | 定石の名前（英語・フランス語） | [3.1](03-playing.md) |
| `options` | | | 今の設定を表示 | [4.1](04-settings.md) |
| `version` | `v` | | 版を表示 | |
| （設定の名前） | | 値 | 設定を変える（14.5） | [4.4](04-settings.md) |
| `book`・`b` | | book のコマンド（14.2） | | [6](06-book-basics.md)〜[8](08-book-maintenance.md) |
| `base` | | base のコマンド（14.3） | | [10.2](10-files.md) |
| `solve` | | ファイル | 問題集を解く | [11.1](11-solve-bench.md) |
| `bench` | | ［個数］ | 速さを測る | [11.2](11-solve-bench.md) |
| `count` | | `games`・`positions`・`shapes` ［深さ］ ［盤の大きさ 6〜8］ | 手順・局面を数える | [11.3](11-solve-bench.md) |
| `perft` | | ［深さ］ | 手順を数える（ハッシュ表なし） | [11.3](11-solve-bench.md) |
| `estimate` | | ［回数］ | 手順の数を見積もる | [11.3](11-solve-bench.md) |
| `microbench`、`script-to-obf`、`select-hard`、`wtest`、`weval`、`edaxify`、`seek`、`mobility`、`debug-pv` | | | 開発・検査用 | [11.4](11-solve-bench.md) |
| `nboard 1`、`xboard`、`protocol_version`、`engine-protocol init` | | | 別のプロトコルに切り替える | [12.1](12-integration.md) |

- `#` で始まる行と、空の行は、何もしません。
- コマンドをファイルから与える（`wEdax-x86-64.exe < commands.txt`）と、上から順に実行されます。最後に `quit` を書いておきます。

## 14.2 book のコマンド

`book`（または `b`）の後に書きます。後ろの言葉は、大文字でも通ります。

| コマンド | 引数（［ ］は省ける。（ ）は省いたときの値） | 意味 | 自分で保存するファイル | 章 |
|---|---|---|---|---|
| `show` | | 今の局面の中身 | | [6.3](06-book-basics.md) |
| `info` | | book 全体の情報 | | [6.3](06-book-basics.md) |
| `stats` | | 数の内訳 | | [6.3](06-book-basics.md) |
| `on`・`off` | | 対局で book を使う・使わない | | [6.5](06-book-basics.md) |
| `randomness` | ［n］（0） | book から手を選ぶ幅 | | [6.5](06-book-basics.md) |
| `depth` | ［n］（36） | 深さの設定 | | [6.4](06-book-basics.md) |
| `new` | ［level］（21） ［深さ］（36） | 新しい空の book。**メモリの中の今の book は捨てられる**。その後の、名前を書かない保存では、それまでの `book-file` のファイルを `.old` の名前で残す | | [6.7](06-book-basics.md) |
| `load`、`open` | ファイル | book を読む。別のファイルを読んだ後の、名前を書かない保存では、それまでの `book-file` のファイルを `.old` の名前で残す | | [6.6](06-book-basics.md) |
| `save` | ［ファイル］（`book-file` のファイル） | book を保存する | | [6.6](06-book-basics.md) |
| `verbose` | n | 表示の量（0〜2） | | [8.7](08-book-maintenance.md) |
| `analyze`、`a` | ［手数］ | 棋譜を book の評価値で振り返る | | [3.4](03-playing.md) |
| `store` | | 今の対局を入れる | `.store` | [7.2](07-book-learning.md) |
| `add` | ファイル | 棋譜のファイルを入れる | `.gam` | [7.2](07-book-learning.md) |
| `check` | ファイル | 棋譜を book と比べる | | [7.2](07-book-learning.md) |
| `learn` | ファイル | 手順のリストを打ち継いで入れる | `.store` | [7.2](07-book-learning.md) |
| `deviate` | ［X］（2） ［Y］（4）。X は −129〜129、Y は 0〜65 | 自動で広げる（片方が外れる変化） | `.dev` | [7.3](07-book-learning.md) |
| `deviate2` | ［X］（2） ［Y］（4）。X は 0〜129、Y は 0〜7740 | 自動で広げる（失点の合計。完全読みの Leaf を除く） | `.dev2` | [7.3](07-book-learning.md) |
| `deviate3` | 同じ | 同じ（完全読みの Leaf も含む） | `.dev3` | [7.3](07-book-learning.md) |
| `enhance` | ［X］（2） ［Y］（4）。0〜129 | 評価値を変えそうな Leaf を広げる | `.enh` | [7.4](07-book-learning.md) |
| `fill` | ［n］（1）。1〜61 | 局面の間を埋める | `.fill` | [7.4](07-book-learning.md) |
| `play` | | 手順の先端を、深さまで伸ばす | `.play` | [7.4](07-book-learning.md) |
| `negamax` | | 評価値を計算し直す | | [8.2](08-book-maintenance.md) |
| `fix` | | 点検・Link の張り直し・negamax | | [8.2](08-book-maintenance.md) |
| `merge` | ファイル | 別の book を取り込む | `.mrg` | [8.3](08-book-maintenance.md) |
| `subtree` | | 今の局面から先だけを残す | | [8.4](08-book-maintenance.md) |
| `prune` | | 片方が最善を打つ変化だけを残す | | [8.4](08-book-maintenance.md) |
| `export`・`import` | ファイル | 文字のファイルに書き出す・読み戻す | | [8.5](08-book-maintenance.md) |
| `leaf-recalculate`・`leaf-recalculate3` | `deviate` と同じ | Leaf を計算し直す | `.leaf`・`.leaf3` | [8.6](08-book-maintenance.md) |
| `leaf-recalculate2`・`leaf-recalculate4` | `deviate2` と同じ | Leaf を計算し直す | `.leaf2`・`.leaf4` | [8.6](08-book-maintenance.md) |
| `correct` | | 完全読みの局面を確かめ直す | `.err` | [8.7](08-book-maintenance.md) |
| `deepen` | | （使わない） | `.dep` | [8.7](08-book-maintenance.md) |
| `extract` | ファイル | 最善の手順を棋譜に書き出す | | [8.8](08-book-maintenance.md) |
| `problem` | ［空き］（24） ［個数］（10） | 局面を問題の形で表示 | | [8.8](08-book-maintenance.md) |
| `feed-hash` | | book の値をハッシュ表に入れる | | [8.7](08-book-maintenance.md) |

## 14.3 base のコマンド

`base` の後に書きます（[10.2](10-files.md)）。

| コマンド | 引数 | 意味 |
|---|---|---|
| `convert` | 入力 出力 | 形式を変える |
| `unique` | 入力 出力 | 同じ対局をまとめる |
| `compare` | ファイル1 ファイル2 | 共通する局面を数える |
| `check` | ファイル ［空き］（24） | 終盤の悪手を探す |
| `correct` | ファイル ［空き］（24） | 終盤の悪手を直す（ファイルを書き換える） |
| `complete` | ファイル | 未完の対局を打ち継ぐ（ファイルを書き換える） |
| `problem` | ファイル ［空き］（24） 出力 | 局面を問題のファイルに書き出す |
| `tofen` | ファイル ［空き］（24） 出力 | 局面を FEN で書き出す |

## 14.4 起動のときだけのオプション

| オプション | 意味 | 章 |
|---|---|---|
| `-?`、`-help` | オプションの一覧を表示して終了 | |
| `-v`、`-version` | 版を表示 | |
| `-solve ファイル` | 問題集を解いて終了 | [11.1](11-solve-bench.md) |
| `-bench n` | 速さを測って終了 | [11.2](11-solve-bench.md) |
| `-count games`（`positions`・`shapes`） 深さ ［`6x6`］ | 数えて終了 | [11.3](11-solve-bench.md) |
| `-wtest ファイル` | WTHOR のファイルの検査 | |
| `-gtp`・`-nboard`・`-xboard`・`-cassio`・`-ggs`（`-edax` は、ふつうの画面） | プロトコルを選ぶ | [12.1](12-integration.md) |
| `-o ファイル`（`-option-file`） | 設定のファイルを追加で読む | [4.3](04-settings.md) |
| `-q`・`-vv` | `-verbose 0`・`-verbose 2` と同じ | [3.10](03-playing.md) |
| `-cpu` | スレッドを CPU ごとに割り当てる | [4.3](04-settings.md) |
| `-info` | 追加の情報の行を表示する | |

## 14.5 設定

どれも、`config.ini`（`名前 = 値`）、コマンドライン（`-名前 値`）、実行中（`名前 値`）の 3 つのしかたで指定できます（[4 章](04-settings.md)）。「組み込みの値」は、何も指定しないときの値です。

### よく使う設定

| 名前（短い名前） | 値 | 組み込みの値 | 配布物の `config.ini` | 意味 |
|---|---|---|---|---|
| `level`（`l`） | 0〜60 | 18 | 18 | 探索の強さ |
| `n-tasks`（`n`） | 1〜論理 CPU の数（64 まで）、`auto` | 論理 CPU の数 | `auto` | 探索のスレッドの数 |
| `hash-table-size`（`h`） | 10〜30（32 ビット版は 25 まで）、`auto` | 21 | `auto` | ハッシュ表の大きさ（実行中に変えると、表を作り直す） |
| `game-time`（`t`） | 時間 | なし | | 1 局の持ち時間 |
| `move-time` | 時間 | なし | | 1 手の時間 |
| `ponder` | `on`／`off` | `on` | | 相手の番にも考える |
| `mode` | 0〜3 | 3 | | 起動のときの `mode` |
| `verbose` | 0〜4 | 1 | | 表示の量 |
| `noise` | 0〜60 | 0 | | この深さより浅い途中経過を表示しない |
| `width` | 3〜250 | 80 | | 表の横幅 |
| `eval-file` | ファイル | `data/eval.dat` | | 評価データ |
| `book-file` | ファイル | `data/book.dat` | | book のファイル |
| `book-usage` | `on`／`off` | `on` | `on` | 対局で book を使う |
| `book-randomness` | 0 以上の数 | 0 | | book から手を選ぶ幅 |
| `book-depth` | 1〜60、`auto` | `auto` | `auto` | 起動のときの book の深さ |
| `book-save-interval` | 0〜525600（分） | 60 | 360 | 育てている途中の、時間ごとの保存 |
| `book-deviate-save-rounds` | 0〜1000000 | 1 | 1 | `book deviate` 系の、周ごとの保存 |
| `book-merge-auto-save` | `on`／`off` | `on` | `on` | `book merge` の後の `.mrg` |
| `book-store-auto-save` | `on`／`off` | `on` | | `book store`・`learn` の後の `.store` |
| `book-expand-tasks` | 1〜論理 CPU の数、`auto` | 1 | `auto` | 同時に広げる局面の数 |
| `book-store-tasks` | 1〜論理 CPU の数、`auto` | `auto` | `auto` | 同時に学習する棋譜の数 |
| `book-leaf-recalculate-rounds` | 1〜1000000 | 1 | 1 | `book leaf-recalculate` 系の回数の上限 |
| `probcut-model` | `standard`／`refit` | `standard` | `standard` | 枝刈りの方式（`refit` は実験用） |
| `auto-start`・`auto-swap`・`auto-store`・`auto-quit` | `on`／`off` | `off` | | 終局したときの動作（[3.9](03-playing.md)） |
| `repeat` | 数 | 0 | | 続けて打つ局数 |
| `search-log-file`・`ui-log-file` | ファイル | なし | | 記録のファイル |
| `name` | 文字 | `Edax 4.5.5` | | プロトコルで名乗る名前 |
| `echo` | `on`／`off` | `off` | | 受け取ったコマンドを表示する |

時間の値は、`秒`、`分:秒`、`時:分:秒`、`日:時:分:秒` の形で書きます（最小は 1 秒）。

### 開発・検査用の設定

ふつうは変えません。意味は、組み込みの説明（`-help`）とソースを見てください。

| 名前 | 値 | 組み込みの値 | 組み込みの説明（要約） |
|---|---|---|---|
| `depth`（`d`）・`selectivity` | 数 | なし | 探索の深さ・確かさを直接指定する（`-solve` などで使われる） |
| `alpha`・`beta` | −64〜64 | −64・64 | 探索する評価値の範囲 |
| `speed` | 数、`auto` | `auto` | 持ち時間の配分に使う探索の速さ（ノード/秒）。`auto` は実測 |
| `nps` | 数 | 0 | 時間をノード数で数える（検査用の時計） |
| `inc-pvnode-sort-depth`・`inc-cutnode-sort-depth`・`inc-allnode-sort-depth` | 数 | `options` の表示では pv = 0、all = −2、cut = −3 | 手の並べ替えの深さの調整 |
| `probcut-d` | 小数 | 0.25 | 枝刈りの深さの比 |
| `pv-debug`・`pv-check`・`pv-guess` | `on`／`off` | `off` | 読み筋の検査 |
| `all-best` | `on`／`off` | `off` | （このビルドでは使われていません） |
| `game-file` | ファイル | `data/game.ggf` | （このビルドの画面では使われていません） |
| `ggs-host`・`ggs-port`・`ggs-login`・`ggs-password`・`ggs-open`・`ggs-log-file` | 文字 | なし | GGS への接続 |
| `debug-cassio`・`follow-cassio` | （スイッチ） | | Cassio 用 |

## 14.6 ファイルの拡張子

| 拡張子 | 中身 | 章 |
|---|---|---|
| `.dat`（`book.dat`） | book | [6.6](06-book-basics.md) |
| `.dat`（`eval.dat`） | 評価データ | [1.3](01-introduction.md) |
| `book.dat.store`・`.gam`・`.dev`・`.dev2`・`.dev3`・`.enh`・`.fill`・`.play`・`.leaf`〜`.leaf4`・`.mrg`・`.err`・`.dep` | book のコマンドが自分で保存した book（中身は book のファイル） | [6.6](06-book-basics.md) |
| `book.dat.tmp.番号` | 保存の途中の一時ファイル（残っていたら、消してよい） | [13.3](13-troubleshooting.md) |
| `book.dat.damaged`（`.damaged.1`…） | 読めなかった book のファイルを、上書きせずに残したもの | [13.1](13-troubleshooting.md) |
| `book.dat.old`（`.old.1`…） | `book new`・`book load 別のファイル`・`book import` の前の book のファイルを、上書きせずに残したもの | [6.6](06-book-basics.md)・[6.7](06-book-basics.md) |
| `.txt`・`.sgf`・`.ggf`・`.pgn`・`.wtb`・`.edx` | 棋譜 | [10.1](10-files.md) |
| `.eps`・`.svg` | 盤の図 | [3.6](03-playing.md) |
| `.obf` | 問題集 | [10.3](10-files.md) |
| `config.ini`・`edax.ini` | 設定 | [4 章](04-settings.md) |

## 14.7 level の早見表

| level | 中盤で読む深さ | 完全読みになる空き |
|---|---|---|
| 1〜10 | level 手（100%） | level×2 以下 |
| 11〜18 | level 手（73%） | 21 以下 |
| 19〜24 | level 手（73%） | 24 以下 |
| 25〜29 | level 手（73%） | 27 以下 |
| 30〜35 | level 手（73%） | 30 以下 |
| 36〜59 | level 手（73%） | level−6 以下 |
| 60 | — | いつでも |

くわしい表は [5.2](05-search.md)にあります。

次：[15. 用語集](15-glossary.md)
