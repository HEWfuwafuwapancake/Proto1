# 引き継ぎメモ（今の状態・次にやること）

作業を始めるときに読み、終えるときに書き直す。**ここには「今の状態」と「次にやること」だけを書く**（細かい記録は [archive/](archive/) へ移す）。
タスクの状態は [tasks.md](tasks.md)（Notion の「[Proto1（腕ダフルアームズ）](https://app.notion.com/p/e5e9f1983bbe4758b06b4adba135a491)」と同じ）、決まったことは [decisions.md](decisions.md)。
必要なときだけ読むもの：腕の書き方と共通の部品は [dev/01_DX11基盤の構成.md](dev/01_DX11基盤の構成.md) の「腕の共通の仕組み・腕の足し方」と「腕の書き方」、腕の操作と効き方は [plan/03_新しいアームの仕様.md](plan/03_新しいアームの仕様.md)、腕のモデルは [dev/02_腕のモデル.md](dev/02_腕のモデル.md)、画面を撮る方法は [dev/04_画面確認の方法.md](dev/04_画面確認の方法.md)。

- 最終更新：2026-11-07（Codex。Shota の Proto1、Visual Studio Community の PC。No.89 段階2の実装と Debug x64 ビルドが済んだ）

---

## 1. 今の状態

- **No.89 段階2 済み（Codex が実装）。Claude の確認待ち**（ブランチ `feature/character-select`。Debug x64 で本体・テストプロジェクトをビルド済み）。
- **No.88 完了**（[PR #147](https://github.com/HEWfuwafuwapancake/Proto1/pull/147)。実装 Codex・レビュー Claude）。キャラは骨つきのメカ（`render/skinnedmodel`）で、待機・走り・左右の腕のポーズを切り替える。アニメは試合中だけ進む（時間切れ後は止まる）。肩の位置は `arm.cpp` の固定値で、アニメの肩には追従しない。
- **No.87 完了**。静止モデルの `ModelLoad` は Assimp 5.2.4 で FBX とテクスチャを読み込む（No.21 も完了）。説明は [dev/01_DX11基盤の構成.md](dev/01_DX11基盤の構成.md) の「FBX モデルとテクスチャ」。
- **No.86 完了**：単体テストを Google Test（`third_party/googletest` v1.15.2）に移した。290 件すべて Debug / Release で成功、警告 0。書き方は [dev/01_DX11基盤の構成.md](dev/01_DX11基盤の構成.md) の「単体テスト」。Visual Studio のテスト エクスプローラーで見えるかは、まだ実機で見ていない。
- **No.85 完了**（PR #141）：パッドの十字キー ← → で手を選び、↓ ↑ で腕を一周（ハサミは飛ばす）。今の腕は左下・右下に日本語で出て、替えるとキューブのように回る。オンラインを始めると全員の装備を触腕に戻す。キーをパッドの代わりにした一時ビルドで確認し、オンライン2つ起動で DESYNC なし。
- **追加のアーム No.59〜82 はすべて完了**（PR #114〜#137。管理者の指示で Codex に渡さず Claude が実装・確認・マージした）。腕は 25 種類（触腕・グラップリング・ドリル・磁石・デカ盾 ＋ 新しい 20 本。ハサミ No.40 は最後）。
  - 腕の選び方：Q（左手）・E（右手）を押す回数で 1〜3段目、数字キー 1〜9・0。並びは [plan/03_新しいアームの仕様.md](plan/03_新しいアームの仕様.md) と `arm.h` の `ARM_TYPE`。
  - 新しい腕は Maya のモデルがないので円柱・箱・球の仮の形。数（秒・距離・ダメージ・強さ）は各 `arm_〇〇.cpp` の「調整用の値」にあり、プランの初期値のまま（ブレードの押す速さだけ 3 → 1.5）。
  - 共通の土台：`ArmPhaseCalc`・`AreaCalc`（No.59）、`ProjectileCalc`（No.70）、状態異常 `StatusCalc`（No.75）、ガード `GuardCalc`（No.80。攻撃した人の体の中心から判定する。飛び道具も撃った人の位置で判定する近似）。
  - 確認：Debug / Release 警告0・テスト 287 件成功・新しい計算の壊す確認・腕ごとの画面確認（ダミー相手）・オンライン（127.0.0.1 の2つ起動）で新しい腕を使って DESYNC なし。腕ごとの記録は [archive/handoff_2026-11.md](archive/handoff_2026-11.md) の「追加のアーム No.59〜82」。
- **まだ確かめていないこと**（ダミーは攻撃もガードもしないので、LOCAL VS の2人＝キーボード＋パッドで確かめる）：
  - ガードが効くこと（デカ盾の前・バリアの全方向・ワープ直後 0.3 秒）と、アックスのガード崩し。
  - スモークの中の人がロックオンされないこと。
  - リペアで自分の % が減ること（計算はテスト済み）。
  - サンダーの連鎖（ダミーが離れているので画面では未確認。計算はテスト済み）。
- **No.83 完了**：起動は最大化した枠ありのウィンドウ（全画面は `--fullscreen` と F11 / Alt+Enter のときだけ）。モニター1枚ではデバッグ用ウィンドウが右上の最前面に出る。No.84 で、ウィンドウの大きさ（最大化・枠を引っぱる・F11）に合わせて黒い帯を付け、ゲームの比率を保つようにした。
- 前からの実機での確認待ち（No.48・51〜54・57）は [archive/handoff_2026-11.md](archive/handoff_2026-11.md) の「2026-11-04 に handoff.md から移したこと」。

## 2. 次にやること

- **No.89 の段階3を Claude が担当する**。下のプランと実装を照合し、確認が通ったら PR・マージへ進む。

### No.89 キャラ選択と MekaGirl（プラン確定・管理者確認済み 2026-11-07。実装 Codex → 確認 Claude）

ブランチ `feature/character-select`（作成済み）。素材は `assets/models/mekagirl_idle.fbx`・`mekagirl_run.fbx`・`mekagirl_armL_forward.fbx`・`mekagirl_armR_forward.fbx`（コピー済み。元は `Downloads/Meka-Girl_FBX`。`AllClips` は使わない）。

**MekaGirl のデータ**（Assimp で調べた値）
- 骨 20 本（pelvis・spine_01/02・chest・head・hair_back・脚・車輪 6 つ）。**腕の骨・メッシュはない**（腕はゲームの腕を肩から出す今の仕組みのまま）。部品 25、材質 1（色は黒・テクスチャなし。今と同じくプレイヤーの色で塗る）。
- 高さ 110.76、**足の高さ 0**（今のメカは 54.7・足 -2.9）。幅 ±13.8。
- **1ファイルにアニメが2本**（`Take 001` と名前つきの `Idle`／`Run`／`ArmForward_L`／`ArmForward_R`。長さは同じ）。待機 72f・走り 16f・腕 72f、24fps。今の `ReadClip` は1本だけを受け付けるので、このままだと読めない。

**やること**
1. `render/skinnedmodel`：1体分のグローバル（`g_Nodes`・`g_Meshes`・`g_Clips`）を `SkinnedModel` クラスに変え、`Load / Finalize / Reset(pose) / Update(pose, ...) / Draw(pose, ...)` をメンバー関数にする。アニメが2本以上あるファイルは `Take 001` 以外の1本を使う（1本だけならそれ）。骨の数が違うモデルの姿勢を渡されたら `Reset` で作り直す。
2. `game/character.h/.cpp`（新規）：`CHARACTER_TYPE { CHARACTER_MEKA, CHARACTER_MEKAGIRL, CHARACTER_COUNT }` と、キャラごとの表（表示名・FBX 4つ・モデルの高さ・足の高さ・向き・肩の位置 X/Y/Z）。`fighter.cpp` の `MODEL_*` と `arm.cpp` の `SHOULDER_OFFSET`・`SHOULDER_FORWARD`（と肩の高さ）をこの表から読む。MekaGirl の肩は胸（chest）の横の高さを仮の値にして、画面確認で直す。
3. 2つのモデルは `FighterManager` が持ち、`Fighter::SetCharacter(CHARACTER_TYPE)` で選ぶ。**当たり判定（体の高さ・太さ）は共通**（見た目だけ違う。計算・同期に影響させない）。ダミーはメカ。
4. 選び方（案A）：
   - 新しいシーン `SCENE_CHARACTER_SELECT` を GAME START とモード選択の間に入れる。← → でキャラ（メカ／MekaGirl）、決定で次へ、戻るで GAME START。選んでいるキャラの名前を出す（モデルを回して見せるのは不要）。
   - この選択は「メニューを操作した人」（SOLO の1人・LOCAL VS の 1P・オンラインの自分）のキャラになる。
   - LOCAL VS のロビーで参加した人は、← →（パッドの十字キー・キーボードの A / D）で自分のキャラを変えられる（1P も）。参加者ごとにキャラ名を出す。
   - RETRY では残す。タイトル（GAME START）へ戻ったら、ロビーで変えた 2P 以降はメカに戻す。
   - ← → で一周する計算と、シーンの遷移は `src/physics/scenecalc`（DirectX なし）に置き、テストを足す（主な動きだけ）。
5. オンライン：`SESSION_MESSAGE` にスロットごとのキャラ（`uint8_t` を `FIGHTER_COUNT` 個）を足す。参加側は JOIN（と機器選択）で自分のキャラを送り、ホストがロビーの状態で全員分を配る。試合開始時にそのキャラを Fighter へ設定する。版（`LOCKSTEP_VERSION`）を上げる。変換のテストを足す。ロックステップの計算・チェックサムには入れない。
6. ドキュメント：`docs/dev/01_DX11基盤の構成.md` の「FBX モデル」にキャラの足し方（表に1行足す）を書く。`.vcxproj`・`.filters` に新しいファイルを入れる。

**確認（Claude）**：Debug / Release 警告0・`run-tests.ps1`（Debug・Release）・新しい計算を壊してテストが落ちる確認・画面確認（MekaGirl の大きさ・向き・足・肩の位置、待機／走り／左右の腕、キャラ選択画面とロビーの表示）・オンライン（127.0.0.1 の2つ起動）で相手のキャラが見えて DESYNC なし。

**段階3への申し送り**

- MekaGirl は FBX 高さ 110.76・足元 0、ゲーム内の描画用の肩は胸の横を想定して X=0.22 / Y=1.28 / Z=0.0、向きの補正は π を仮設定した。正面・足元・肩のつながりと、4つの名前つきアニメが再生されるかを画面で確かめる。
- 腕は描画の付け根だけキャラ別。攻撃の起点は従来のメカの値を全キャラで使う。触腕が敵を保持したときは、描画用の肩から計算済みの先端へつなぐ。
- LOCAL VS の決定を押した機器を 1P に割り当てる。ロビーで変えたキャラは RETRY に残り、オフラインの結果から TITLE を選ぶと GAME START に戻る。2P 以降はメカへ戻り、1P の初期値はキャラ選択画面で選んだ値に戻る。オンラインの TITLE はロビーに戻って全員の選択を保つ。
- キャラ番号は DirectX を使わない scenecalc.h に置き、game/character.h から参照する。通信では JOIN・READY・STATE にだけ番号を含める。版は 7。旧版クライアントの JOIN は新版ホストで版不一致を返せるが、新版クライアントから旧版ホストへ送ると旧版側が新しい長さの JOIN を破棄するため、接続待ちになる。
- Debug x64 の本体とテストプロジェクトをビルド済み。段階3では上の「確認」を行い、特に LOCAL VS で機器別にキャラを変更できることと、オンラインで両端末のキャラが一致することを確かめる。

### 管理者の実機確認

- Claude の画面確認はキーをパッドの代わりにした一時ビルドなので、実機のパッドで一度だけ触る。まだ見ていないのは、やり直し後も手の選択が残ることと、時間切れ後も切り替えられること。
- No.86 のついでに、Visual Studio で **テスト → テスト エクスプローラー** を開き、Proto1Tests のテストが出て「すべて実行」で通るかを一度見る（出ないときは、Visual Studio Installer の「C++ によるデスクトップ開発」で「Google Test の Test Adapter」に印があるか確かめる）。
- メンバーは `git lfs pull` で `third_party/assimp` の lib・dll と `assets/models/` のメカの FBX を取ってからビルドする。
