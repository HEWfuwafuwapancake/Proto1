# Proto1 チーム制作 — Claude向けのプロジェクトメモ

DirectX 11（C++ / Visual Studio 2022）で作る 3D ゲームの**プロトタイプ**のチーム制作リポジトリ。
GitHub の Organization（HEWfuwafuwapancake）上の public リポジトリで、git-flow（main / develop / feature / release / hotfix）で運用する。
DX11 の土台は `HEW2026GitTest` から持ってきた（水のレイトレーシングは入れていない）。企画書は `docs/plan/` に置く。

## 進め方のルール
- **セッションを始めるとき**（Claude・Codex 共通。同じ `Proto1` を交代で使うので、もう一方の AI が作業中でないことも確かめる）：
  1. `docs/handoff.md`（今の状態・次にやること）と `docs/tasks.md` を読む。`docs/dev/01_DX11基盤の構成.md` はコードを書くときに読む。
  2. Notion の「[Proto1 開発ログ](https://app.notion.com/p/939057f5e85e4bdb9d6d1d3bc1bc28da)」（`collection://26982506-1bf2-4345-b4d9-be248dbacd7c`）の **「タイトル」「要点」「対応モジュール」の列だけ**を SQL で読み、同じミスをしない。本文（起きたこと／原因）は、今の作業に関係する行だけ開く。
  3. `git status` だけを先に見る（自分のものではない変更があればユーザーに聞く）→ `.\tools\fetch-latest.ps1` → `gh pr list`。
  4. Notion の「Proto1（腕ダフルアームズ）」と `tasks.md` の状態がそろっているか確かめる（ずれていたらユーザーに聞く）。
  5. handoff.md の「次にやること」から始める（プランが確認待ちなら、確認を取ってから実装する）。
- **作業を終えるとき**：`docs/handoff.md` の「今の状態」「次にやること」「最終更新」（PC と AI も）を書き直す。**handoff.md に書くのはこの2つだけ**。終わったタスクの記録は消すか `docs/archive/` へ移し、使い終わったプランは消す（git の履歴に残る）。
  `tasks.md` と Notion の状態をそろえる。途中で終わるときは `save-work.ps1` で GitHub に送っておく。
  今も効いている決まりが会話で決まったら、`docs/decisions.md` に1行足す。
- **ミスをして直したときや、ユーザーに間違いを指摘されたとき**は、Notion の「Proto1 開発ログ」に1行足す（タイトル・要点（次からどうするか、1〜2行）・対応モジュール・種別・書いたAI・日付。本文に起きたこと／原因／次からどうするか）。
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

## コードの構成（フォルダ分け）
| フォルダ | 入れるもの |
|---|---|
| `src/core/` | ウィンドウ・メインループ（`main`）、DX11（`dx11`）、シェーダーの読み込み（`shader`）、時間、デバッグ出力 |
| `src/input/` | キーボード・マウス・パッド、入力を「行動」に変える仕組み |
| `src/render/` | 形（箱・球・カプセル）・2D・数字の表示など、描画の部品 |
| `src/game/` | プレイヤー・カメラ・ステージ・敵・触腕・試合の流れ（ゲームの中身） |
| `src/physics/` | 当たり判定・吹っ飛びの計算（**DirectX を使わない**。単体テストする） |
| `shaders/` | `.hlsl` / `.hlsli` |
| `tests/` | 単体テストのプロジェクト |

- `#include` は **`src` からのパス**で書く（例：`#include "core/dx11.h"`、`#include "game/player.h"`）。同じフォルダのファイルでも省略しない。
- 新しいフォルダを作るときは、この表と `docs/dev/01_DX11基盤の構成.md` に追記し、`.vcxproj.filters` にも同じ名前のフィルターを作る。
- **ゲームの計算（当たり判定・吹っ飛び・スコアなど）は、DirectX を使わないファイルに分ける。** 描画するファイルは、計算した結果を使って描くだけにする。
- 依存の向き：`game` は `core`・`input`・`render`・`physics` を使ってよい。`core`・`input`・`render`・`physics` は `game` を使わない（`main.cpp` だけは全体をまとめるので例外）。
- 調整する値（速さ・強さ・時間など）は、各ファイルの上の「調整用の値」に `static constexpr` でまとめる。

## コードの約束
- 授業（DX21）のひな形をもとにした DX11 基盤。構成は `docs/dev/01_DX11基盤の構成.md`。書き方はひな形に合わせる（`〇〇Initialize / Finalize / Update / Draw`）。
- クラスは `TentacleCalc`・`Player` のように命名する。メンバー変数は `private` に置いて `m_` を頭に付け、読むときは `GetXxx() const` を使う。`Initialize / Finalize / Update / Draw` はメンバー関数名で残す。クラスと `public` なメンバー関数・メンバー変数には、すべて Doxygen 形式の日本語コメントを付ける。
- ヘッダーで公開する関数・構造体には **Doxygen 形式の日本語コメント**（`/// @brief` など）を付ける。bool を受け取る関数は「〜を設定する（true：〜、false：〜）」と書き、構造体はメンバーを1つも残さず説明する。
- C++（`.cpp` / `.h`）は **BOM付きUTF-8・CRLF**。シェーダー（`.hlsl` / `.hlsli`）は **BOMなしUTF-8**（fxc は BOM があると `X3000` で失敗する）。
- 文字セットは **マルチバイト**（ひな形が `char` 文字列のため）。`/utf-8` オプションは付けない。サブシステムは Windows。
- 定数バッファは `src/core/shader.h` / `shader.cpp` の構造体と `shaders/shaderCommon.hlsli`（b0〜b4）の並び・番号をそろえる。
- ステージは空に浮かぶ四角い浮島が1つ（`src/game/stage.cpp`。地形 `field.cpp` は今は使っていない）。浮島の大きさ・場外の下の線は `stage.cpp` の「調整用の値」、計算は `src/physics/stagecalc.h`。
  浮島の上に物を置くときは `StageGet().TopY` の高さ、浮島の範囲かは `StageCalcIsOnPlatform`、場外かは `StageCalcIsOut` で調べる。
- ファイルを足したら `.vcxproj` と `.vcxproj.filters` にも追加する（`save-work.ps1` も登録漏れを止める）。
- ビルド確認：
  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" Proto1.vcxproj /p:Configuration=Debug /p:Platform=x64 /v:minimal /nologo`
  （Visual Studio のエディション／パスはPCによって違う場合がある）
  scratchpad など **一時フォルダ（%TEMP%）の中でビルドするときは `/t:Rebuild`** を付ける（一時フォルダではヘッダーの変更が追跡されず、古いままのビルドになることがある。警告 MSB8029）。

## 単体テスト
- テストのプロジェクトは `tests/Proto1Tests/`（Visual Studio 標準の C++ 単体テスト）。テストのファイルは `〇〇_test.cpp`。
- 実行：`.\tools\run-tests.ps1`（Debug）／`.\tools\run-tests.ps1 -Release`。毎回作り直してから実行する。Visual Studio のテスト エクスプローラーからも実行できる。
- **テストするのは DirectX を使わないファイル**（`src/physics/`、`src/core/mathutil.h` など）。テストのプロジェクトからは、そのファイルの `.cpp` だけを「既存の項目」で追加してコンパイルする（ゲーム本体の `.cpp` をまるごと入れない）。
- 計算（当たり判定・吹っ飛び・スコア）を足したり変えたりしたら、テストも足す。PR の前に `run-tests.ps1` が全部通ること。
- テストを足したら、一度わざと計算を壊して、そのテストが失敗することを確かめる（失敗しないテストは意味がない）。
  **新しく足した計算のファイルだけ**を対象に、壊すところをまとめて1回で確かめる（前からあるファイルを少し変えただけなら、テストが通ることだけでよい）。

## 確認と記録を軽くする（トークン節約。2026-10-23、管理者が決めた）
- **画面を撮る確認**は、見た目が変わるタスクだけ。計算だけのタスクは、単体テストとビルド（Debug / Release、警告 0）で済ませる。
- **PR の説明**は短く：「やったこと」3〜5行と「確認したこと」だけ。細かい説明はコードのコメントと `docs/` に書く。
- **Notion** はステータスと PR のリンクだけ更新する。プランの本文は `docs/handoff.md` の「次にやること」だけに書く。
- **`docs/handoff.md`** の「今の状態」は短い要約（数行）だけ。タスクごとの状態の表は `docs/tasks.md` を見る。

## ドキュメント
- **`docs/handoff.md`（引き継ぎメモ。今の状態・次にやることだけ）**
- `docs/decisions.md`（会話の中で管理者が決めた、今も効いている決まり。関係する作業のときに読む）
- `docs/archive/`（終わったタスクの記録。普段は読まない）
- `docs/plan/`（企画書。ゲームの目的・確かめたいこと・仕様）
- **`docs/tasks.md`（開発タスクの一覧と優先度。上から順に「プラン → 管理者の確認 → 実装 → テスト → 完了チェック」で進め、終わったら状態を ✅ にする）**
- `docs/team/01_管理者向け_環境構築.md` / `02_メンバー向け_環境構築.md` / `03_ブランチ運用ルール.md` / `04_コマンド一覧.md`
- `docs/dev/01_DX11基盤の構成.md`（プログラムの構成と、オブジェクトの足し方）
- `docs/dev/02_腕のモデル.md`（腕の OBJ の原点・長さ・部品分け。腕のプランや取付位置を決めるときに読む）
- Notion の「Proto1 開発ログ」（AI の失敗と教訓。旧 `docs/lessons.md`。Codex と共有）
- `docs/ai/01_Codexとの相互レビュー.md`（管理者だけが使う、Claude と Codex の相互レビューの手順。Codex への指示は `AGENTS.md`）
- スクリプトの動きを変えたり、新しいスクリプトを足したりしたら、これらのドキュメント（特に `04_コマンド一覧.md`）も合わせて直す。
