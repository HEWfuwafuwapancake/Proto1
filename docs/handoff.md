# 引き継ぎメモ（今どこまで進んだか・次に何をするか）

別の PC（メインPC・ノートPC）や、別の日の Claude Code で作業を続けるためのメモです。
**作業を始めるときに読み、作業を終えるときに更新します。** タスクの一覧は [tasks.md](tasks.md)、同じ内容を Notion の
「[Proto1（腕ダフルアームズ）](https://app.notion.com/p/e5e9f1983bbe4758b06b4adba135a491)」データベースでも管理しています。

- 最終更新：2026-10-23（Visual Studio Enterprise の PC の Claude Code）

---

## 1. 今の状態

| No | タスク | 状態 | PR |
|---|---|---|---|
| 1 | 企画の要点を `docs/plan/` にまとめる | ✅ 完了 | [#3](https://github.com/HEWfuwafuwapancake/Proto1/pull/3) |
| 2 | コードの構成ルールとフォルダ分け（`src/core` `src/input` `src/render` `src/game` `shaders`） | ✅ 完了 | [#4](https://github.com/HEWfuwafuwapancake/Proto1/pull/4) |
| 3 | 単体テストのプロジェクト（`tests/Proto1Tests`、`tools/run-tests.ps1`） | ✅ 完了 | [#5](https://github.com/HEWfuwafuwapancake/Proto1/pull/5) |
| 4 | 時間の管理（`src/core/gametime`・`src/core/timer`） | ✅ 完了 | [#7](https://github.com/HEWfuwafuwapancake/Proto1/pull/7) |
| 5 | 入力を「行動」に変える仕組み（`src/input/action`・`src/input/actioncalc`） | ✅ PR を出した。**管理者のマージ待ち** | PR は `tasks.md`・Notion を参照 |
| **6** | **PS4 コントローラー（DUALSHOCK 4）を USB で読む** | **⬜ プランを出した。管理者の確認待ち** | — |

- No.5 の PR（`feature/input-action`）がマージ待ち。それ以外の変更は develop にマージ済み。
- ゲームは起動すると、丘と砂のくぼ地・回る立方体・2D パネルが出る（FPS 60）。単体テストは 28 個すべて通る（No.5 の PR の時点）。
- No.5 の PR で残っている確認（人が操作して見る）：左右クリック・Space・E・R で VS の出力ウィンドウに「行動：〇〇」が出る／マウス・矢印キーでカメラが前と同じように回る。
- **メインPC でまだやっていないこと**：Claude に `gh pr merge` を許す設定（下の「3. 決まったこと」の「マージの許可」）。

## 2. 次にやること：No.6 PS4 コントローラー（DUALSHOCK 4）を USB で読む（プラン・確認待ち）

No.5 の PR のマージのあと、管理者が「進めて」と言ったら、このプランで実装する。**テストのときに DUALSHOCK 4 を USB でつなぐ**。

**やること**
1. **`src/input/pad.h / .cpp`**（ひな形の書き方：`PadInitialize / Finalize / Update`）
   - Raw Input に「ゲームパッド（HID）」も登録し、`WndProc` の `WM_INPUT` で、マウスでなければパッドに渡す
   - DUALSHOCK 4 だけを読む（ソニーの VID `054C`・PID `05C4`（初代）/ `09CC`（2代目）で見分ける）。1台だけ（プロトは1人で遊ぶ）
   - 抜いた／つないでいないときは「何も押していない」扱い（落ちない）
2. **USB のデータ（レポート）の読み方は `src/input/padcalc.h / .cpp` に分けて単体テストする**（DirectX も Windows も使わない）
   - 左右のスティック（0〜255 → -1〜1。上がプラス）、真ん中の遊び（デッドゾーン）、×・□・L1・R1・OPTIONS のボタン
   - テストは、決まったバイト列を渡して、スティックとボタンが正しく読めるかを確かめる
3. `action.cpp` でキーボードとパッドをまとめる：ボタンは「どちらかを押している」、移動は `ActionCalcCombineAxis`（長いほう）、カメラは右スティック × 回る速さを足す
4. Debug ビルドでは、つないだ／外れたときに出力ウィンドウに出す

**確かめ方**
- `padcalc` の単体テスト（スティックの真ん中が 0・端が ±1・デッドゾーンの中は 0・ボタンのビット）。わざと壊してテストが失敗することも確かめる
- ビルド警告 0、起動して FPS 60。パッドの ×・□・L1・R1・OPTIONS で「行動：〇〇」が出る、右スティックでカメラが回る、抜いても落ちない

**終わったら**：PR を出す → `tasks.md` の No.6 を ✅ → Notion の No.6 を「完了」（PR のリンク）→ このファイルを更新 → No.7 のプランを出す。

## 3. これまでに決まったこと（会話の中で決めたこと）

- **進め方**：`tasks.md` の優先度：高から順に、1つずつ「プラン → 管理者の確認 → 実装 → テスト → 完了チェック」。1タスク＝1フィーチャーブランチ＝1 PR。
- **マージ**：管理者が「マージして」と言ったら Claude が `gh pr merge <番号> --merge --admin` でマージし、GitHub 上のブランチを消す。言われていないときはマージしない。
- **マージの許可**：Claude Code の安全チェックが `gh pr merge` を止めるので、管理者の PC だけ `Proto1\.claude\settings.local.json` に次を入れる（GitHub には送らない。`.gitignore` 済み）。
  `{"permissions":{"allow":["PowerShell(gh pr merge:*)","Bash(gh pr merge:*)"]}}`
  Visual Studio Enterprise の PC は設定済み。**メインPC はまだ**（メインPC の Claude は、管理者に確認してから作る）。
  `start-work.ps1`（中で develop を `git merge` する）も止められることがある。そのときは管理者に実行してもらう。
- **状態の記録**：タスクの状態は `tasks.md` と Notion の両方をそろえる。プランを出したら、このファイルの「次にやること」と Notion のタスクのページの本文に書く。
- **プロトのルール**（`docs/plan/01_企画の要点.md`）：制限時間 90 秒・ダミー3体・場外で +1・L1 / R1 で左右の触腕（キーボードは左右クリック）・□ ではたく（E）・OPTIONS でやり直し（R）。
- **PS4 コントローラー**は外部ライブラリなしで読む（Raw Input と HID）。No.6 のテストのときに USB でつなぐ。
- **Maya のモデル**は No.22 まで使わない。そのときに FBX か OBJ で書き出したものをもらう。
- **時間**：動きは「1秒にどれだけ × `GameTimeGetDeltaTime()`」で書く。1フレームは 1/60 秒で固定（No.4 で決めた。`docs/dev/01_DX11基盤の構成.md` の「時間の使い方」）。
- **ビルド**：MSBuild の場所は PC によって違う（Community の PC と Enterprise の PC がある）。`vswhere.exe` で探してから使う（`docs/lessons.md` 参照）。
- **作業場所**：Claude が確かめのために一時フォルダ（%TEMP%）の worktree でビルドするときは `/t:Rebuild` を付ける（`docs/lessons.md` 参照）。

## 4. 別の PC・新しいセッションで始めるとき（Claude がやること）

1. `CLAUDE.md`・`docs/lessons.md`・このファイル・`docs/tasks.md` を読む
2. `git status` だけを先に見る（自分のものではない変更があればユーザーに聞く）
3. `.\tools\fetch-latest.ps1` で develop を最新にし、`gh pr list` で開いている PR を確かめる
4. Notion の「Proto1（腕ダフルアームズ）」で、`tasks.md` と状態がそろっているか確かめる（ずれていたらユーザーに聞く）
5. 「次にやること」の続きから始める（プランが確認待ちなら、ユーザーに確認を取ってから実装する）

## 5. 作業を終えるとき

- このファイルの「今の状態」「次にやること」「最終更新」を書き直す（どの PC で更新したかも）
- `tasks.md` と Notion の状態をそろえる
- 途中で終わるときは、作業中のブランチを `save-work.ps1` で GitHub に送っておく（別の PC で `start-work.ps1 <同じ作業名>` すると続きから始められる）
