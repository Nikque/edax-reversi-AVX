# 12. ほかのプログラムから使う

[目次](README.md) ｜ 前：[11. 問題を解く・速さを測る](11-solve-bench.md) ｜ 次：[13. 困ったときは](13-troubleshooting.md)

Edax は、文字で操作するプログラムですが、ほかのプログラムの「頭脳」（思考エンジン）としても使えます。方法は 3 つあります。

| 方法 | 向いている用途 |
|---|---|
| **GUI のソフトとつなぐ**（12.1） | 盤を絵で見ながら、マウスで対局・解析したい |
| **ライブラリ（libedax）を呼ぶ**（12.2） | 自分でプログラムを書いて、Edax の機能を使いたい |
| **edax_runner を使う**（12.3） | 学習させたい手順をリストに書いて、book を自動で育てたい |

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
- 設定 `book-store-tasks` が 1 以外のとき（既定の `auto` を含む）は、「Edax どうしで 1 局」の行を、その数ずつまとめて、同時に学習します。
- この章のこれ以外の内容と、edax_runner の動作そのものは、このマニュアルの作成では確かめ直していません（edax_runner の試験は、別に行っています）。くわしくは、edax_runner の `README.ja.md` を見てください。

次：[13. 困ったときは](13-troubleshooting.md)
