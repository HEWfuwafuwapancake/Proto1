# 引き継ぎメモ（今どこまで進んだか・次に何をするか）

別の PC（メインPC・ノートPC）や、別の日の Claude Code で作業を続けるためのメモです。
**作業を始めるときに読み、作業を終えるときに更新します。** タスクの一覧は [tasks.md](tasks.md)、同じ内容を Notion の
「[Proto1（腕ダフルアームズ）](https://app.notion.com/p/e5e9f1983bbe4758b06b4adba135a491)」データベースでも管理しています。

- 最終更新：2026-10-23（Visual Studio Enterprise の PC の Claude Code。No.8 まで）

---

## 1. 今の状態

| No | タスク | 状態 | PR |
|---|---|---|---|
| 1 | 企画の要点を `docs/plan/` にまとめる | ✅ 完了 | [#3](https://github.com/HEWfuwafuwapancake/Proto1/pull/3) |
| 2 | コードの構成ルールとフォルダ分け（`src/core` `src/input` `src/render` `src/game` `shaders`） | ✅ 完了 | [#4](https://github.com/HEWfuwafuwapancake/Proto1/pull/4) |
| 3 | 単体テストのプロジェクト（`tests/Proto1Tests`、`tools/run-tests.ps1`） | ✅ 完了 | [#5](https://github.com/HEWfuwafuwapancake/Proto1/pull/5) |
| 4 | 時間の管理（`src/core/gametime`・`src/core/timer`） | ✅ 完了 | [#7](https://github.com/HEWfuwafuwapancake/Proto1/pull/7) |
| 5 | 入力を「行動」に変える仕組み（`src/input/action`・`src/input/actioncalc`） | ✅ 完了 | [#8](https://github.com/HEWfuwafuwapancake/Proto1/pull/8) |
| **6** | **PS4 コントローラー（DUALSHOCK 4）を USB で読む**（`src/input/pad`・`src/input/padcalc`） | **🔄 マージ済み。実物のパッドでの確認だけ残っている** | [#9](https://github.com/HEWfuwafuwapancake/Proto1/pull/9) |
| 7 | ステージ（浮島）と場外の判定（`src/game/stage`・`src/physics/stagecalc`） | ✅ 完了 | [#10](https://github.com/HEWfuwafuwapancake/Proto1/pull/10) |
| 8 | プレイヤー（`src/game/player`・`src/physics/playercalc`）・形の描画の部品（`src/render/shape`）・仮の追いかけカメラ。回る立方体は消した | ✅ 完了 | `tasks.md`・Notion を参照 |
| **9** | **TPS カメラ** | **⬜ プランを出した。管理者の確認待ち** | — |

- 開いている PR はなし（No.8 の PR はマージ済み）。
- ゲームは起動すると、空に浮かぶ浮島（30 × 30）の真ん中に赤いカプセルのプレイヤーが立っている（FPS 60）。W/A/S/D・左スティックで歩き、Space・×でジャンプ、浮島から落ちると真ん中に戻る。
  カメラは仮にプレイヤーの後ろ上についていくだけ（マウス・右スティックで回る）。単体テストは 52 個すべて通る。
- **No.6 で残っている確認（DUALSHOCK 4 を USB でつないで、人が操作して見る）**：Debug で起動して、出力ウィンドウに「パッド：DUALSHOCK 4 をつなぎました」→ ×・L1・R1・□・OPTIONS で「行動：〇〇」→ 左スティックで歩く・右スティックでカメラが回る → 抜くと「外れました」で、落ちない。
  確かめられたら `tasks.md` と Notion の No.6 を ✅ / 完了にし、`action.cpp` の確かめ用の「行動：〇〇」の出力を消す。Bluetooth は読まない（USB だけ）。
- 人が操作して見る確認（No.5・No.8）：左右クリック・Space・E・R で「行動：〇〇」が出る／W/A/S/D で歩ける・Space でジャンプ／浮島の端から落ちると出力ウィンドウに「プレイヤー：場外」と出て真ん中に戻る。
- **メインPC でまだやっていないこと**：Claude に許可を足す設定（下の「3. 決まったこと」の「Claude の許可」）。

## 2. 次にやること：No.9 TPS カメラ（プラン・確認待ち）

管理者が「進めて」と言ったら、このプランで実装する。

**やること**
1. **計算 `src/physics/cameracalc.h / .cpp`**（DirectX を使わない・単体テストする）
   - 見ている点（プレイヤーの頭のあたり）・左右と上下の向き・距離から、カメラの位置を出す
   - **浮島にめり込まない**：見ている点からカメラへの線が浮島の箱にぶつかったら、ぶつかる手前までカメラを近づける
   - **なめらかに追う**：カメラの位置を、目標の位置へ少しずつ近づける（近づく速さは dt を使い、FPS が変わっても同じ動き）
2. **`src/game/camera.cpp` を作り直す**：No.8 の仮の動きを、上の計算を使った TPS カメラに置き換える
   - 上下の向きの範囲（見下ろしすぎ・見上げすぎない）、距離などは「調整用の値」
3. **回す速さを「1秒に何度 × dt」にする**：矢印キーと右スティックで回る速さ（`action.cpp` の `KEY_LOOK_SPEED`・`PAD_LOOK_SPEED`）を1秒あたりに直す。マウスは動かした量のままでよい
4. `docs/dev/01_DX11基盤の構成.md` の「プレイヤーとカメラ」を直す

**確かめ方**
- `cameracalc` の単体テスト（向きと距離から位置が出る・浮島に線がぶつかると近づく・ぶつからなければそのまま・なめらかに追うと目標に近づき、行き過ぎない）。わざと壊してテストが失敗することも確かめる
- ビルド警告 0、起動して FPS 60。歩くとカメラが少し遅れてついてくる／下から見上げても浮島の中にカメラが入らない

**決めてほしいこと**：プレイヤーが歩いたときに、カメラが自動でプレイヤーの後ろに回り込むか（案：**回り込まない**。向きはマウス・右スティックで自分で変える。触腕を「カメラの向いている方へ」伸ばすので、勝手に回ると狙いにくい）。

**終わったら**：PR を出してマージ → `tasks.md` の No.9 を ✅ → Notion の No.9 を「完了」（PR のリンク）→ このファイルを更新 → No.10 のプランを出す。

## 3. これまでに決まったこと（会話の中で決めたこと）

- **進め方**：`tasks.md` の優先度：高から順に、1つずつ「プラン → 管理者の確認 → 実装 → テスト → 完了チェック」。1タスク＝1フィーチャーブランチ＝1 PR。
- **マージ**：Claude がこの流れで作った PR は、テストとビルドが通れば、管理者に聞かずに Claude が `gh pr merge <番号> --merge --admin --delete-branch` でマージしてよい（2026-10-23、管理者が「基本マージして問題ない」と決めた）。他の人の PR はマージしない。
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
