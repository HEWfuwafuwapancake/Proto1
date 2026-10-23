# 引き継ぎメモ（今どこまで進んだか・次に何をするか）

別の PC（メインPC・ノートPC）や、別の日の Claude Code・Codex で作業を続けるためのメモです（Claude と Codex のどちらでも続きから始められる）。
**作業を始めるときに読み、作業を終えるときに更新します。** タスクの一覧は [tasks.md](tasks.md)、同じ内容を Notion の
「[Proto1（腕ダフルアームズ）](https://app.notion.com/p/e5e9f1983bbe4758b06b4adba135a491)」データベースでも管理しています。

- 最終更新：2026-10-23（source/repos/Proto1 の PC・Codex。No.13 実装・検証済み、No.14 のプラン作成が次）

---

## 1. 今の状態

| No | タスク | 状態 | PR |
|---|---|---|---|
| 1〜5 | 企画の要点・構成ルール・単体テスト・時間の管理・入力を「行動」に変える仕組み | ✅ 完了 | #3〜#8 |
| **6** | **PS4 コントローラー（DUALSHOCK 4）を USB で読む**（`src/input/pad`・`src/input/padcalc`） | **🔄 マージ済み。実物のパッドでの確認だけ残っている** | [#9](https://github.com/HEWfuwafuwapancake/Proto1/pull/9) |
| 7 | ステージ（浮島）と場外の判定（`src/game/stage`・`src/physics/stagecalc`） | ✅ 完了 | [#10](https://github.com/HEWfuwafuwapancake/Proto1/pull/10) |
| 8 | プレイヤー（`src/game/player`・`src/physics/playercalc`）・形の描画の部品（`src/render/shape`）。回る立方体は消した | ✅ 完了 | [#11](https://github.com/HEWfuwafuwapancake/Proto1/pull/11) |
| 9 | TPS カメラ（`src/game/camera`・`src/physics/cameracalc`）。自動で回り込まない | ✅ 完了 | [#12](https://github.com/HEWfuwafuwapancake/Proto1/pull/12) |
| 10 | 当たり判定の共通部品（`src/physics/collision`・`src/physics/vec3.h`） | ✅ 完了 | [#13](https://github.com/HEWfuwafuwapancake/Proto1/pull/13) |
| 11 | ダミーの敵3体（`src/game/dummy`・`src/physics/dummycalc`）。プレイヤーが押せる・場外から復活 | ✅ 完了 | [#14](https://github.com/HEWfuwafuwapancake/Proto1/pull/14) |
| 12 | ダメージ%と吹っ飛び（`src/physics/knockbackcalc`・`DummyHit`）。F1 の仮攻撃は No.13 で削除 | ✅ 完了 | `tasks.md`・Notion を参照 |
| **13** | **触腕：伸ばして敵をつかみ、向いた方へ投げる** | **✅ 実装・検証完了** | [#18](https://github.com/HEWfuwafuwapancake/Proto1/pull/18) |

- No.13 は [PR #18](https://github.com/HEWfuwafuwapancake/Proto1/pull/18)。マージ状態は GitHub を確認する。
- ゲームは起動すると、空に浮かぶ浮島（30 × 30）の真ん中に赤いカプセルのプレイヤー、前と左右の奥に青いカプセルのダミーが3体立っている（FPS 60）。
  W/A/S/D・左スティックで歩き、Space・×でジャンプ。ダミーにぶつかると押せる。**左右クリック／L1・R1でつかみ、再入力で投げられる**（投げで12%ずつ）、% が高いほど遠くへ吹っ飛ぶ（飛ばされている間は黄色）。
  場外に落ちたダミーは 2 秒後に最初の場所に 0% で戻る。カメラは TPS カメラ。単体テストは Debug / Release とも103個すべて通る。両構成のゲームビルドは警告0。投げ判定を壊したとき追加テストが失敗することも確認済み。
- **No.6 で残っている確認（DUALSHOCK 4 を USB でつないで、人が操作して見る）**：Debug で起動して、出力ウィンドウに「パッド：DUALSHOCK 4 をつなぎました」→ ×・L1・R1・□・OPTIONS で「行動：〇〇」→ 左スティックで歩く・右スティックでカメラが回る → 抜くと「外れました」で、落ちない。
  確かめられたら `tasks.md` と Notion の No.6 を ✅ / 完了にし、`action.cpp` の確かめ用の「行動：〇〇」の出力を消す。Bluetooth は読まない（USB だけ）。
- 人が操作して見る確認（No.5・No.8・No.9・No.11・No.12）：浮島の端から落ちると真ん中に戻る／マウスで回すとカメラがなめらかに回る／ダミーを押して落とすと 2 秒後に戻る／
  触腕で何回か投げると、出力ウィンドウに「ダミー〇：〇%・速さ 〇」と出て、だんだん遠くへ飛ぶ（吹っ飛びの数字は仮。No.19 で調整）。
- **メインPC でまだやっていないこと**：Claude に許可を足す設定（下の「3. 決まったこと」の「Claude の許可」）。

## 2. 次にやること：No.14 試合の流れ（プランを出す）

No.13 は管理者の承認後に実装済み。次は No.14 のプランを出し、確認後に実装する。
- 90秒の制限時間、`DummyGetTotalOutCount()` を使った場外 +1、時間切れ、OPTIONS / R での再開。
- No.15 の表示へ渡す残り時間・スコア・試合状態の取得方法を決める。
- 試合終了と再開時に触腕の保持を解除し、プレイヤー・敵・カメラ・時間を初期化する順序を決める。
- 計算は DirectX なしでテストする。表示の本格実装は No.15。

No.13 の記録：`docs/plan/02_触腕の実装プラン.md`。左右独立、命中から3秒保持、投げはカメラの上下左右方向。F1 の仮攻撃は削除済み。
画面では60 FPSでの起動・保持中の表示を確認。左右連続操作や場外復活などの網羅的な手動試遊、PS4 パッド実機確認は未実施（計算は単体テスト済み）。
ゲーム画面でユーザー操作が検出されたため、入力を止めてゲームを開いたままにした。最終 Debug ビルドは `x64/VerifyDebug/`、Release は通常の出力先。
共有の `source/repos/Proto1` を使う。隣の `Proto1-codex` は未使用で残してある。

## 3. これまでに決まったこと（会話の中で決めたこと）

- **共有フォルダ**：Claude と Codex は同じ `Proto1` を交代で使う。同時には作業しない。切り替える前にコミット・push と引き継ぎメモ更新を行う（2026-10-23、管理者の指示）。
- **AI の役割**：Notion の委譲先・レビュー担当は当面無視し、担当欄は変更しない。実装とレビューの分担は管理者が指定したときに適用する。
- **進め方**：`tasks.md` の優先度：高から順に、1つずつ「プラン → 管理者の確認 → 実装 → テスト → 完了チェック」。1タスク＝1フィーチャーブランチ＝1 PR。
- **マージ**：Claude・Codex がこの流れで作った PR は、テストとビルドが通れば、管理者に聞かずにその AI が `gh pr merge <番号> --merge --admin --delete-branch` でマージしてよい（2026-10-23、管理者が「基本マージして問題ない」と決めた。Codex も同じと 2026-10-23 に決めた）。他の人の PR はマージしない。
- **TPS カメラ**：プレイヤーが歩いても、カメラは自動で後ろに回り込まない（向きはマウス・右スティックで自分で変える。触腕をカメラの向きへ伸ばすので、勝手に回ると狙いにくい。2026-10-23、No.9 で決めた）。
- **ダミー**：3体・その場に立つだけ。プレイヤーがぶつかると押せる（押して落とせば「場外 → 復活」を試せる。2026-10-23、No.11 で決めた）。場外に落ちたら 2 秒後に最初の場所に戻る。
- **ゲームに関係ないサンプルは消す**：回る立方体（`cube`）は No.8 で消した（2026-10-23、管理者の指示）。地形（`field.cpp`）は No.7 で使うのをやめたが、ファイルは残してある。
- **Claude の許可**：Claude Code の自動モードの安全チェックが、マージや `start-work.ps1`（中で develop を `git merge` する）、Notion の更新を止めることがある。
  そのため、管理者の PC だけ `Proto1\.claude\settings.local.json` の `permissions.allow` に次を入れる（GitHub には送らない。`.gitignore` 済み）。
  - `PowerShell(gh pr merge:*)`・`Bash(gh pr merge:*)`：PR のマージ
  - `PowerShell(.\tools\start-work.ps1:*)`：作業を始めるスクリプト
  - Notion の `notion-update-page` ツール（名前の `mcp__〇〇__` の部分は PC ごとに違うので、その PC のツール名で入れる）
  Visual Studio Enterprise の PC は設定済み。**メインPC はまだ**（メインPC の Claude は、管理者に確認してから作る）。
  許可を足しても、ブランチの削除などの「先に確認する」決まりは変わらない。
- **状態の記録**：タスクの状態は `tasks.md` と Notion の両方をそろえる。プランを出したら、このファイルの「次にやること」と Notion のタスクのページの本文に書く。
- **プロトのルール**（`docs/plan/01_企画の要点.md`）：制限時間 90 秒・ダミー3体・場外で +1・L1 / R1 で左右の触腕（キーボードは左右クリック）・□ ではたく（E）・OPTIONS でやり直し（R）。
- **PS4 コントローラー**は外部ライブラリなしで読む（Raw Input と HID）。No.6 のテストのときに USB でつなぐ。
- **Maya のモデル**は No.22 まで使わない。そのときに FBX か OBJ で書き出したものをもらう。
- **時間**：動きは「1秒にどれだけ × `GameTimeGetDeltaTime()`」で書く。1フレームは 1/60 秒で固定（No.4 で決めた。`docs/dev/01_DX11基盤の構成.md` の「時間の使い方」）。
- **ビルド**：MSBuild の場所は PC によって違う（Community の PC と Enterprise の PC がある）。`vswhere.exe` で探してから使う（`docs/lessons.md` 参照）。
- **作業場所**：Claude が確かめのために一時フォルダ（%TEMP%）の worktree でビルドするときは `/t:Rebuild` を付ける（`docs/lessons.md` 参照）。

## 4. 別の PC・新しいセッションで始めるとき（Claude・Codex がやること）

Claude（Claude Code）でも Codex でも、共有の `Proto1` フォルダで同じ手順で続きから始める。もう一方の AI が作業中でないことを確認する（`AGENTS.md`）。

1. `CLAUDE.md`（Codex は `AGENTS.md` も）・`docs/lessons.md`・このファイル・`docs/tasks.md`・`docs/dev/01_DX11基盤の構成.md` を読む
2. `git status` だけを先に見る（自分のものではない変更があればユーザーに聞く）
3. `.\tools\fetch-latest.ps1` で develop を最新にし、`gh pr list` で開いている PR を確かめる
4. Notion の「Proto1（腕ダフルアームズ）」で、`tasks.md` と状態がそろっているか確かめる（ずれていたらユーザーに聞く。Notion を読めない AI は飛ばして、最後に管理者に伝える）
5. 「次にやること」の続きから始める（プランが確認待ちなら、ユーザーに確認を取ってから実装する）

## 5. 作業を終えるとき

- このファイルの「今の状態」「次にやること」「最終更新」を書き直す（どの PC の、どの AI（Claude／Codex）で更新したかも）
- `tasks.md` と Notion の状態をそろえる（Notion を書き換えられない AI は、管理者に「Notion の No.〇 を〇〇に」と伝える）
- 次にやるタスクのプランをまだ出していないときは、「次にやること」に「プランに入れること」のメモを残す（次の AI がそこから始められるように）
- 途中で終わるときは、作業中のブランチを `save-work.ps1` で GitHub に送っておく（別の PC で `start-work.ps1 <同じ作業名>` すると続きから始められる）
