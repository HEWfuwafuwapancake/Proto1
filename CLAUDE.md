# Proto1 チーム制作 — Claude向けのプロジェクトメモ

DirectX 11（C++ / Visual Studio 2022）で作る 3D ゲームの**プロトタイプ**のチーム制作リポジトリ。
GitHub の Organization（HEWfuwafuwapancake）上の public リポジトリで、git-flow（main / develop / feature / release / hotfix）で運用する。
DX11 の土台は `HEW2026GitTest` から持ってきた（水のレイトレーシングは入れていない）。企画書は `docs/plan/` に置く。

## 進め方のルール
- **作業を始める前に `docs/lessons.md` を読み、同じミスをしないこと。**
  ミスをして直したときや、ユーザーに間違いを指摘されたときは、`docs/lessons.md` に「起きたこと／原因／次からどうするか」を1項目として追記する（書いた AI：Claude）。
- Codex との相互レビュー（実装やレビューを Codex と分担する）を頼まれたときは、`docs/ai/01_Codexとの相互レビュー.md` の手順に従う。
  頼まれていないときは、いつも通りに作業する（相互レビューを使うのは管理者だけ）。
- **プロトタイプなので、企画書の「確かめたいこと」を早く遊べる形にするのを優先する。** 頼まれていない機能を先回りして作らない。
- ユーザーへの返答は日本語。ユーザーに示すコマンドは **PowerShell** 形式（コードブロックは `powershell`）。
- `.ps1` に日本語を書くときは **BOM付きUTF-8** で保存する（Windows PowerShell 5.1 対策）。

## git のルール（必ず守る）
- **`main` と `develop` には直接コミット・push しない。** 作業は必ず `feature/<作業名>` で行う（GitHub の Rulesets でも禁止されている）。
- git 操作はできるだけ `tools/` のスクリプトを使う：
  - 作業開始：`.\tools\start-work.ps1 <作業名>`（英小文字・数字・ハイフン）
  - 保存：`.\tools\save-work.ps1 "やったこと"`
  - レビュー依頼：`.\tools\submit-pr.ps1`
  - develop の最新を取ってくるだけ（作業は始めない）：`.\tools\fetch-latest.ps1`
- `git push --force`、`git reset --hard`、ブランチやタグの削除は、ユーザーに確認してから。
- develop / main へのマージ、release / hotfix、タグ付けは **管理者だけ**（`tools/admin/`）。ユーザーが管理者かどうか分からなければ聞く。
- 画像・モデル・音声は Git LFS（`.gitattributes`）。新しい種類の素材ファイルを入れるときは `.gitattributes` に追加する。

## コードの約束
- 授業（DX21）のひな形をもとにした DX11 基盤。構成は `docs/dev/01_DX11基盤の構成.md`。書き方はひな形に合わせる（`〇〇Initialize / Finalize / Update / Draw`）。
- ヘッダーで公開する関数・構造体には **Doxygen 形式の日本語コメント**（`/// @brief` など）を付ける。bool を受け取る関数は「〜を設定する（true：〜、false：〜）」と書き、構造体はメンバーを1つも残さず説明する。
- C++（`.cpp` / `.h`）は **BOM付きUTF-8・CRLF**。シェーダー（`.hlsl` / `.hlsli`）は **BOMなしUTF-8**（fxc は BOM があると `X3000` で失敗する）。
- 文字セットは **マルチバイト**（ひな形が `char` 文字列のため）。`/utf-8` オプションは付けない。サブシステムは Windows。
- 定数バッファは `shader.h` / `shader.cpp` の構造体と `shaderCommon.hlsli`（b0〜b4）の並び・番号をそろえる。
- 地形の式（高さ・法線・色）は `field.cpp` にある。地面の上に物を置くときは `FieldGetHeight(x, z)` で高さを調べる。
- ファイルを足したら `.vcxproj` と `.vcxproj.filters` にも追加する（`save-work.ps1` も登録漏れを止める）。
- ビルド確認：
  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" Proto1.vcxproj /p:Configuration=Debug /p:Platform=x64 /v:minimal /nologo`
  （Visual Studio のエディション／パスはPCによって違う場合がある）

## ドキュメント
- `docs/plan/`（企画書。ゲームの目的・確かめたいこと・仕様）
- `docs/team/01_管理者向け_環境構築.md` / `02_メンバー向け_環境構築.md` / `03_ブランチ運用ルール.md` / `04_コマンド一覧.md`
- `docs/dev/01_DX11基盤の構成.md`（プログラムの構成と、オブジェクトの足し方）
- `docs/dev/02_カメラの解説.md`（カメラ・マウス・キーボード・カメラの当たり判定のコードの解説。行番号付き）
- `docs/lessons.md`（AI の失敗と教訓。作業前に読む・ミスのたびに追記する。Codex と共有。HEW2026GitTest から引き継いだ）
- `docs/ai/01_Codexとの相互レビュー.md`（管理者だけが使う、Claude と Codex の相互レビューの手順。Codex への指示は `AGENTS.md`）
- スクリプトの動きを変えたり、新しいスクリプトを足したりしたら、これらのドキュメント（特に `04_コマンド一覧.md`）も合わせて直す。
