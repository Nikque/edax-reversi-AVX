# eval2 テストと実測の記録

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
