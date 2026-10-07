# 8. book の保守

[目次](README.md) ｜ 前：[7. book を育てる](07-book-learning.md) ｜ 次：[9. 大きな book を扱う](09-large-books.md)

この章は、できた book を「直す・まとめる・切り詰める・書き出す・評価値を計算し直す」コマンドの説明です。

**どのコマンドも、メモリの中の book を変えます。** ファイルに残るのは、`book save` を打ったとき、コマンドが自分で保存するとき（[6.6](06-book-basics.md)の表）、そして Edax を終了したとき（book が変わっていれば `book-file` へ）です。**やり直しはできません。** 大事な book を保守する前に、ファイルの写しを取っておいてください。

## 8.1 保守のコマンドの見取り図

| したいこと | コマンド | 節 |
|---|---|---|
| 評価値を、下から上へ伝え直す | `book negamax` | 8.2 |
| 欠けている Link を張り、おかしな局面を直す | `book fix` | 8.2 |
| 2 つの book を 1 つにまとめる | `book merge` | 8.3 |
| book を浅くする・ある定石の先だけ残す | `book depth`＋`book subtree` | 8.4 |
| 悪い手ばかりの変化を捨てる | `book prune` | 8.4 |
| 文字のファイルに書き出す・読み戻す | `book export`・`book import` | 8.5 |
| `eval.dat` を替えた後などに、Leaf の評価値を計算し直す | `book leaf-recalculate` 系 | 8.6 |
| 完全読みの局面の値を確かめ直す | `book correct` | 8.7 |
| book から棋譜や問題を取り出す | `book extract`・`book problem` | 8.8 |

## 8.2 negamax と fix

### book negamax

```
>book negamax
Negamaxing book...done
Sorting book...done>
```

初期局面から Link をたどって、全部の局面の評価値を、下の局面の値から計算し直します。その後、各局面の手を良い順に並べ替えます。

- book を変えるコマンドは、自分で negamax を行うので、ふつうは自分で打つ必要はありません。打つのは、途中経過のファイル（とくに `book leaf-recalculate` の途中の保存）を読み込んだ後などです。
- 初期局面から Link でたどれない局面（どの局面からも Link が張られていない、はぐれた局面）は、計算し直されません。

### book fix

```
>book fix
Fixing book...
Fixing book...0 done
Linking book...
Searching positions...
Searching positions...6 done
Linking book...2246 done
Negamaxing book...done
Sorting book...done>
```

book 全体を点検して、整えます。順に、次のことを行います。

1. **点検**（`Fixing book`）：中身のおかしい局面（打てない手が記録されている、など）を見つけて、作り直す。`0 done` なら、おかしな局面はなかった。
2. **Link の張り直し**（`Linking book`）：どの局面についても、「打つと book の中の別の局面に行く手」が Link になっているようにする。Leaf の手が Link になった局面は、新しい Leaf を探索して求める（`Searching positions`）。
3. negamax と並べ替え。

`book fix` が役に立つ場面：

- **`book-expand-tasks` を 2 以上（`auto` を含む）にして育てた後**。同時に広げた局面どうしは互いを見ないので、「別の手順でも同じ局面に行ける」ときの Link が欠けていることがあります（[7.6](07-book-learning.md)）。`book fix` で張られます。
- ほかの人の book や、古い版で作った book を使い始めるとき。
- book の様子がおかしいとき（Edax が book の途中で止まる、など）。

大きな book では時間がかかります（[9 章](09-large-books.md)）。局面を作り直すときは、その局面を book の level で探索します。

## 8.3 book をまとめる：book merge

```
>book load data/a.dat
>book merge data/b.dat
Checking book data/b.dat...
Merging book data/b.dat...
Merging book data/b.dat...1 positions added
Linking book...
 …
Negamaxing book...done
Sorting book...done>
Merged book saved to data/my.dat.mrg
```

今の book（統合先）に、別の book のファイル（統合元）を取り込みます。

- **統合元にしかない局面だけ**を足します。両方にある局面は、統合先のものを残します（上書きしません）。
- 足した後、Link の張り直し・点検・negamax・並べ替えまでを行います（`book fix` を別に打つ必要はありません）。
- 設定 `book-merge-auto-save = on`（既定）なら、結果を `<book のファイル名>.mrg` に自動で保存します。元の book のファイルは、この保存では上書きされません。`.mrg` を使うには、`book load` で読むか、名前を変えます。
- 統合元のファイルがない・壊れているときは、何も取り込まずに、今の book を保ちます。

  ```
  cannot open data/nofile.dat
  WARNING: Book data/nofile.dat was not merged
  ```
- 統合元は、メモリに読み込まれません（ファイルを 2 回読みます）。使うメモリは、統合先の book の分だけです。

**気を付けること**

- 2 つの book は、**同じ level** で育てたものにしてください。level が違うと、まとめた book の中で評価値の確かさがまちまちになり、Link の張り直しで必要になる探索も増えます。
- 同じ局面が両方にあるとき、残るのは統合先の値です。「新しいほう・確かなほうを統合先にする」と覚えてください。
- 別々の PC やフォルダーで同じ book を並行して育て、後でまとめる、という使い方ができます。ただし、この修正版では 1 つの Edax で多くのスレッドを使い切れるので（[7.2](07-book-learning.md)・[7.6](07-book-learning.md)）、その必要は少なくなっています。

## 8.4 book を切り詰める

### book depth と book subtree：浅くする・一部だけ残す

`book subtree` は、**今の盤の局面から、Link をたどって行ける局面だけ**（book の深さの範囲のもの）を残し、ほかを全部消します。

**使い方 1：book を浅くする**（例：深さ 40 の book を 39 にする）

```
>book depth 39
>init
>book subtree
```

`book depth 39` で深さの設定を変え、`init` で初期局面に戻してから `book subtree` を打つと、深さ 39 より先の局面が消えます。初期局面からたどれない、はぐれた局面も消えます。

**使い方 2：ある定石の先だけを取り出す**

```
>play f5d6c3d3
>book subtree
{board:--------------------OX-----XO-----XXO-------O------------------- X; level:8; best: +0 [-2, +2];moves: [F4:+0] <F6:-5>}
Book subtree 13... done
done
 …
>book save data/sub.dat
```

今の盤の局面と、その先だけが残ります。**初期局面からそこまでの局面も消える**ので、できた book は、その局面から始める研究用です（初期局面で `book show` を打っても、何も表示されなくなります）。元の book を残したいなら、`book save` で**別の名前**に保存し、終了する前に元の book を読み直してください（下の注意）。

- 今の盤の局面が、book の深さより 2 つ以上先のときは、警告が出て、何もしません（v4.5.5-nikque.13 から。それより前の版では、book が空になっていました）。

  ```
  WARNING: book subtree: this position is deeper than the book depth; the book is not changed
  ```
- `book subtree` は、切り詰めた後に Link の張り直しを行いません（v4.5.5-nikque.12 から）。必要なら、前か後に `book fix` を打ちます。

### book prune：悪い手ばかりの変化を捨てる

```
>book prune
{board:---------------------------OX------XO--------------------------- X; level:8; best: +0 [-2, +2];moves: [D3:+0] [C4:+0] [F5:+0] [E6:+0]}
Book prune 808... done
Book prune 1448... done
done
 …
>book info
Positions: 1269 (moves = 1296 links + 1265 leaves);
```

初期局面から、「**片方の側はどんな手でも打つが、もう片方は最善の手だけを打つ**」という手順でたどれる局面だけを残します（黒がどんな手でも打つ場合と、白がどんな手でも打つ場合の両方）。両方の側が最善でない手を打った先の局面は、消えます。上の例では、2,262 局面が 1,269 局面になりました。

対局で使う book を小さくしたいときに使います（相手がどう打っても、自分は最善で応じられる範囲が残ります）。研究用に「両者の間違い」まで育てた book（`book deviate2` で広げたもの）に使うと、その大半が消えます。

### 切り詰めるときの注意

**v4.5.5-nikque.13 から、`book subtree`・`book prune` で切り詰めた book は、そのまま Edax を終了すると、`book-file` のファイルに保存されます**（切り詰める前の book が、上書きされます）。

- 切り詰める前の book を残したいときは、**先にファイルの写しを取る**か、切り詰めた book を `book save 別の名前` で保存してから、終了する前に `book load` で元の book を読み直してください。
- `book depth` で深さの設定を変えただけでは、保存されません。

## 8.5 文字のファイルに書き出す・読み戻す

```
>book export data/my.txt
>book import data/my.txt
```

`book export` は、book を、1 行に 1 局面の文字のファイルに書き出します。

```
----------X--O----XXOX--XXXOO----OOOO-----O--------------------- X,8,F4,5
```

行の中身は、盤（64 マス。**手番の側の石が `X`、相手の石が `O`** で、手番の欄はいつも `X`）、level、Leaf の手、その評価値です。Link は書き出されません（取り込むときに張り直されます）。

ファイルの最後の行には、book の深さが書かれます（v4.5.5-nikque.13 から）。

```
% depth 12
```

`book import` は、そのファイルを読んで、**今の book と入れ替えます**。読んだ後、Link の張り直し・点検・negamax・並べ替えを行います。深さは、最後の行の `% depth` のとおりになります（深さ 12 の book を書き出して取り込むと、`Depth: 12`）。取り込んだ book は、Edax を終了するときに `book-file` に保存されます。それまでの `book-file` のファイルは、`.old` を付けた名前で残ります（[6.6](06-book-basics.md)の注意 2）。

- ファイルが開けない、局面が 1 つもない、というときは、今の book を保ちます（`was not imported; current book retained` と表示）。局面として読めない行は、飛ばして続けます。
- Link を張り直すので、取り込んだ book は、書き出す前の book と、一部の Leaf が違うことがあります（2,246 局面の book で試すと、取り込んだ後にもう一度書き出したファイルは、7 行が違いました）。
- **`% depth` の行がないファイル**（v4.5.5-nikque.12 までの版が書き出したファイルや、自分で作ったファイル）を取り込むと、深さは、**いちばん深い局面がちょうど入る深さ**になります。book を、決めた深さまで育て切っていなかったときは、元より浅くなります（深さ 30 で作って 3 手目までの 4 局面しかない book を、古い版で書き出して取り込むと、`Depth: 3`）。取り込んだ後に `book info` で深さ（Depth）を確かめ、違っていれば `book depth 30` のように打って直してください。`% depth` の後ろが 1〜60 の数でないときは、`WARNING: wrong depth: …` と表示されて、行がないときと同じになります。
- **v4.5.5-nikque.12 までの版で取り込むと**、深さは、それより 1 つ大きくなります（深さ 12 まで育った book なら `Depth: 13`）。これらの版は、新しい版が書き出したファイルの最後の行を読めないので、`WARNING: wrong board: % depth 12` と `WARNING: 1 lines of … hold no position: skipped` を表示して、その行を飛ばします（局面は、全部読まれます）。
- 大きな book では、文字のファイルは book のファイルよりずっと大きくなります（上の小さい book で、99KB に対して 170KB）。ふだんの保存には `book save` を使ってください。

## 8.6 Leaf の評価値を計算し直す：book leaf-recalculate

book の中の Leaf の評価値は、その Leaf を作ったときの探索の値です。`eval.dat` を別のものに替えると、新しく足す局面は新しい評価関数の値になりますが、**前からある Leaf は古い値のまま**です。`book leaf-recalculate` の仲間は、Leaf を探索し直して、今の評価関数の値に置き換えます。局面は足しません。

| コマンド | たどる範囲 | 計算し直す Leaf |
|---|---|---|
| `book leaf-recalculate X Y` | `book deviate X Y` と同じ | `book deviate X Y` が「広げる」と選ぶ Leaf |
| `book leaf-recalculate2 X Y` | `book deviate2 X Y` と同じ | `book deviate2 X Y` が「広げる」と選ぶ Leaf |
| `book leaf-recalculate3 X Y` | `book deviate X Y` と同じ | たどった局面の全部の Leaf |
| `book leaf-recalculate4 X Y` | `book deviate2 X Y` と同じ | たどった局面の全部の Leaf |

- X・Y の意味と、今の盤から始めることは、対応する `book deviate`・`book deviate2` と同じです（[7.3](07-book-learning.md)）。
- **完全読みの局面の Leaf は、計算し直しません**（評価関数によらず同じ値だからです）。Leaf のない局面（全部の手が Link）も対象外です。
- `3`・`4` は、いちばん深い層（Link がなく Leaf だけの局面）の Leaf も計算し直します。
- 探索は、その局面に記録されている level で行います。同時に行う数は、`book-expand-tasks` に従います。
- 途中経過と結果は、`<book のファイル名>.leaf`・`.leaf2`・`.leaf3`・`.leaf4` に保存されます。**途中で保存されたファイルは、「一部の Leaf だけ新しく、negamax はまだ」の状態**なので、読み込んで使うときは `book negamax` を打ってください。続きから再開するしくみはありません（もう一度打つと、最初から計算し直します）。

表示の例（小さい book）：

```
>book leaf-recalculate4 2 4
Book leaf-recalculate4 2 4:
Book leaf-recalculate4 123 todo
Book leaf-recalculate4...
Book leaf-recalculate4 2 4...finished: 123 leaves, 0 scores changed (0 up, 0 down, largest +0 / -0), 0 moves changed
```

最後の行は、計算し直した Leaf の数、評価値が変わった数（上がった数・下がった数・いちばん大きな変化）、手が変わった数です。

### 1 回では足りないことがある：book-leaf-recalculate-rounds

このコマンドは、既定では **1 回たどって終わり**ます。ところが、Leaf を計算し直すと評価値が動き、「たどる範囲」も動くので、同じコマンドをもう一度打つと、1 回目には範囲の外だった Leaf が、新しく対象になります。

設定 `book-leaf-recalculate-rounds = n`（v4.5.5-nikque.13 から）にすると、「たどる → 計算し直す → negamax」を最大 n 回繰り返し、**Leaf が 1 つも変わらなかった回で終わります**。既定は 1 です。

- 配布物の README の例（649 万局面の book、`book leaf-recalculate4 1 1`）：1 回ずつ繰り返すと、対象は 4,066 → 780 → 247 → 485 → 386、評価値が変わった数は 1,419 → 411 → 42 → 46 → 0。`book-leaf-recalculate-rounds = 10` にすると、5 回で終わりました。
- 複数スレッドの探索は実行ごとに値が少し変わるので、「変わらなかった回」が来ないことがあります。そのときは n 回で終わります。
- **`eval.dat` を替えていなくても、計算し直すと値が変わる Leaf があります**（作ったときの探索と、後から単独で行う探索では、条件が少し違うため）。くわしい数字は、README の v4.5.5-nikque.12・13 の節にあります。

### eval.dat を替えたときの手順の例

1. book のファイルの写しを取る。
2. 新しい `eval.dat` で Edax を起動する（`-eval-file` か、ファイルの置き換え）。
3. `init` の後、`book leaf-recalculate4 X Y`（X・Y は、ふだん `book deviate2` に使っている数）を打つ。
4. 終わったら `book info`・`book show` で確かめ、`book save` で保存する。

時間は、Leaf の数に比例します。README の実測（6 億 6,162 万局面の book、level 18、32 スレッド）では、`book leaf-recalculate2 5 5` の対象が 275 万件で、探索の速さは 1 分に約 2,350 件でした（全部を行った時間は測られていません）。

## 8.7 そのほかの保守

### book correct：完全読みの局面を確かめ直す

```
>book correct
Correcting solved positions...
Correcting solved positions...0 done (0 error found)
 …
```

完全読みの範囲にある局面（その局面の level で、空きの数が完全読みになるもの）の Leaf を探索し直し、記録と違っていたら直します。その後、`book fix` と同じ処理を行います。直した数が `error found` に出ます。途中経過は `.err` に保存されます。

正しく作られた book なら、`0 error found` になるはずのコマンドです。古い版の不具合や、途中で止めた探索の結果が入った疑いがあるときに使います。

**v4.5.5-nikque.12 までの版では、このコマンドが、終局の局面とパスの局面の Leaf を消して、book の評価値を壊していました**（`error found` に大きな数が出て、評価値が ±127 になる）。v4.5.5-nikque.13 で直っています。古い版で `book correct` をかけて壊れた book は、v4.5.5-nikque.13 の `book correct` をかけると直りました（試験用の小さい book での確認です）。

### book deepen：使わないでください

組み込みの説明には「book の level を変えて、全体を評価し直す」とありますが、**そのとおりには働きません**（局面の level が book の level と違う局面の Leaf を探索し直しますが、探索は「その局面の今の level」で行い、level も書き換えません。元の Edax からの作りです）。book の level を上げたいときは、新しい level の book を作り直すことになります。

### book verbose：表示の量

`book verbose 0` で book のコマンドの表示をなくし、`1`（ふつう）で進み具合を表示し、`2` で探索の表も表示します。

### book feed-hash

今の盤の局面から先の、book の評価値を、探索のハッシュ表に入れます（次の探索が、book の値を使えるようにします）。何も表示されません。

## 8.8 book から取り出す

### book extract：最善の手順を棋譜にする

```
>book extract data/lines.txt
49 games extracted
```

初期局面から、各局面で「評価値がいちばん良い手」（同点なら全部）をたどった手順を、棋譜のファイルに書き出します。形式は拡張子で決まります（[10 章](10-files.md)）。

- **出力先のファイルがすでにあると、その後ろに足されます。**
- 新しいファイルのときは、`WARNING: Cannot open file data/lines.txt` と表示されますが、ファイルは作られます（「前からあるファイルを読もうとして、なかった」という意味の表示です）。

### book problem：局面を問題として取り出す

```
>book problem 52 3
Extracting 3 positions at 52 ...
-----------------OOOO----XOXXX----OOO--------------------------- X % bm E2:+1; ba C6:-2;
```

book の中から、空きが 52 で、最善の手が 1 つに決まっている局面（2 番目の手より評価値が良い局面）を、3 つまで画面に表示します（数を省くと、空き 24・10 個。選ばれるのは、book の中の並びで先にあるものです）。行の中身は、盤、手番、`bm`（最善の手と評価値）、`ba`（2 番目の手と評価値）です。

## 8.9 この章のコマンド

| コマンド | 意味 |
|---|---|
| `book negamax` | 評価値を計算し直して、手を並べ替える |
| `book fix` | 点検、Link の張り直し、negamax、並べ替え |
| `book merge ファイル` | 別の book を取り込む |
| `book depth n` | 深さの設定を変える |
| `book subtree` | 今の盤の局面から先だけを残す |
| `book prune` | 片方が最善を打つ変化だけを残す |
| `book export ファイル`・`book import ファイル` | 文字のファイルに書き出す・読み戻す |
| `book leaf-recalculate`（`2`・`3`・`4`）`X Y` | Leaf の評価値を計算し直す |
| `book correct` | 完全読みの局面を確かめ直す |
| `book deepen` | （使わない） |
| `book extract ファイル` | 最善の手順を棋譜のファイルに書き出す |
| `book problem 空き 個数` | 局面を問題の形で表示する |
| `book feed-hash` | book の値をハッシュ表に入れる |
| `book verbose n` | 表示の量 |

次：[9. 大きな book を扱う](09-large-books.md)
