# 4. 設定

[目次](README.md) ｜ 前：[3. 対局と解析のコマンド](03-playing.md) ｜ 次：[5. 探索の基礎](05-search.md)

Edax の動作は、**設定**で変えられます。設定のしかたは 3 つあり、どれも同じ名前を使います。

| しかた | いつ効くか | 例 |
|---|---|---|
| 設定ファイル `config.ini` に書く | 起動のたびに、いつも | `level = 18` |
| 起動のときのオプション（コマンドライン） | その 1 回の起動だけ | `wEdax-x86-64.exe -level 18`（短く `-l 18`） |
| 実行中に、`>` の後に打つ | 打った後、次に探索やコマンドを始めるときから | `level 18` |

## 4.1 設定が読まれる順番

Edax は、起動のときに次の順に設定を読みます。**後から読んだものが優先**されます。

1. プログラムに組み込まれた値（4.5 の表の「組み込みの値」）
2. 今いるフォルダーの `edax.ini`（あれば。配布物には入っていません）
3. **実行ファイルと同じフォルダー**の `config.ini`（配布物では `bin/config.ini`）
4. コマンドラインのオプション

注意：

- `config.ini` は「実行ファイルの隣」のものが読まれます。今いるフォルダーに置いた `config.ini` は、実行ファイルが別のフォルダーにあるなら、読まれません。
- 実行ファイルを、フォルダーを付けずに名前だけで起動した場合（`PATH` の通った場所に置いた場合）は、実行ファイルの場所が分からないので、今いるフォルダーの `config.ini` を探します。見つからなければ、次の警告を出して、組み込みの値で動きます。

  ```
  WARNING: config.ini was not found in the current folder (Edax was started without its folder name): default settings are used
  ```
- 今の値は、`>` の後に `options` と打つと、全部表示されます。

## 4.2 config.ini の書き方

`config.ini` は、メモ帳などで開いて書き換えられる、ふつうの文字のファイルです。1 行に 1 つ、`名前 = 値` と書きます。

```
# 探索の強さ
level = 18
n-tasks = auto
book-file = data/book.dat
```

- `#` から行の終わりまでは、説明（コメント）です。空の行は読み飛ばされます。
- `level = 18`、`level=18`、`level 18`、`set level 18` は、どれも同じ意味です。
- 名前は、大文字と小文字を区別しません。名前の中の空白・`_`・`-` も同じに扱います（`book-depth`、`book_depth`、`Book Depth` は同じ設定です）。
- `on`／`off` の設定には、`true`／`false`、`yes`／`no`、`1`／`0` とも書けます。
- `=` を書いた場合は、行の残りが全部、値になります（空白を含むファイル名も書けます）。
- 全角の空白と全角の「＝」も使えます。文字コードは UTF-8（BOM があってもなくてもよい）、改行は Windows の形式でも Linux の形式でもかまいません。
- 名前を間違えると、起動のときに、ファイル名と行の番号を付けて警告が出ます（その行は無視されます）。

  ```
  WARNING: config.ini:7: unknown or incomplete setting "levle" ignored
  ```
- 数でない値（`level = abc`）や、`on`／`off` でない値も、警告が出て無視されます。範囲の外の数は、警告が出て、範囲の端の値になります。

コマンドラインのオプションは、どれも、頭の `-` を取った名前で `config.ini` に書けます（`-n 16` は `n-tasks = 16`、`-book-file data/my.dat` は `book-file = data/my.dat`）。

## 4.3 コマンドラインで指定する

```
wEdax-x86-64.exe -l 12 -n 4 -book-file data/my.dat
```

- `-名前 値` の形で、いくつでも並べられます。
- 値のない、スイッチだけのオプションもあります：`-q`（静かに。`-verbose 0` と同じ）、`-vv`（くわしく。`-verbose 2` と同じ）、`-cpu`（スレッドを CPU ごとに割り当てる。Linux では、その CPU に固定されます。Windows では「できればその CPU で動かす」という希望を出すだけです。Linux でこれを付けると、book のコマンドは、同時に行う探索（`book-expand-tasks`・`book-store-tasks`）を使いません）。
- `-?` または `-help` で、オプションの一覧（英語）を表示して終了します。`-v` または `-version` で、版を表示します。
- 設定のほかに、起動のしかたを変えるものがあります。

  | オプション | 意味 | 章 |
  |---|---|---|
  | `-solve ファイル` | 問題集を解いて、結果を表示して終了する | [11](11-solve-bench.md) |
  | `-bench n` | 速さを測って終了する | [11](11-solve-bench.md) |
  | `-gtp`・`-nboard`・`-xboard`・`-ggs`・`-cassio` | ほかのプログラムとつなぐための言葉（プロトコル）で起動する | [12](12-integration.md) |
  | `-o ファイル`（`-option-file`） | 設定を、指定のファイルからも読む（書き方は `config.ini` と同じ） | |

## 4.4 実行中に変える

`>` の後に、名前と値を打ちます。

```
>level 12
>n-tasks 4
>book-randomness 2
```

- 何も表示されなければ、受け付けられています。間違った値には警告が出ます。

  ```
  >level abc
  WARNING: level: "abc" is not a number; ignored
  ```
- `hash-table-size` を実行中に変えると、探索の表が、その大きさで作り直されます（表の中身は消えます）。`hash-table-size = auto` のときは、`n-tasks` を実行中に変えても、スレッドの数に合わせて作り直されます（1 スレッドで起動して `n-tasks 16` と打つと、使うメモリが 79MB から 242MB になりました）。v4.5.5-nikque.12 までの版では、`options` の表示だけが変わり、表の大きさは起動のときのままでした。
- `book-file` を実行中に変えても、今の book は読み直されません。変わるのは、これ以降の「自動の保存先」です（[6 章](06-book-basics.md)）。別の book を読むには `book load ファイル` を使います。

## 4.5 配布物の config.ini の設定

配布物の `bin/config.ini` には、次の設定が書いてあります。「組み込みの値」は、`config.ini` がないとき（または、その行を消したとき）の値です。

| 設定 | 配布物の値 | 組み込みの値 | 意味 |
|---|---|---|---|
| `level` | `18` | 18 | 起動のときの探索の強さ（0〜60）。[5 章](05-search.md) |
| `n-tasks` | `auto` | 論理 CPU の数 | 探索に使うスレッドの数。`auto` は、その PC の論理 CPU の数 |
| `book-depth` | `auto` | `auto` | 起動のときの book の深さ。`auto` は、book のファイルに保存されている深さのまま。1〜60 の数を書くと、起動のときにその深さにする。[6 章](06-book-basics.md) |
| `book-usage` | `on` | `on` | 対局で、book にある手を打つかどうか |
| `book-save-interval` | `360` | 60 | book を育てている途中で、時間で保存する間隔（分）。`0` は時間では保存しない。[7 章](07-book-learning.md) |
| `book-deviate-save-rounds` | `1` | 1 | `book deviate` 系で、局面が増えた周を何周終えるごとに保存するか。`0` は終わったときだけ。[7 章](07-book-learning.md) |
| `book-merge-auto-save` | `on` | `on` | `book merge` が成功するたびに、`<book のファイル名>.mrg` に保存する。[8 章](08-book-maintenance.md) |
| `hash-table-size` | `auto` | 21 | 探索用のハッシュ表の大きさ。`auto` は、スレッドの数から決める。[5 章](05-search.md) |
| `book-expand-tasks` | `auto` | 1 | book を自動で広げるコマンドが、同時に広げる局面の数。[7 章](07-book-learning.md)・[9 章](09-large-books.md) |
| `book-leaf-recalculate-rounds` | `1` | 1 | `book leaf-recalculate` 系を繰り返す回数の上限。[8 章](08-book-maintenance.md) |
| `book-store-tasks` | `auto` | `auto` | 棋譜から book を育てるコマンドが、同時に学習する棋譜の数。[7 章](07-book-learning.md)・[9 章](09-large-books.md) |
| `probcut-model` | `standard` | `standard` | 探索の枝刈りの方式。`refit` は実験用（探索の結果が変わります。配布物の README の v4.5.5-nikque.5 の節を読んでから使ってください） |

**`config.ini` の `level` についての注意**：`config.ini`（と `edax.ini`）に書いた `level` は、「起動したときの level」を決めるだけです。持ち時間つきの対局（`-t`・`-move-time`）では、読みの上限になりません。上限にしたいときは、コマンドラインの `-l` か、実行中の `level` コマンドで指定します（[3.8](03-playing.md)）。

## 4.6 よく使うそのほかの設定

| 設定 | 組み込みの値 | 意味 |
|---|---|---|
| `book-file` | `data/book.dat` | book のファイル。起動のときに読み、自動の保存もここ（と、この名前に拡張子を足した名前）に行う |
| `eval-file` | `data/eval.dat` | 評価データのファイル |
| `book-randomness` | 0 | book から打つとき、最善の手より何石まで悪い手を選んでよいか。[6 章](06-book-basics.md) |
| `book-store-auto-save` | `on` | `book store`・`book learn` の後で、`<book のファイル名>.store` に保存するかどうか。[7 章](07-book-learning.md) |
| `game-time`（`-t`） | なし | 1 局の持ち時間。[3.8](03-playing.md) |
| `move-time` | なし | 1 手の時間 |
| `ponder` | `on` | 相手の番の間も考える |
| `mode` | 3 | 起動したときの `mode`（[3.2](03-playing.md)） |
| `verbose` | 1 | 表示の量（0〜4。[3.10](03-playing.md)） |
| `search-log-file` | なし | 探索の記録を書き出すファイル |
| `ui-log-file` | なし | 入力と表示の記録を書き出すファイル |
| `auto-start`・`auto-swap`・`auto-store`・`auto-quit`・`repeat` | `off`・0 | 続けて対局するときの動作（[3.9](03-playing.md)） |

全部の設定は、[14 章](14-reference.md)の表にあります。

## 4.7 設定の決め方の目安

| 使い方 | 設定 |
|---|---|
| 対局・解析だけに使う | 配布物の `config.ini` のままで使えます。強さは `level` で調整します |
| ほかの作業をしながら使う | `n-tasks` を、論理 CPU の数の半分くらいにします（PC の動きが重くなりにくい） |
| 結果を毎回同じにしたい（検証など） | `n-tasks = 1` にして、`hash-table-size` を数で固定します（[5.6](05-search.md)） |
| book を育てる | [7 章](07-book-learning.md)と [9 章](09-large-books.md)を読んでから、`book-expand-tasks`・`book-store-tasks`・`book-save-interval` を決めます |
| 以前の版（元の Edax）とまったく同じ動きで book を育てたい | `book-expand-tasks = 1`、`book-store-tasks = 1` にします |

次：[5. 探索の基礎](05-search.md)
