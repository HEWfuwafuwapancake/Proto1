# 引き継ぎメモ（今の状態・次にやること）

作業を始めるときに読み、終えるときに書き直す。**ここには「今の状態」と「次にやること」だけを書く。**
タスクの状態は [tasks.md](tasks.md)（Notion の「[Proto1（腕ダフルアームズ）](https://app.notion.com/p/e5e9f1983bbe4758b06b4adba135a491)」と同じ）。
決まったことは [decisions.md](decisions.md)、腕のモデルの原点と長さは [dev/02_腕のモデル.md](dev/02_腕のモデル.md)、終わったタスクの記録は [archive/](archive/)（普段は読まない）。

- 最終更新：2026-10-27（Claude。Shota の共有 Proto1。No.38 を確認してマージ。No.39 のプランが管理者確認済みになり、プロンプトを書いた）

---

## 1. 今の状態

- **No.38（磁石）は完了**（Codex が実装、Claude が確認してマージ）。確認で、磁石の先が腕の線より下に描かれていたのを、ドリルと同じく半回転（`MODEL_ROLL`＝π）で上に直した（OBJ は読み込みで Z が反転する）。
- 管理者が遊んで決めること：全種類の構えが普段の画面で分かるか／グラップリング・ドリル・磁石の取付位置と強さ（No.19）／実物のパッドでの腕切り替え。マウス操作は止めたまま（[decisions.md](decisions.md)）。

## 2. 次にやること：No.39（デカ盾）を Codex が実装する

- 管理者が下の「No.39 を Codex に頼むプロンプト」を Codex に貼る。Codex の保存のあと、「No.39 を Claude が確かめるときのプロンプト」を Claude Code の新しいセッションに貼る。下の確認済みプランは、Claude の確認が終わるまで残す。
- 順番（管理者が決めた）：No.39 デカ盾 → No.40 ハサミ（最後）。No.18・No.19 はプランがまだ（アームのあとで管理者が選ぶ）。
- 画面の確認のヒント：カメラは敵を見続けるので、腕を横から見たいときは `camera.cpp` の `Camera::MakeMatrices` でビューを置きかえた一時ビルドを、OutDir・IntDir を scratchpad に分けて作る（コミットしない。終わったら `git checkout` で戻す）。**ビューの見ている点を肩（`ArmCalcShoulder` に `Player` の位置・向きを渡した点）にすると、肩が画面の真ん中に来て、部品が肩についているか分かる**。真横（キャラの左へ 2.5、前へ 0.6、肩と同じ高さ）から見ると、部品が上下どちらに曲がっているか分かる。

### 新しいアームの仕様（管理者確認済み、2026-10-26。各腕の細かい値は、その腕のプランで決める）

- **全部の腕で同じ**：操作は今と同じ L1 / R1（左右クリック）だけ。腕ごとに「押したとき」と「効いている間にもう一度押したとき」を決める（「操作はシンプルに」）。
  装備は試すための切り替えだけ（十字キー ← → で左手、□ ○ で右手。キーボードは 1〜6 で左手、Shift＋1〜6 で右手）。腕カプセルは作らない。色は今の左右の色と、効いている間の色だけ。
- **作り方**（No.44 のあと）：腕ごとの処理は `src/game/arm_〇〇.h / .cpp` の `Arm` の派生クラス、計算は `src/physics/〇〇calc.h / .cpp` の DirectX なしのクラス（テストする）。`arm.cpp` の `ARM_FACTORIES` の1行を `CreateArm<〇〇Arm>` にする。
  描画は `Arm::LoadModel` と `Arm::DrawPiece`（中で `TentacleCalcPieceAxes`）を使う（そのため OBJ の向きは触腕と同じ -Y が伸びる向きにそろえる）。グラップリングのワイヤーは細い円柱で描く。

| 腕 | 押したとき | もう一度押したとき | 敵への効き方 |
|---|---|---|---|
| 触腕（今のまま） | 伸ばしてつかむ | 投げる | % ＋12・吹っ飛ぶ |
| グラップリング（No.36） | フックを撃ち、刺さった所へ自分が引っぱられて飛ぶ。刺さるのは敵と浮島のふち | 途中で押すと離す | 敵なら着いたときの体当たりで小さく吹っ飛ばす（＋5）。ふちに刺されば場外から戻れる |
| ドリル（No.37） | 伸ばさずに突き出す（近い敵だけ）。当たった敵に 0.5 秒くい込み % が小刻みに大きく増える（＋4 × 5回） | なし（自動で戻る） | 最後に水平に押し出す（上にはあまり飛ばない） |
| 磁石（No.38） | ターゲット（いなければ正面）を自分の前まで引き寄せる（ダメージなし） | 引き寄せている間なら反発して強く吹っ飛ばす | 反発で % ＋8 |
| デカ盾（No.39） | 前に構えて遅く歩く（離しても構えたまま、2 秒で下ろす） | 構えている間なら盾で押す | 前の敵を % に関係なく一定の強さで押し出す（% ＋0）。ガードの効き目は No.24 のあと |
| ハサミ（No.40・最後） | 相手の腕を切り落とす | — | 切られた相手は新しい腕を探す。相手が腕を使う No.24 と、腕を拾う No.27（腕カプセル）のあと |

### No.39 のプラン：デカ盾（管理者確認済み、2026-10-27。押し方は A）

- **やること**：L1 / R1 を押すと盾を前に構え（離しても構えたまま）、構えている間は遅く歩く。構えている間にもう一度押すと、盾で前の敵を押し出す（% に関係なく同じ速さ、% ＋0）。押さなければ 2 秒で下ろす。ガードの効き目（自分が受けるダメージを減らす）は No.24 のあと。
  ボタンが1つなので、仕様の「押している間構える」は「押すと構える（離しても構えたまま）」にした（磁石の「押す → もう一度押す」と同じ操作）。
- **動き**（数字は `arm_shield.cpp` の「調整用の値」。仮の値で、管理者が遊んで決める）：
  1. 待機中に押す → 「構え」。`GUARD_RAISE_SECONDS` 0.1 秒で腕を待機の向きからキャラの正面・水平へ（`ArmCalcBlendDirection`）。構えている間、歩く速さ × `GUARD_MOVE_SCALE` 0.4。
  2. 構えている間にもう一度押す → 「押し出し」：肩からキャラの正面（`GetPlayer().GetYaw()`。水平）へ長さ `BASH_RANGE` 2.0・半径 `BASH_RADIUS` 0.9 のカプセル（`CollisionCapsuleCapsule`。浮島には当たらない）に重なる `Available` な敵を**全部**、キャラの正面（水平）へ速さ `BASH_SPEED` 12、上向き `BASH_UP` 0.15（水平の向きに上を足して正規化）で押し出す（% ＋0。% に関係なく同じ速さ）。押し出しは押した瞬間の1回だけ。腕は `BASH_SECONDS` 0.15 秒だけ構えたまま、前へ `BASH_REACH` 0.3 突き出して見せる → 「休み」。
  3. 押さずに `GUARD_SECONDS` 2.0 秒たつ → 押し出さずに下ろす →「休み」。
  4. 休み（`COOLDOWN_SECONDS` 0.3 秒）：押しても何もしない。終わったら待機（腕は 0.1 秒で待機の向きへ戻る）。
  5. 取りやめ：プレイヤーが復活した（`GetPlayer().DidRespawn()`）→ 押し出さずに休み。
  - 押し出した瞬間は触腕の投げと同じ振動（0.6・0.6・0.15 秒）。だれにも当たらなくても押し出しの動きはする（振動なし）。色：構え・押し出し中は `GetColor(true)`。
  - 敵はつかまないので `IsHolding()` は false（カメラはロックオンのまま）。`IsActive()` は待機以外で true。
- **今の作りに小さく足すもの**（ほかの腕のファイルは触らない）：
  - 遅く歩く：`Arm` に `virtual float GetMoveScale() const`（基本クラスで 1.0 を返す。`ShieldArm` は構え・押し出し中に `GUARD_MOVE_SCALE`）、`ArmManager::GetMoveScale()`（左右の今の腕の小さいほう）。`Player::Update` で `params.MoveSpeed = MOVE_SPEED * m_Arms->GetMoveScale()`。プレイヤーは腕より先に動くので、1フレーム前の構えで遅くなる（気にならない差）。ジャンプは変えない。
  - % に関係ない押し出し：`DummyManager::Push(int index, const VEC3& velocity)`（`IsActive` で `Held` でない敵だけ。% を足さず、速度を与えて地面から離し、吹っ飛び中（`Launched`）にする）。`Launch` は % で速さが変わるので使わない。
- **見た目**（`LoadModel`・`DrawPiece` を使う。伸ばさない2部品）：
  - `arm_shield_base.obj`（長さ 19.250364 cm、倍率 0.08）を共通の `GetShoulder()` から描く。向きは待機が `GetRestDirection()`、構え・押し出しはキャラの正面へ水平（`ArmCalcBlendDirection`、まっすぐ度は 0.1 秒で変える。`MagnetArm` と同じ作り）。
  - `arm_shield_plate.obj`（長さ 2.901904 cm）は base の取付位置に base と同じ軸で描く。取付位置は OBJ 読み込み後（Z 反転済み）のモデル座標で `PLATE_OFFSET` (0.229075, -15.022320, -0.102787) cm（Maya の pivot の差）。`GrappleCalcModelPoint(PLATE_OFFSET, 肩, base の先, MODEL_SCALE, 右手なら true, MODEL_ROLL)` で始点を、そこから base と同じ向きへ板の長さ分を終点にして `DrawPiece`。
  - ひねり `MODEL_ROLL` は**まず 0** で書き、Claude が画面で上下を確かめて決める（OBJ は読み込みで Z が反転する。No.38 では π が要った）。両方の OBJ に `vn 0 0 0` はない（確かめ済み）。
- **新しいファイル・変えるファイル**：
  - `src/physics/shieldcalc.h / .cpp`（DirectX なし・テストする。`drillcalc`・`magnetcalc` の作りをまねる）：
    データだけのもの：`SHIELD_STATE`（待機／構え／押し出し／休み）、`SHIELD_PARAMS`（`GUARD_SECONDS`・`BASH_SECONDS`・`COOLDOWN_SECONDS`・`BASH_RANGE`・`BASH_RADIUS`）。
    `class ShieldCalc`：状態・その状態になってからの時間を `private` の `m_〇〇` に持ち、`Reset()`・`Step(trigger, cancel, dt, params)`（このフレームに押し出したら true）・`GetState()`・`GetTime()`。
    状態を使わない自由関数：`ShieldCalcBashCapsule(肩, 水平の向き, params)`、`ShieldCalcPushVelocity(水平の向き, 速さ, 上向き)`。
  - `src/game/arm_shield.h / .cpp`：`class ShieldArm : public Arm`（コンストラクタ `(int hand, const ARM_LINKS& links)` で2つのモデルを読んで `Reset()`。`Update`・`Draw`・`Reset`・`IsActive`・`IsHolding`・`GetMoveScale` を `override`）。押し出したフレームに、カプセルに重なる `Available` な敵へ `GetDummies().Push(…)`。
  - `src/game/arm.h / .cpp`：`GetMoveScale` の追加、`#include "game/arm_shield.h"`、`ARM_FACTORIES` の盾の行を `CreateArm<ShieldArm>`（5 / Shift＋5 / Q・E のあと 5 で選べる）。
  - `src/game/dummy.h / .cpp`：`Push` の追加だけ。`src/game/player.cpp`：`MoveSpeed` に掛けるだけ。
- **テスト**（`shieldcalc_test.cpp`。主な動きだけ）：待機で押すと構え／構え中に押すと押し出し（その1フレームだけ true）→ 0.15 秒で休み → 0.3 秒で待機／押さなければ 2 秒で休み（押し出さない）／休み中に押しても何もしない／取りやめで休み／カプセルは肩から水平の向きへ 2.0（高さを捨てる）／押し出しの速度は水平の向き＋上向きで、長さが速さと同じ。
- **確かめること（Claude）**：Debug / Release 警告0、`run-tests.ps1`、`shieldcalc` を壊すと足したテストが失敗する。画面：5 でタイトルバー「左：デカ盾」、待機で 45° 下に盾の腕（肩を中心にした一時ビルドで真横から上下を確かめ、管理者に見せる）／押すと前に構えて遅く歩く／もう一度押すと前の敵が % 0 のまま押し出される（% が高い敵でも同じくらい飛ぶ）／押さなければ 2 秒で下ろす／1 で触腕に戻る。
- **やらないこと**：ガードの効き目（No.24）、ほかの腕の値と動き、腕カプセル・HUD・エフェクト、ジャンプの高さ。

### No.39 を Codex に頼むプロンプト

```text
Proto1 の No.39「デカ盾」を実装してください。プランは docs/handoff.md の「No.39 のプラン：デカ盾」と「新しいアームの仕様」です。プランにないことはしません。

## 始める前に
- CLAUDE.md・AGENTS.md・docs/handoff.md・docs/tasks.md・docs/dev/01_DX11基盤の構成.md（特に「腕を足すとき」と「磁石（No.38）」）と Notion の開発ログ（関係するモジュールの行）を読み、Claude が作業中でないことを確かめる（git status）。
- .\tools\start-work.ps1 arm-shield
- docs/tasks.md の No.39 を 🔄 にする（Notion を書き換えられなければ、最後に「No.39 を進行中に」と管理者に伝える）。

## いちばん大事なこと
- 盾の腕は伸びない（arm_shield_base.obj と arm_shield_plate.obj の2部品を肩から描くだけ）。押し方は「押すと構える（離しても構えたまま）→ 構え中にもう一度押すと盾で押す → 押さなければ 2 秒で下ろす」。
- 腕の部分は Arm を派生した ShieldArm（arm_shield.h/.cpp）を足して、arm.cpp の ARM_FACTORIES の1行を CreateArm<ShieldArm> にする。今の作りに足すのは、Arm::GetMoveScale（基本クラスは 1.0）と ArmManager::GetMoveScale、Player::Update で MoveSpeed に掛けること、DummyManager::Push だけ。tentacle・arm_grapple・arm_drill・arm_magnet とその計算・値は触らない。
- 押し出しは % を足さず、% に関係なく同じ速さ（DummyManager::Push。Launch は使わない）。
- 計算（構え・押し出し・下ろす・休み・取りやめと、押し出しのカプセル・速度）は DirectX を使わない src/physics/shieldcalc の class ShieldCalc と自由関数に置き、テストを書く。MagnetArm・MagnetCalc の作りをまねる。

## やること
1. src/physics/shieldcalc.h/.cpp：プランの SHIELD_STATE・SHIELD_PARAMS・class ShieldCalc（Reset・Step・Get〇〇、状態は private の m_〇〇）・ShieldCalcBashCapsule・ShieldCalcPushVelocity。Doxygen コメント（クラス・public のメンバー、構造体のメンバーは全部。bool の引数は「〜を設定する（true：〜、false：〜）」）。
2. src/game/arm_shield.h/.cpp：class ShieldArm : public Arm。調整用の値はプランの数字（GUARD_SECONDS 2.0・GUARD_MOVE_SCALE 0.4・GUARD_RAISE_SECONDS 0.1・BASH_SECONDS 0.15・BASH_REACH 0.3・BASH_RANGE 2.0・BASH_RADIUS 0.9・BASH_SPEED 12・BASH_UP 0.15・COOLDOWN_SECONDS 0.3・MODEL_SCALE 0.08・長さ 19.250364 と 2.901904・PLATE_OFFSET (0.229075, -15.022320, -0.102787)・MODEL_ROLL 0）。振動は触腕の投げと同じ値。
   - 構え・押し出しの向きはキャラの正面（GetPlayer().GetYaw()）の水平。復活（GetPlayer().DidRespawn()）で休み。
   - IsHolding は常に false、IsActive は待機以外 true、GetMoveScale は構え・押し出し中だけ GUARD_MOVE_SCALE。
   - 描画：LoadModel（右手を鏡写し）・DrawPiece。板の始点は GrappleCalcModelPoint(PLATE_OFFSET, 肩, base の先, MODEL_SCALE, 右手なら true, MODEL_ROLL)。構え・押し出し中は GetColor(true)。
3. arm.h/.cpp に GetMoveScale を足し、#include "game/arm_shield.h" と ARM_FACTORIES の盾の行。dummy.h/.cpp に Push、player.cpp で MoveSpeed に ArmManager::GetMoveScale() を掛ける。
4. テスト：tests/Proto1Tests/shieldcalc_test.cpp にプランの「テスト」を全部（shieldcalc.cpp をテストのプロジェクトに「既存の項目」で足す）。新しいファイルは .vcxproj と .vcxproj.filters に登録する（フィルター名は既存の登録を読んでそろえる）。
5. C++ は BOM付きUTF-8・CRLF。docs/dev/01_DX11基盤の構成.md に盾の節を短く書く（GetMoveScale と Push も）。
6. Debug のビルドが通ることだけ確かめ、.\tools\save-work.ps1 "No.39 デカ盾" で保存する。docs/handoff.md の「今の状態」に「No.39 は Codex が実装。Claude の確認待ち」と、迷ったことを短く書く。

## やらないこと
- run-tests.ps1・Release のビルド・わざと壊す確認・画面の確認・MODEL_ROLL を決めること・PR・マージ（Claude がやる）。
- ガードの効き目（No.24）・ほかの腕（ハサミ）・腕カプセル・HUD・エフェクト。ジャンプの高さ。触腕・グラップリング・ドリル・磁石の値と動き。

## 報告すること（短く）
- 足した・変えたファイル
- プランと違うことをしたところ、迷ったところ
```

### No.39 を Claude が確かめるときのプロンプト（モデル：Opus。Codex の保存のあと、管理者が Claude Code の新しいセッションに貼る）

```text
Proto1 の No.39（デカ盾）を Codex が feature/arm-shield に push しました。Claude の担当分（確かめる → PR → マージ）をやってください。
- CLAUDE.md の手順どおり handoff.md・tasks.md と Notion の開発ログを読み、Codex が作業中でないことを確かめてから .\tools\start-work.ps1 arm-shield。
- プラン（handoff.md の No.39）と違うところがないかコードを読む。特に「押すと構える（離しても構えたまま）→ 構え中にもう一度押すと押し出し → 2 秒で下ろす」「ShieldArm は Arm の派生で、arm.cpp は ARM_FACTORIES の1行・include・GetMoveScale だけ」「GetMoveScale は基本クラスが 1.0 でほかの腕のファイルは触っていない」「Push は % を足さず、Launch を使っていない」「dummy は Push の追加だけ、player は MoveSpeed に掛けるだけ」「tentacle・grapple・drill・magnet の計算と値は前と同じ」「計算が class ShieldCalc にありテストがある」「m_ と Doxygen コメント」。
- Debug / Release 警告0、.\tools\run-tests.ps1 -Release も含めて全部通る。shieldcalc の新しい計算をまとめて壊すと、足したテストが失敗する。
- 画面を撮る（ゲームが前に出ていることを確かめてから入力を送る。Notion の開発ログ）：
  5・Shift＋5 でタイトルバー「左：デカ盾」／待機で 45° 下に盾の腕／押すと前に構えて遅く歩く／もう一度押すと前の敵が % 0 のまま押し出される（% が高い敵でも同じくらい飛ぶ）／押さなければ 2 秒で下ろす／1 で触腕に戻る。
- 盾の上下の向き：肩を中心にした一時ビルドで真横から撮り（handoff.md の「画面の確認のヒント」）、MODEL_ROLL が 0 と π のどちらが自然か比べて、両方の絵を管理者に見せて決めてもらう（No.38 では π が要った）。板が base の手についているかも見る。
- 直すところはこのブランチに直接コミットしてよい。tasks.md の No.39 を ✅、Notion の No.39 を「完了」にして PR のリンクを入れ、PR を出してマージする（テストが通り、管理者が向きを決めていれば聞かずにマージしてよい）。
- 終わったら handoff.md の「次にやること」を No.40 ハサミ（No.24・No.27 のあと）か、No.18・No.19 のどれにするか管理者に聞くことにする。
```

