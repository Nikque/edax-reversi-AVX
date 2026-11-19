# eval2 テストと実測の記録

## v4.5.5-eval2.18

- 度外石の再現テストで、正解 +10 に対して修正後の境界が +10 になりました。
- Windows 版の 5 種の実行ファイルは、fforum-20-39 の 20 局面で着手・スコアの誤り 0、既知の 1,205 局面でスコアの誤り 0 でした。Cassio EOF の 7 項目も通りました。
- Windows 版の DLL 3 種で API の 279 項目がそれぞれ通りました。
- BMI2 の最終手は 71,168 局面を独立した実装と照合しました。初期評価・差分評価と確定石の回帰テストも通りました。
- [配布ビルドの CI](https://github.com/Nikque/edax-reversi-AVX/actions/runs/44466216151) で Linux・macOS・Android・Windows ARM64 のバイナリをビルドしました。

未確認は、対局での強さ、実 book、SVE の実機での動作と速度です。
