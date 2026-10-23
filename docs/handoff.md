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
| 5 | 入力を「行動」に変える仕組み（`src/input/action`・`src/input/actioncalc`） | ✅ 完了 | [#8](https://github.com/HEWfuwafuwapancake/Proto1/pull/8) |
| **6** | **PS4 コントローラー（DUALSHOCK 4）を USB で読む**（`src/input/pad`・`src/input/padcalc`） | **🔄 PR を出した。実物のパッドでの確認とマージ待ち** | `tasks.md`・Notion を参照 |
| 7 | ステージ（浮島）と場外の判定（`src/game/stage`・`src/physics/stagecalc`） | ✅ PR を出した。**管理者のマージ待ち** | `tasks.md`・Notion を参照 |
| **8** | **プレイヤー：カプセルで移動・ジャンプ・重力・着地** | **⬜ プランを出した。管理者の確認待ち** | — |

- No.6 の PR（`feature/pad-input`）と No.7 の PR（`feature/stage`）がマージ待ち。それ以外の変更は develop にマージ済み。
  2つの PR はどちらも `docs/handoff.md` を変えている。No.7 の `handoff.md` は No.6 の内容を含めて書いてあるので、後からマージする側で食い違ったら No.7 の内容を使う。
- ゲームは起動すると、空に浮かぶ浮島（30 × 30）・回る立方体・2D パネルが出る（FPS 60）。丘と湖の地形は使っていない（No.7）。
  単体テストは No.6 の PR で 38 個、No.7 の PR で 33 個（両方マージすると 43 個）。
- **No.6 で残っている確認（DUALSHOCK 4 を USB でつないで、人が操作して見る）**：Debug で起動して、出力ウィンドウに「パッド：DUALSHOCK 4 をつなぎました」→ ×・L1・R1・□・OPTIONS で「行動：〇〇」→ 右スティックでカメラが回る → 抜くと「外れました」で、落ちない。
  確かめられたら `tasks.md` と Notion の No.6 を ✅ / 完了にする。Bluetooth は読まない（USB だけ）。
- No.5 で残っている確認（人が操作して見る）：左右クリック・Space・E・R で「行動：〇〇」が出る／マウス・矢印キーでカメラが前と同じように回る。
- **メインPC でまだやっていないこと**：Claude に許可を足す設定（下の「3. 決まったこと」の「Claude の許可」）。

- No.7 で残っている確認（人が操作して見る）：Debug で起動し、カメラを Ctrl で浮島の下（高さ -15 より下）へ降ろすと出力ウィンドウに「ステージ：場外（カメラ）」、上がると「場内に戻った」と出る。

## 2. 次にやること：No.8 プレイヤー（カプセルで移動・ジャンプ・重力・着地）（プラン・確認待ち）

No.5〜No.7 の PR がマージされたあと、管理者が「進めて」と言ったら、このプランで実装する（行動・パッド・浮島を使うので、No.6・No.7 のマージが先）。

**やること**
1. **計算 `src/physics/playercalc.h / .cpp`**（DirectX を使わない・単体テストする）
   - `PLAYER_BODY`（足元の位置・速度・浮島に立っているか）を、1フレーム分進める関数（移動の向き・ジャンプ・dt・浮島を渡す）
   - 移動：行動の「移動」×速さ（空中でも同じように動ける）。ジャンプ：立っているときだけ上向きの速さを与える。重力：毎フレーム下向きに速さを足す
   - 着地：落ちてきて浮島の上の面を下に通り過ぎたら、上の面に止めて「立っている」にする。浮島の外（`StageCalcIsOnPlatform` が false）では止めずに落ちる
   - 調整用の値（案）：歩く速さ 6 m/秒・ジャンプの速さ 9 m/秒・重力 25 m/秒²（すべて × `GameTimeGetDeltaTime()`）
2. **描画の部品 `src/render/shape.h / .cpp`**：箱・球・カプセル（円柱＋球2つ）を色と位置を指定して描く関数。No.11 のダミー・No.13 の触腕（球をつないだ腕）でも使う
3. **プレイヤー `src/game/player.h / .cpp`**：カプセル（半径 0.5・高さ 1.8・仮の色）で描く。移動の向きはカメラの左右の向き（前 = カメラが向いている向き）に合わせる
   - 場外（`StageCalcIsOut`）に落ちたら、浮島の真ん中の上に戻す（試合の流れは No.14 なので、今はすぐ戻すだけ）
4. **カメラ**：自由カメラの移動（W/A/S/D・Space/Ctrl・Shift）をやめ、**仮に**プレイヤーの後ろ上から追いかけるだけにする（マウス・右スティックで回すのは今のまま）。きちんとした TPS カメラ（めり込まない・なめらかに追う など）は No.9
5. 確かめ用の「行動：〇〇」の出力（No.5）と「場外（カメラ）」の出力（No.7）を消す。代わりに Debug ビルドでは、プレイヤーが場外に落ちたときだけ出す

**確かめ方**
- `playercalc` の単体テスト（立っているときだけジャンプできる・重力で落ちる・浮島の上に着地して止まる・浮島の外では止まらずに落ちる・斜めに動いても速くならない）。わざと壊してテストが失敗することも確かめる
- ビルド警告 0、起動して FPS 60。W/A/S/D（左スティック）で歩ける、Space（×）でジャンプ、浮島の端から落ちると真ん中に戻る

**決めてほしいこと**：カメラを No.8 で「仮に追いかけるだけ」にしてよいか（しないと、プレイヤーが画面の外に歩いていってしまう）。

**終わったら**：PR を出す → `tasks.md` の No.8 を ✅ → Notion の No.8 を「完了」（PR のリンク）→ このファイルを更新 → No.9 のプランを出す。

## 3. これまでに決まったこと（会話の中で決めたこと）

- **進め方**：`tasks.md` の優先度：高から順に、1つずつ「プラン → 管理者の確認 → 実装 → テスト → 完了チェック」。1タスク＝1フィーチャーブランチ＝1 PR。
- **マージ**：管理者が「マージして」と言ったら Claude が `gh pr merge <番号> --merge --admin` でマージし、GitHub 上のブランチを消す。言われていないときはマージしない。
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
