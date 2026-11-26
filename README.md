# ブラキオンファイト（Proto1）

変な腕で相手をつかむ・殴る・投げて、場外へぶっ飛ばす **3D 乱闘アクション**のプロトタイプです（企画名「腕ダフルアームズ」）。
C++ と DirectX 11（Visual Studio 2022）で作っています。HEW のチーム制作のリポジトリです。

- 最大4人（同じ PC で分割画面、またはオンライン）。空いた枠には CPU かダミーが入る。
- 腕は8種類（触手・ハンマー・ナックル・レーザー・刀・モーニングスター・槍・ミサイル）。カプセルを拾って左右別々に装備する。
- 殴られるほどダメージ％がたまり、％が高いほど遠くへ飛ぶ（スマブラ式）。相手を場外にすると +1。

## 動かす

1. 環境を準備する：[メンバー向けの手順](docs/team/02_メンバー向け_環境構築.md)（何も入っていない PC から1〜2時間）
2. `Proto1.sln` を Visual Studio で開き、**F5** でビルドして起動する
3. 操作は [操作一覧](docs/team/05_操作一覧.md)（キーボードだけでも遊べる）

## 作業のしかた

作業は必ず `feature/〇〇` のブランチで行い、`tools/` のスクリプトを使います（`develop` と `main` には直接 push できません）。

```powershell
.\tools\start-work.ps1 player-move     # 作業を始める
.\tools\save-work.ps1 "やったこと"       # こまめに保存して GitHub に送る
.\tools\submit-pr.ps1                  # 終わったらプルリクエスト
```

コマンドの一覧は [04_コマンド一覧.md](docs/team/04_コマンド一覧.md)、ブランチのルールは [03_ブランチ運用ルール.md](docs/team/03_ブランチ運用ルール.md)。

## 文書の地図

| 知りたいこと | 文書 |
|---|---|
| 今どうなっていて、次に何をするか | [docs/handoff.md](docs/handoff.md)（引き継ぎメモ） |
| まだ終わっていないタスク | [docs/tasks.md](docs/tasks.md) |
| 管理者が決めた決まり | [docs/decisions.md](docs/decisions.md) |
| ゲームの企画・ルール・腕の仕様 | [docs/plan/](docs/plan/README.md) |
| プログラムの構成と、新しいものの足し方 | [docs/dev/01_DX11基盤の構成.md](docs/dev/01_DX11基盤の構成.md) |
| 環境の準備・コマンド・操作 | [docs/team/](docs/team/02_メンバー向け_環境構築.md) |
| 終わったタスクの記録 | [docs/archive/](docs/archive/tasks_done.md)（普段は読まない） |

AI（Claude Code・Codex）への指示は `CLAUDE.md` と `AGENTS.md` にあります。
