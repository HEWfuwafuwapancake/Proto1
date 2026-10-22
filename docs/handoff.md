# 引き継ぎメモ（今どこまで進んだか・次に何をするか）

別の PC（メインPC・ノートPC）や、別の日の Claude Code で作業を続けるためのメモです。
**作業を始めるときに読み、作業を終えるときに更新します。** タスクの一覧は [tasks.md](tasks.md)、同じ内容を Notion の
「[Proto1（腕ダフルアームズ）](https://app.notion.com/p/e5e9f1983bbe4758b06b4adba135a491)」データベースでも管理しています。

- 最終更新：2026-10-23（メインPC の Claude Code）

---

## 1. 今の状態

| No | タスク | 状態 | PR |
|---|---|---|---|
| 1 | 企画の要点を `docs/plan/` にまとめる | ✅ 完了 | [#3](https://github.com/HEWfuwafuwapancake/Proto1/pull/3) |
| 2 | コードの構成ルールとフォルダ分け（`src/core` `src/input` `src/render` `src/game` `shaders`） | ✅ 完了 | [#4](https://github.com/HEWfuwafuwapancake/Proto1/pull/4) |
| 3 | 単体テストのプロジェクト（`tests/Proto1Tests`、`tools/run-tests.ps1`） | ✅ 完了 | [#5](https://github.com/HEWfuwafuwapancake/Proto1/pull/5) |
| **4** | **時間の管理** | **⬜ プランを出した。管理者の確認待ち** | — |

- Proto1 の develop に、ここまでの変更はすべてマージ済み。開いている PR はなし。
- ゲームは起動すると、丘と砂のくぼ地・回る立方体・2D パネルが出る（FPS 60）。単体テストは 6 個すべて通る。

## 2. 次にやること：No.4 時間の管理（プラン・確認待ち）

管理者が「進めて」と言ったら、このプランで実装する。変えたいと言われたら直してから実装する。

**やること**
1. **`src/core/gametime.h / .cpp`**（ひな形の書き方：`GameTimeInitialize / Update`）
   - `GameTimeGetDeltaTime()`：ゲームの計算に使う1フレームの秒数。**1/60 秒で固定**（メインループがちょうど1秒に60回なので、固定のほうが計算が安定して、同じ操作で同じ結果になる）
   - `GameTimeGetElapsed()`：ゲームが始まってからの秒数
   - `GameTimeSetScale(scale)`：時間の進む速さ（0 で止まる、1 でふつう）。No.17 のヒットストップの土台になる
2. **タイマー `src/core/timer.h / .cpp`**（DirectX を使わない・単体テストする）：No.14 の制限時間や、あとでダミーの復活に使う「○秒数える」部品
   - `TimerStart(timer, seconds)` / `TimerUpdate(timer, dt)` / `TimerGetRemaining` / `TimerIsFinished`
3. `main.cpp` の `Update` の最初で `GameTimeUpdate()` を呼ぶ
4. 立方体の回転を「1フレームに何度」から「1秒に何度 × dt」に変える（dt を使う最初の例）。カメラは No.9 で作り直すので、今は変えない

**確かめ方**
- タイマーと時間の単体テスト（数え終わる・0 を過ぎても負にならない・scale 0 で止まる など）。わざと壊してテストが失敗することも確かめる
- ビルド警告 0、起動して FPS 60、立方体が前と同じ速さで回る（1/60 秒 × 60 回 = 同じ）

**終わったら**：PR を出す → `tasks.md` の No.4 を ✅ → Notion の No.4 を「完了」（PR のリンク）→ このファイルを更新 → No.5 のプランを出す。

## 3. これまでに決まったこと（会話の中で決めたこと）

- **進め方**：`tasks.md` の優先度：高から順に、1つずつ「プラン → 管理者の確認 → 実装 → テスト → 完了チェック」。1タスク＝1フィーチャーブランチ＝1 PR。
- **マージ**：管理者が「マージして」と言ったら Claude が `gh pr merge <番号> --merge --admin` でマージし、GitHub 上のブランチを消す。言われていないときはマージしない。
- **状態の記録**：タスクの状態は `tasks.md` と Notion の両方をそろえる。プランを出したら、このファイルの「次にやること」と Notion のタスクのページの本文に書く。
- **プロトのルール**（`docs/plan/01_企画の要点.md`）：制限時間 90 秒・ダミー3体・場外で +1・L1 / R1 で左右の触腕（キーボードは左右クリック）・□ ではたく（E）・OPTIONS でやり直し（R）。
- **PS4 コントローラー**は外部ライブラリなしで読む（Raw Input と HID）。No.6 のテストのときに USB でつなぐ。
- **Maya のモデル**は No.22 まで使わない。そのときに FBX か OBJ で書き出したものをもらう。
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
