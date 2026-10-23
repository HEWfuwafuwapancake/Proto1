# Codex との相互レビュー（管理者だけが使う）

Claude と Codex の2つの AI で、**片方が実装して、もう片方がレビューする**ための手順です。
使うのは管理者（Shota57）だけです。ほかのメンバーは読まなくてよく、作業の流れ（`start-work` / `save-work` / `submit-pr`）も変わりません。

- PR の流れはチームと同じです。AI どうしのやりとりは PR の上で行います。
- どちらの AI も管理者の GitHub アカウントで動くので、GitHub の「承認（Approve）」はできません。
  **レビューはコメントで行い、マージするかどうかは管理者が決めます**（`tools/admin/merge-pr.ps1`）。

---

## 1. PC ごとの準備（1回だけ）

AI ごとに作業フォルダを分けます。同じフォルダで2つの AI が同時に動くと、ブランチの切り替えや
`save-work.ps1`（`git add -A`）で相手の作業途中のファイルを巻き込んでしまうためです。

| フォルダ | 使う AI |
|---|---|
| `Documents\GitHub\Proto1`（PC によっては `source\repos\Proto1`） | Claude（Claude Code） |
| `Documents\GitHub\Proto1-codex`（Claude のフォルダと同じ場所に `-codex` を付けて作る） | Codex |

レビューだけでなく、Codex が開発の続き（次のタスクの実装）をするときも、この Codex 用のフォルダを使います。
始め方は `AGENTS.md` の「開発の続きをするとき」と、`docs/handoff.md`（引き継ぎメモ）に書いてあります。

Codex 用のフォルダを作る：

```powershell
git clone https://github.com/HEWfuwafuwapancake/Proto1.git "$HOME\Documents\GitHub\Proto1-codex"
Set-Location "$HOME\Documents\GitHub\Proto1-codex"
git lfs pull
.\tools\fetch-latest.ps1
```

2台目の PC でも同じことをします。この手順書や `AGENTS.md` は Git に入っているので、`fetch-latest.ps1` で届きます。

## 2. 1つのタスクの流れ

### 実装する AI
1. 自分のフォルダで `.\tools\start-work.ps1 <作業名>`
2. 実装して、Debug / Release でビルドが通ることと、起動して動くことを確かめる
3. `.\tools\save-work.ps1 "やったこと"` → `.\tools\submit-pr.ps1`
4. PR の説明の「見てほしいところ」に、**`実装：Claude／レビュー：Codex`**（またはその逆）と書く

### レビューする AI
1. 自分のフォルダで PR を取ってくる：`gh pr checkout <PR番号>`
2. 下の「3. レビューで見ること」に沿って読み、必要ならビルドして動かす
3. 指摘を PR にコメントする
   - **Claude がレビューするとき**：Claude Code で `/code-review <PR番号> --comment`
     （`--comment` を付けないと、PR には書かずに会話の中で結果を返す）
   - **Codex がレビューするとき**：Codex 用フォルダで Codex を開き、`/review` を使う。
     PR にコメントとして残したいときは、結果を `gh pr comment <PR番号> --body-file <ファイル>` で書く
4. 指摘には重さを付ける：**必須**（直さないとマージできない）／**推奨**（直したほうがよい）／**質問**
5. 相手のブランチには**コミットしない**（直すのは実装した AI）
6. 終わったら `git switch develop` で develop に戻る
   （PR はまだマージされていないので、`fetch-latest.ps1` では戻らない。`fetch-latest.ps1` が develop に切り替えるのはマージ済みのときだけ）
7. PR がマージされたら、`.\tools\fetch-latest.ps1` の最後に「消してOK」と出る手元のブランチを `git branch -d <ブランチ名>` で消す

### 実装した AI（指摘を受けて）
1. 必須の指摘を直す。推奨は直すか、直さない理由を PR に返信する
2. `.\tools\save-work.ps1 "レビューの指摘を直す"`（同じ PR に反映される）
3. 自分のミスだった指摘は `docs/lessons.md` に追記する

### 管理者
- 指摘が片付いたら `.\tools\admin\merge-pr.ps1 <PR番号>` でマージする
- 次のタスクでは、実装とレビューの役を入れ替える

## 3. レビューで見ること

**ビルドと動作**
- [ ] Debug / Release（x64）でビルドが通り、新しい警告が出ていない（シェーダーの警告も）
- [ ] 起動して、変えたところが動く。前からあった機能が壊れていない

**このプロジェクトの決まり**（詳しくは `CLAUDE.md`）
- [ ] C++（`.cpp` / `.h`）は BOM付きUTF-8、シェーダー（`.hlsl` / `.hlsli`）は BOMなしUTF-8
- [ ] 足したファイルが `.vcxproj` と `.vcxproj.filters` に入っている
- [ ] 定数バッファの構造体（`shader.h` / `shader.cpp`）と HLSL 側（`shaderCommon.hlsli`）の並び・番号がそろっている
- [ ] 書き方がひな形に合っている（`〇〇Initialize / Finalize / Update / Draw`、作ったものは `Finalize` で `SAFE_RELEASE`）
- [ ] スクリプトを変えたときは `docs/team/04_コマンド一覧.md` も直っている

**中身**
- [ ] やりたいことに対して、変更が多すぎない・関係ない変更が混ざっていない
- [ ] コメントが日本語で、ほかのメンバーが読んで分かる
- [ ] `docs/lessons.md` に書いてある失敗を繰り返していない

## 4. 記録

- **`docs/lessons.md`**：ミスは、見つけたのが自分でも相手のレビューでも、ミスをした AI が追記する（「書いた AI」に名前を書く）
- **Notion（このプロジェクトのデータベース）**：「委譲先」に実装する AI、「レビュー担当」にレビューする AI を入れる。マージされたらステータスを「完了」にする
