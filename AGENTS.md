# Proto1 チーム制作 — Codex 向けの指示

DirectX 11（C++ / Visual Studio 2022）で作る 3D ゲームのプロトタイプのチーム制作リポジトリ。
Codex を使うのは管理者（Shota57）だけ。ほかのメンバーは Codex を使わず、`tools/` のスクリプトでいつも通り作業している。

## 作業を始める前に必ず読むもの
1. **`CLAUDE.md`** … このプロジェクトのルール（git のルール、コードの約束、ビルド方法）。
   「Claude向け」と書いてあるが、Codex も同じルールに従う。
   ただし Claude Code 専用の機能（`/code-review` などのコマンド）の話は除く。
2. **Notion の「[Proto1 開発ログ](https://app.notion.com/p/939057f5e85e4bdb9d6d1d3bc1bc28da)」** … AI（Claude と Codex）の失敗と教訓（旧 `docs/lessons.md`）。
   Notion の MCP で `collection://26982506-1bf2-4345-b4d9-be248dbacd7c` の「タイトル」「要点」「対応モジュール」の列だけを読み、同じミスをしないこと。本文は関係する行だけ開く。
3. **`docs/ai/01_Codexとの相互レビュー.md`** … Claude との相互レビューの手順と、レビューで確認する観点。

ルールは `CLAUDE.md` にまとめてある。このファイルにはルールを写さず、Codex だけのことを書く。

## 開発の続きをするとき（Claude から引き継いだ作業）
Claude と Codex は同じ「引き継ぎメモ」を使って、どちらでも続きから開発できる。
1. `CLAUDE.md` の「セッションを始めるとき」の手順に従い、**`docs/handoff.md`（引き継ぎメモ）** を読む。
   今の状態・次にやること（次のタスクのプランやプロンプト）だけが書いてある。会話で決まったことは `docs/decisions.md`。
2. **`docs/tasks.md`** で、次のタスクと「前提」を確かめる。上から順に「プラン → 管理者の確認 → 実装 → テスト → 完了チェック」で進める。
   **プランを出したら、管理者が「進めて」と言うまで実装しない。**
3. **`docs/dev/01_DX11基盤の構成.md`** で、今のコードの作り（ファイルの役割・`Update` の順番・当たり判定や吹っ飛びの使い方）を確かめる。
4. 作業を終えるときは、`CLAUDE.md` の「作業を終えるとき」に従って、引き継ぎメモと `docs/tasks.md` を更新する（「最終更新」に「Codex」と書く）。
- **Notion**：Codex から Notion を書き換えられないときは、`docs/tasks.md` と `docs/handoff.md` だけを更新し、
  最後に「Notion の No.〇 を〇〇にしてください」と管理者に伝える（次に Claude が始めるときにも、`tasks.md` とのずれを確かめて直す）。
- **ビルド**：MSBuild のパスは PC によって違うので、`vswhere.exe` で探してから使う（Notion の開発ログ参照）。テストは `.\tools\run-tests.ps1`。
- **マージ**：Codex がこの流れで出した PR は、テスト（`run-tests.ps1`）とビルド（Debug / Release、警告 0）が通れば、管理者に聞かずに
  `gh pr merge <番号> --merge --admin --delete-branch` でマージしてよい（2026-10-23、管理者が決めた。Claude と同じ）。他の人の PR はマージしない。
- `docs/decisions.md` の「Claude の許可」（`.claude/settings.local.json`）は Claude Code だけの設定なので、Codex は気にしなくてよい。

## Codex だけのこと
- **Codex はコードを書くだけにする**（2026-10-24、管理者が決めた。トークン節約のため、確認を Claude と二重にしない）。
  - Codex がやること：プランどおりにコードとテストのコードを書く → **Debug のビルドが通ることだけ**確かめる → `save-work.ps1` で保存する → `docs/handoff.md` の「今の状態」に「Codex が実装。Claude の確認待ち」と書く。
  - Codex がやらないこと：`run-tests.ps1`・Release のビルド・わざと壊す確認・ゲームを起動して画面を撮ること・細かいデバッグ。これらと、見つかった問題の修正は **Claude Code が行う**（Claude は Codex の作業ブランチに直接コミットしてよい）。
  - 管理者が「Codex も確かめて」と言ったときだけ、今までどおり確かめる。
- **Claude と Codex は同じ `Proto1` フォルダを交代で使う**（2026-10-23、管理者の指示）。同時には作業しない。
  AI を切り替える前に変更をコミット・push し、引き継ぎメモを更新する。開始時に `git status` を確認する。
- 引き継いだ AI は同じ作業ブランチで開発を続けてよい。レビューだけを担当するときはコミットせず、コメントで指摘する。
- Notion の「委譲先」「レビュー担当」は当面、作業の制約にしない。実装とレビューの分担は管理者が指定したときに適用する。
- ミスをして直したときや、ユーザーに間違いを指摘されたときは、Notion の「Proto1 開発ログ」に1行足す（書いたAI：Codex。書き方は `CLAUDE.md`）。Notion を書き換えられないときは、管理者に内容を伝える。
- ユーザーへの返答は日本語。ユーザーに示すコマンドは PowerShell 形式。
