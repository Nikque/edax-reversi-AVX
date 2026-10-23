# 12. ほかのプログラムから使う

[目次](README.md) ｜ 前：[11. 問題を解く・速さを測る](11-solve-bench.md) ｜ 次：[13. 困ったときは](13-troubleshooting.md)

Edax は、文字で操作するプログラムですが、ほかのプログラムの「頭脳」（思考エンジン）としても使えます。方法は 3 つあります。

| 方法 | 向いている用途 |
|---|---|
| **GUI のソフトとつなぐ**（12.1） | 盤を絵で見ながら、マウスで対局・解析したい |
| **ライブラリ（libedax）を呼ぶ**（12.2） | 自分でプログラムを書いて、Edax の機能を使いたい |
| **edax_runner を使う**（12.3） | 学習させたい手順をリストに書いて、book を自動で育てたい |
| **book の解析の命令**（12.4） | GUI のソフトから、book の精度の段階・うみがめ数・局面の数などを数えさせたい |

## 12.1 GUI のソフトとつなぐ

GUI のソフトは、Edax を裏で起動し、決まった言葉（**プロトコル**）で「この局面で打て」「この手を打った」とやり取りします。Edax は、次のプロトコルを話せます。

| 起動のときのオプション | プロトコル | 相手 |
|---|---|---|
| （なし） | Edax 自身の言葉（このマニュアルで説明しているコマンド） | 人、または Edax のコマンドを直接送るプログラム |
| `-gtp` | GTP（Go Text Protocol） | GTP に対応した GUI |
| `-nboard` | NBoard | NBoard |
| `-xboard` | XBoard（WinBoard） | リバーシに対応した XBoard・WinBoard |
| `-cassio` | Cassio のエンジンの言葉 | Cassio（macOS） |
| `-ggs` | GGS（インターネットの対局サーバー）に、Edax 自身がつなぐ | GGS |

オプションを付けなくても、Edax は、最初に受け取った言葉が次のどれかなら、そのプロトコルに切り替わります：`nboard 1`（NBoard）、`xboard`（XBoard）、`protocol_version`（GTP）、`engine-protocol init`（Cassio）。

**つなぎ方の基本**（GUI の側の設定）

1. GUI の「エンジンを登録する」画面で、Edax の実行ファイル（`bin` の中の `wEdax-x86-64.exe` など）を指定します。
2. **作業フォルダー（実行するときのフォルダー）を、`bin` にします。** Edax は、そこから `data/eval.dat` を探します（[2.1](02-quick-start.md)）。作業フォルダーを指定できない GUI では、起動のオプションに `-eval-file` と `-book-file` を、フォルダーを含めた名前で書きます。
3. 必要なら、起動のオプションにプロトコル（`-gtp` など）と、強さ（`-l 18`）、スレッドの数（`-n 4`）を書きます。`config.ini` の設定は、GUI から起動したときにも読まれます。

**このマニュアルの作成で確かめたこと**：GUI のソフトそのものとの接続は、試していません。確かめたのは、3 つのプロトコル（GTP・NBoard・XBoard）で起動して、基本の言葉に答えることだけです。

GTP の例（`>>>` の行が送った言葉、その下が Edax の答え）：

```
>>> protocol_version
= 2

>>> name
= Edax

>>> boardsize 8
=

>>> play black f5
=

>>> genmove white
= d6
```

GTP で使える言葉は、`list_commands` で表示されます：`protocol_version`、`name`、`version`、`known_command`、`list_commands`、`quit`、`boardsize`、`clear_board`、`komi`、`play`、`genmove`、`undo`、`time_settings`、`time_left`、`set_game`、`list_games`、`loadsgf`、`reg_genmove`、`showboard`。

NBoard の例：

```
>>> nboard 1
>>> set depth 4
set myname Edax4
>>> go
status Edax is thinking
nodestats 257 0.00
=== d6 2.00 0.0
status Edax is waiting
```

XBoard の例（`protover 2` に、対応している機能を答える）：

```
>>> protover 2
feature setboard=1 playother=1 ping=1 draw=0 sigint=0 sigterm=0 analyze=1 myname="Edax 4.5.5" variants="reversi" colors=0 nps=1 memory=1 smp=1 done=1
```

- プロトコルで動いているときも、対局の強さは `level`（NBoard の `set depth`、XBoard の `sd` でも指定できます）と持ち時間で決まり、book も使われます。
- 配布物の `bin/README.MS-Windows.txt`（元の Edax の説明。英語）に、NBoard と WinBoard とのつなぎ方が書いてあります。古い説明なので、ファイル名などは今の配布物と違います。
- GGS（`-ggs`）は、設定 `ggs-host`・`ggs-port`・`ggs-login`・`ggs-password` で接続先を決めます（このマニュアルの作成では試していません）。

## 12.2 ライブラリ（libedax）

**libedax** は、Edax の機能を、ほかのプログラムから関数として呼べるようにしたものです（lavox 氏が Edax 4.4 向けに作ったライブラリと、同じ関数を持ちます）。libedax 向けに書かれたプログラムは、ライブラリのファイルを差し替えるだけで、この修正版の Edax を使えます。

配布物の `bin/` に入っているライブラリ：

| ファイル | 環境 |
|---|---|
| `libedax-x64.dll` | Windows（64 ビット）。どの CPU でも動く |
| `libedax-x64-v3.dll`・`libedax-x64-v4.dll` | Windows。AVX2・AVX-512 に対応した CPU 用 |
| `libedax-x86-64.so`・`-v3.so`・`-v4.so` | Linux |
| `libedax.universal.dylib` | macOS（Apple silicon と Intel の両用） |
| `libedax-arm64-v8a.so`・`libedax-armeabi-v7a.so` | Android（ビルドの確認だけで、実機では動かしていません） |

使い方の流れ：

1. ライブラリと `data/eval.dat`（必須）を、自分のプログラムと同じフォルダーに置きます（`data/book.dat` と `config.ini` は、必要なら）。
2. C・C++ からは、ソースの `src/libedax.h` を取り込みます。ほかの言語からは、その言語の「外部の関数を呼ぶしくみ」でライブラリを読み込みます（Dart には libedax4dart というパッケージがあります）。
3. 最初に `libedax_initialize` を呼び（ここで設定が読まれます）、`edax_init` で対局を始め、最後に `libedax_terminate` を呼びます。
4. 関数は、Edax のコマンドに対応しています（`edax_play`・`edax_go`・`edax_hint`・`edax_book_deviate` など）。

短い例（ソースの `tests/libedax_example.c`。説明の文だけ、日本語にしてあります）：

```c
#include <stdio.h>
#include "libedax.h"

int main(void)
{
	char *args[] = {"", "-eval-file", "data/eval.dat", "-book-file", "data/book.dat", "-level", "12", "-n-tasks", "2"};
	char moves[] = "f5d6c3";
	static LibedaxHintList hints;
	LibedaxMove last;
	int i;

	libedax_initialize(9, args);  /* edax.ini、config.ini、この引数の順に設定を読む */
	edax_init();                  /* 新しい対局 */
	edax_play(moves);
	edax_hint(2, &hints);         /* 良い手を 2 つ：hint[1] 〜 hint[n_hints] */
	for (i = 1; i <= hints.n_hints; ++i)
		printf("%c%c %+d\n", 'a' + hints.hint[i].move % 8, '1' + hints.hint[i].move / 8, hints.hint[i].score);
	edax_go();                    /* Edax が 1 手打つ */
	edax_get_last_move(&last);    /* マスの番号：A1 = 0、B1 = 1、…、H8 = 63 */
	libedax_terminate();
	return 0;
}
```

知っておくこと：

- 1 つのプロセスで使える Edax は、1 つです。関数は、1 つのスレッドから順に呼びます（実行中の処理を止める `edax_stop` と `edax_book_stop_count_bestpath` だけは、別のスレッドから呼べます）。
- book は、メモリに読み込んで使い、保存のときにファイルを置き換えます。同じ book のファイルを 2 つのプロセスで学習して保存すると、後に保存した方だけが残ります。1 つの book のファイルに書くプロセスは、1 つにします。
- 設定は、Edax 本体と同じ名前・同じ書き方です。
- **book を変える関数と、棋譜のファイルを書く関数（`edax_base_complete`・`edax_base_correct`）の実行中は、`edax_stop` が探索を止めません**（途中で打ち切った探索の結果が、book やファイルに入るのを防ぐためです）。
- `edax_get_moves` が返す手順は、80 手（着手とパスの合計）までです。
- Windows では、ファイル名を UTF-8 として読みます（UTF-8 として正しくないときは、その PC の文字コード）。日本語の名前を、その PC の文字コード（CP932）で渡すと、まれに、別の名前として読まれることがあります。ファイル名は、半角の英数字にしておくのが安全です。
- くわしい説明（元の libedax との違い、追加した関数、ビルドのしかた）は、配布物の `README-NIKQUE.ja.md` の「libedax：Edax をライブラリとして使う」にあります。関数の一覧は `src/libedax.h`、全部の関数を呼ぶ例は `tests/libedax_test.c` です。

## 12.3 edax_runner

**edax_runner**（`Nikque/edax_runner`。sensuikan1973 氏の edax_runner を、この修正版の libedax で動くようにしたもの）は、**学習させたい手順をリストに書いておくと、上から順に book に学習させていく**小さなプログラムです。Edax の画面でコマンドを打つ代わりに、文字のファイルを書くだけで済みます。

使い方（edax_runner の README より）：

1. edax_runner の Release から、配布物をダウンロードして展開する。
2. `learning_list.txt` に、学習させたい内容を書く。
3. `config.ini` を、好みに合わせて書き換える（書き方は Edax と同じ。[4 章](04-settings.md)）。
4. 育てたい book があれば、`data/book.dat` に置く。
5. edax_runner を起動する。
6. 学習が済んだ行は、`learned_log.txt` に記録される。

`learning_list.txt` の書き方：

| 目的 | 書き方 | 例 |
|---|---|---|
| その手順から、Edax どうしで 1 局打って学習する（`book learn` の 1 行と同じ） | `book-randomness の値,手順` | `2,F5F6F7F8` |
| その手順の局面から `book deviate` を行う | `[相対誤差 絶対誤差] 手順` | `[1 1] F5F6F7F8` |
| `book fix` を行う | `fix` | `fix` |
| 説明 | `// 説明` | `// ここから虎定石` |
| edax_runner を終わらせる | `exit` | `exit` |

- `2,` を省いて手順だけを書くと、`0,手順` と同じです。
- edax_runner は、1 行（同時に学習する場合は、その 1 組）が終わるたびに、book を `data/book.dat` に保存し、リストを読み直します。実行中にリストを書き換えてもかまいません。
- 同じフォルダーで 2 つ目の edax_runner を起動すると、何もせずに止まります（5.3.0+nikque.10 から）。
- 設定 `book-store-tasks` が 1 以外のとき（既定の `auto` を含む）は、「Edax どうしで 1 局」の行を、その数ずつまとめて、同時に学習します。
- この章のこれ以外の内容と、edax_runner の動作そのものは、このマニュアルの作成では確かめ直していません（edax_runner の試験は、別に行っています）。くわしくは、edax_runner の `README.ja.md` を見てください。

## 12.4 GUI のための book の解析の命令

GUI のソフトが、book をたどって数える計算を Edax に任せるための命令です（v4.5.5-nikque.19 から）。

- どれも、ほかの命令と並んで裏で動きます。解析の間も、`hint` や手を打つ命令などを受け付けます。
- book を読むだけで、変えません。book を変える命令（`book store` など）を送ると、Edax は先に解析を止めます。
- 番号 `id=…` を付けて送ります。答えは `@` で始まる行で、1 行ずつ行の終わりまで一度に書きます。ほかの表示（警告など）が書きかけのときは、その行の途中に入ることがあります（`WARNING: @end id=5 ms=0` のように。表示の残りは次の行）。値は、今の局面で手番の側から見た石差です。

| 命令 | 答え |
|---|---|
| `book moveinfo id=N` | 全部の合法手について、book の評価値と level、精度の段階、次の段階に足りない Leaf の数、うみがめ数、手の後に book でたどれる局面の数 |
| `book missing id=N` ［`sq=手`］ | 次の段階に足りない Leaf までの手順の一覧（`sq=` を書くと、その手の後から） |
| `book levels id=N` | 今の局面から先の局面の、level ごとの数（`scope=all` は book 全体） |
| `book bestlines id=N` | 両方が最善の手（`loss=` の石損までなら次善の手も）だけを打つ進行の数。`file=` を書くと、進行の端までの手順を書き出す |
| `book findline id=N target=局面` ［`n=`］ | 今の局面から `target` の局面までの、book の局面だけを通る手順（評価値の変わり方がいちばん小さいものから `n` 本、既定は 1 本） |
| `book reducecount id=N` ［`x=`・`y=`］ | `book reduce X Y`（[8.4](08-book-maintenance.md)）を今の局面で打ったときに残る局面と消える局面の数（book は変えない。v4.5.5-nikque.20 から） |
| `stop id=N` | 解析 N を止める（ほかの解析と、待っている命令は残る） |

**精度の段階**（`stage=`）

| 段階 | 意味 |
|---|---|
| 0 | 手の後の局面が book にない |
| 1 | book にあるが、両方が最善の手だけを打っても、完全読みの局面に届かない |
| 2 | 届くが、段階 3 の範囲が終わっていない |
| 3〜7 | その手の後の局面で `book deviate 1 1`〜`book deviate 5 5` を打っても、何も広げない |

- 基準の命令は `base=deviate2`・`base=deviate3` に、範囲は `stages=1/1,2/2,3/3,4/4,5/5` の形で変えられます。

**うみがめ数**（`umi=黒:白`）

- その手の後で、条件を満たす全部の進行を覚えるのに、最小で何通りの進行の端を覚えればよいか、を黒と白それぞれについて数えた数です。覚える側の番では 1 手を選べばよく、相手の番では全部の手に備えます。
- 別の手順から同じ局面に来たときは、その先を 1 回だけ数えます。
- 条件は、既定が両方の最善の手だけです。`umi=loss x=… y=…`（1 手の石損と合計の石損）・`umi=range lo=… hi=…`（評価値の範囲）でも数えられます。範囲が広いと、最小とは限らない値に `exact=0` が付きます。

例（試験の book）：

```
>book moveinfo id=1
@begin id=1 ply=0 turn=black moves=4 book=1
@move id=1 sq=d3 book=1 value=-2 level=4 lines=505 stage=1 missing=1
...
@move id=1 sq=d3 part=umi umi=1:1
...
@move id=1 sq=d3 part=count positions=1546
...
@end id=1 ms=0
```

- `book moveinfo` は、book にない手、精度の段階、うみがめ数（`part=umi`）、局面の数（`part=count`）の順に答えます。大きな book の序盤では局面の数に時間がかかるので、`parts=stage,umi` のように一部だけを頼むこともできます。`lines=` は、book に書かれた終わっていない進行の数です（すぐ出ます。合流は進行ごとに数えます）。
- `book bestlines` の答えは `@end id=N lines=… ends=… incomplete=… incomplete_ends=… positions=… written=…` の 1 行です。`lines` は進行ごと、`ends` は端の局面ごとの数、`incomplete` は完全読みに届かない端の数です。`file=` のファイルには、端の局面ごとに 1 行、今の局面までの手順から端までの手順を書きます（`which=incomplete` は完全読みに届かない端だけ）。
- `book findline` の `target=` は、a1〜h8 の 64 マス（`X` 黒・`O` 白・`-` 空き、`setboard` と同じ）の後に手番（`X` か `O`）を続けた 65 文字です。今の局面と `target`、その間の局面がすべて book にある手順を探します（`target` の石のあるマスにだけ打ち、打てる手がなく相手が打てるときはパス `pa`）。手順の順は、各手で book の評価値（黒から見た値）が変わった幅の合計の小さい順です。答えは手順ごとに `@line id=N moves=f5d6c3… cost=…`、最後に `@end id=N count=… paths=… positions=…` です（`paths` は手順の全部の数、`positions` はたどった局面の数）。`target` が book にないときは `notinbook=1`、今の局面が book にないときは `startnotinbook=1` が付き、`count=0` です。`n=` は 1〜100 です。
- `book reducecount` の答えは `@end id=N kept=… removed=… positions=… ms=…` の 1 行です（`kept` は残る局面、`removed` は消える局面、`positions` は book 全体の局面の数）。`x=`・`y=` は `book reduce X Y` の X（0〜129、既定 2）と Y（0〜254、既定 4）です。今の局面から先の評価値を、book を変えずに計算し直してから数えるので（`book reduce` と同じ）、`book reduce` の後の局面の数と同じになります。今の局面が book にないときは `notinbook=1`、book の深さより 2 つ以上先のときは `deep=1` が付き、`kept=` は book 全体、`removed=0` です（`book reduce` は book を変えません）。今の局面から先の book が壊れているとき（すでに石のあるマスへの手や、同じ局面に戻る手のつながりがあるとき。v4.5.5-nikque.21 から）は、数えずに `@error id=N damaged=1` と `@end id=N error=1` を返します。GUI のソフトは、これで数を見せて確かめてから `book reduce` を送れます。
- 引数の誤りには `@error id=N word=…` と `@end id=N error=1`、止めた解析には `@cancel id=N` と答えます。番号 `id=` が整数（int の範囲）でないときは、解析も `stop` も行わず `@error word=id` とだけ答えます。命令の語は空白で区切り、`…` の中は空白も含めて 1 つの値です（その中の `id=` は番号ではありません）。名前（`id=` など）の大文字・小文字は区別しません。`stop id=N` が、終わった後の解析や、まだ受け付けていない解析に届いたときは `@cancel id=N state=none` と答えます（終わった解析は、その前に `@end id=N` を答えています。受け付ける前の解析は、その後で最後まで行い、`@end id=N` を答えます）。
- **このマニュアルの作成で確かめたこと**：小さな試験の book（自分で作ったもの、最大 1,914 万局面）で、答えを別に書いた計算と照らし合わせ、精度の段階は実際に `book deviate` を打って確かめました。`book reduce`・`book reducecount` は、別に書いた計算（`book deviate3` の歩き方と評価値の計算し直し）と、残る局面・消える局面・残った局面の中身・その後の `book deviate3` が最初に選ぶ Leaf（別の計算で求めたもの）を照らし合わせました（4 つの試験の book、546 通り）。壊れた book 2 つと誤った語の `book reduce` も試しました（v4.5.5-nikque.21 から）。GUI のソフトそのものとの接続は、試していません。

次：[13. 困ったときは](13-troubleshooting.md)
