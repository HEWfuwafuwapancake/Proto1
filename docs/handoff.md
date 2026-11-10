# 引き継ぎメモ（今の状態・次にやること）

作業を始めるときに読み、終えるときに書き直す。**ここには「今の状態」と「次にやること」だけを書く**（細かい記録は [archive/](archive/) へ移す）。
タスクの状態は [tasks.md](tasks.md)（Notion の「[Proto1（腕ダフルアームズ）](https://app.notion.com/p/e5e9f1983bbe4758b06b4adba135a491)」と同じ）、決まったことは [decisions.md](decisions.md)。
必要なときだけ読むもの：腕の書き方と共通の部品は [dev/01_DX11基盤の構成.md](dev/01_DX11基盤の構成.md) の「腕の共通の仕組み・腕の足し方」と「腕の書き方」、腕の操作と効き方は [plan/03_新しいアームの仕様.md](plan/03_新しいアームの仕様.md)、腕のモデルは [dev/02_腕のモデル.md](dev/02_腕のモデル.md)、画面を撮る方法は [dev/04_画面確認の方法.md](dev/04_画面確認の方法.md)。

- 最終更新：2026-11-10（Claude。Shota の Proto1、Visual Studio Community の PC。No.102 完了）

---

## 1. 今の状態

- **No.94〜100 完了**。宙に浮く高台4つと、腕6本（触手・ハンマー・ナックル・C4・ガンランス・刀）が develop に入った。腕は 1〜6 ですべて選べる。足りない武器の部品は mayapy で作った（`docs/dev/02_腕のモデル.md`）。各腕の作りは `docs/dev/01_DX11基盤の構成.md` の「腕の共通の仕組み・腕の足し方」。No.101 で単押し・長押しの判定、上を狙う触手、突進2倍、刀（握る形・居合・ワープ）を見直した。No.102 で F / R3 のフリーカメラを足した。
- No.48・51〜54・57 の実機確認待ちは [archive/handoff_2026-11.md](archive/handoff_2026-11.md) の「2026-11-04 に handoff.md から移したこと」。

## 2. 次にやること

- **次のタスクは管理者に聞く**（腕の置き換えのプランは終わった。tasks.md の ⬜ は優先度が低いものだけ）。
- 決めること（管理者）：`GuardCalc`・`StatusCalc` のしびれ以外・Q / E の段の切り替えは、腕の置き換えのあいだ触らず残している。残すか消すかを決める。
- 使わなくなった OBJ（磁石・グラップリング・盾の板・ハサミの残り）は消していない。

### 管理者の実機確認

- No.102：実機のパッドの R3 で切り替わることを見る。Claude はキーボードの F と矢印キーで確認した（オンラインの DESYNC は見ていない）。
- No.101：実機で単押し・長押しの感触（`armpresscalc.cpp` の `HOLD_SECONDS`・`TAP_BUFFER_SECONDS`）、見上げるカメラ（`camera.cpp` の `PITCH_MAX`）、刀の数（`arm_katana.cpp`）を見る。Claude はキーボードとマウスの SOLO だけ確認した。居合の受け流し（相手の攻撃が要る）・オンラインの DESYNC・実機のパッドは見ていない。
- No.94〜100：実機で腕6本を遊んで、数は各腕の `.cpp` の「調整用の値」で直す。Claude はキーボードとマウスの SOLO だけ確認した（オンラインの DESYNC と実機のパッドは見ていない）。遊んで気になったら直す点：高台から落ちる間はカメラが元の高さのままで、着地まで一瞬キャラが画面の下に外れる（`camera.cpp` の `m_PlayerGoal.Y`）。高台と浮島が同じ色で、高台の上だとふちが見分けにくい（`stage.cpp` の `STAGE_COLOR`）。
- No.93：実機のパッドの十字キーで腕を選べて、未実装の 3〜6 を飛ばすことを見る（Claude はキーボードの数字キーと単押しだけ確認した。オンラインは未確認）。
- No.91：実機で走り出し・走り続け・急停止・振り向きの腕を見る。流れ方は `ROPE_AIR_DRAG`、揺れの残り方は `ROPE_DAMPING`、振られ方は `ROPE_BODY_FOLLOW`、肩の範囲は `ROPE_SHOULDER_*`（`tentacle.cpp`）で直す。Claude は真横からの止め絵（走り・停止）と数値のシミュレーションで見た（横走り・後ろ走りと、実際の動いている滑らかさは見ていない）。
- No.90：実機で動かして、脚と上半身の向き・後ろ走り・ジャンプ／着地・傾きが自然か見る。強すぎ／弱すぎなら `fighter.cpp` の「調整用の値」（`LEG_TURN_SPEED`・`MAX_TWIST`・`BACK_RUN_ANGLE`・`LEAN_FORWARD`・`LEAN_SIDE_MAX`）と、Maya のポーズ（`assets/models/source/mekagirl_motion.mb`）を直す。Claude は画面を止めて撮った絵でしか見ていない（オンラインの DESYNC は見ていない。描画だけの変更）。
- No.89：実機のパッドで、LOCAL VS のロビーで 2P がキャラを変えられることと、MekaGirl の右腕のポーズを一度見る（Claude はキーボードで 1P だけ確認した）。MekaGirl の腕の付け根の位置が気になれば `character.cpp` の肩の値を直す。
- Claude の画面確認はキーをパッドの代わりにした一時ビルドなので、実機のパッドで一度だけ触る。まだ見ていないのは、やり直し後も手の選択が残ることと、時間切れ後も切り替えられること。
- No.86 のついでに、Visual Studio で **テスト → テスト エクスプローラー** を開き、Proto1Tests のテストが出て「すべて実行」で通るかを一度見る（出ないときは、Visual Studio Installer の「C++ によるデスクトップ開発」で「Google Test の Test Adapter」に印があるか確かめる）。
- メンバーは `git lfs pull` で `third_party/assimp` の lib・dll と `assets/models/` のメカの FBX を取ってからビルドする。
