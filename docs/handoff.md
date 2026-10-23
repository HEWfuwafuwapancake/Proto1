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
| 7 | ステージ（浮島）と場外の判定 | ⬜ プランを出した。管理者の確認待ち | — |

- No.6 の PR（`feature/pad-input`）がマージ待ち。それ以外の変更は develop にマージ済み。
- ゲームは起動すると、丘と砂のくぼ地・回る立方体・2D パネルが出る（FPS 60）。単体テストは 38 個すべて通る（No.6 の PR の時点）。
- **No.6 で残っている確認（DUALSHOCK 4 を USB でつないで、人が操作して見る）**：Debug で起動して、出力ウィンドウに「パッド：DUALSHOCK 4 をつなぎました」→ ×・L1・R1・□・OPTIONS で「行動：〇〇」→ 右スティックでカメラが回る → 抜くと「外れました」で、落ちない。
  確かめられたら `tasks.md` と Notion の No.6 を ✅ / 完了にする。Bluetooth は読まない（USB だけ）。
- No.5 で残っている確認（人が操作して見る）：左右クリック・Space・E・R で「行動：〇〇」が出る／マウス・矢印キーでカメラが前と同じように回る。
- **メインPC でまだやっていないこと**：Claude に許可を足す設定（下の「3. 決まったこと」の「Claude の許可」）。

## 2. 次にやること：No.7 ステージ（浮島）と場外の判定（プラン・確認待ち）

No.6 の PR のマージのあと、管理者が「進めて」と言ったら、このプランで実装する。

**やること**
1. **場外の計算 `src/physics/stagecalc.h / .cpp`**（`src/physics/` を新しく作る。DirectX を使わない・単体テストする）
   - `STAGE`（浮島の中心・横幅・奥行き・上の面の高さ）と `BLAST_ZONE`（場外の線：左右・前後・下）を「調整用の値」で決める
   - `StageCalcIsOnPlatform(stage, x, z)`：その場所の真下に浮島があるか（あれば上の面の高さに着地できる）
   - `StageCalcIsOut(blastZone, position)`：場外の線を越えたか（スマブラのように、横に飛ばされても下に落ちてもアウト）
2. **描画 `src/game/stage.h / .cpp`**：浮島を平たい箱で描く（`StageInitialize / Finalize / Update / Draw`）。仮の形・仮の色
3. 置き場所（案）：今の丘と湖の地形の**上空**に浮かべる（浮島の上の面を y = 10、広さ 30 × 30）。場外の下の線は浮島より下（y = -5 くらい）。地形は背景として残す
4. `CLAUDE.md` の表・`docs/dev/01_DX11基盤の構成.md`・`.vcxproj.filters` に `src/physics` を足す（`CLAUDE.md` の決まり）
5. 確かめ用に、Debug ビルドでは、カメラの位置が場外に出たら出力ウィンドウに「場外」と出す（プレイヤーは No.8 なので、今はカメラで試す）

**確かめ方**
- `stagecalc` の単体テスト（浮島の真ん中・端・外、場外の線の内側・ちょうど・外、下に落ちたとき）。わざと壊してテストが失敗することも確かめる
- ビルド警告 0、起動して FPS 60。浮島が見える。カメラで浮島の外へ出ると「場外」と出る

**決めてほしいこと**：浮島の置き場所（地形の上空に浮かべる案でよいか、地形を消して浮島だけにするか）と広さ。

**終わったら**：PR を出す → `tasks.md` の No.7 を ✅ → Notion の No.7 を「完了」（PR のリンク）→ このファイルを更新 → No.8 のプランを出す。

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
