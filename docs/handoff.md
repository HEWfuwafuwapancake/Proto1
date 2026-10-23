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
| 4 | 時間の管理（`src/core/gametime`・`src/core/timer`） | ✅ PR を出した。**管理者のマージ待ち** | [#7](https://github.com/HEWfuwafuwapancake/Proto1/pull/7) |
| **5** | **入力を「行動」に変える仕組み** | **⬜ プランを出した。管理者の確認待ち** | — |

- PR #7（`feature/game-time`）がマージ待ち。それ以外の変更は develop にマージ済み。
- ゲームは起動すると、丘と砂のくぼ地・回る立方体・2D パネルが出る（FPS 60）。単体テストは 20 個すべて通る（PR #7 の時点）。
- PR #7 で残っている確認：立方体が前と同じ速さで回るかを目で見る（計算上は同じ）。

## 2. 次にやること：No.5 入力を「行動」に変える仕組み（プラン・確認待ち）

PR #7 のマージのあと、管理者が「進めて」と言ったら、このプランで実装する（No.4 の時間は使わないので、マージ前に始めてもよい）。

**やること**
1. **`src/input/action.h / .cpp`**（ひな形の書き方：`ActionInitialize / Update`）
   - 行動の種類 `ACTION`：ジャンプ・左の触腕・右の触腕・はたく・やり直し（企画書の「5. 操作方法」の表のとおり）
   - `ActionIsPress / ActionIsTrigger / ActionIsRelease(action)`：押している／押した瞬間／離した瞬間
   - `ActionGetMove()`：移動（左スティックの代わり。左右・前後が -1〜1。斜めでも長さは 1 まで）
   - `ActionGetLook()`：カメラを回す量（右スティックの代わり。マウスの移動量 ＋ 矢印キー）
   - キーの割り当ては `action.cpp` の上の「調整用の値」に表でまとめる（W/A/S/D・Space・左右クリック・E・R）
2. **計算の部分は `src/input/actioncalc.h / .cpp` に分けて単体テストする**（DirectX も Windows も使わない）
   - 4つのキーから移動の向きを作る（斜めは長さ 1 に直す）
   - 複数の機器の入力を1つにまとめる（No.6 でパッドを足すとき、キーボードとパッドのどちらでも動くように）
3. マウスの左右クリックは、今の `KeyboardIsPress(VK_LBUTTON)` で取れるので、`keyboard.cpp` を使う（新しい仕組みは作らない）
4. `main.cpp` の `Update` で、キーボード・マウスの次に `ActionUpdate()` を呼ぶ
5. 今のカメラ（自由カメラ）は、回す部分だけ `ActionGetLook()` に変える。移動（W/A/S/D・Space/Ctrl で上下）は No.8・No.9 でプレイヤーと TPS カメラに置き換えるので、今は変えない
6. 確かめ用に、Debug ビルドのときだけ、押した行動の名前を VS の出力ウィンドウに出す（No.8 で消す）

**確かめ方**
- `actioncalc` の単体テスト（斜めの長さが 1・反対のキーを同時に押すと 0・2つの機器の大きいほうが勝つ など）。わざと壊してテストが失敗することも確かめる
- ビルド警告 0、起動して FPS 60。左右クリック・Space・E・R で出力ウィンドウに行動の名前が出る。マウスでカメラが前と同じように回る

**終わったら**：PR を出す → `tasks.md` の No.5 を ✅ → Notion の No.5 を「完了」（PR のリンク）→ このファイルを更新 → No.6 のプランを出す。

## 3. これまでに決まったこと（会話の中で決めたこと）

- **進め方**：`tasks.md` の優先度：高から順に、1つずつ「プラン → 管理者の確認 → 実装 → テスト → 完了チェック」。1タスク＝1フィーチャーブランチ＝1 PR。
- **マージ**：管理者が「マージして」と言ったら Claude が `gh pr merge <番号> --merge --admin` でマージし、GitHub 上のブランチを消す。言われていないときはマージしない。
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
