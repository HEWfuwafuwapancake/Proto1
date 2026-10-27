# 引き継ぎメモ（今の状態・次にやること）

作業を始めるときに読み、終えるときに書き直す。**ここには「今の状態」と「次にやること」だけを書く。**
タスクの状態は [tasks.md](tasks.md)（Notion の「[Proto1（腕ダフルアームズ）](https://app.notion.com/p/e5e9f1983bbe4758b06b4adba135a491)」と同じ）。
決まったことは [decisions.md](decisions.md)、腕のモデルの原点と長さは [dev/02_腕のモデル.md](dev/02_腕のモデル.md)、終わったタスクの記録は [archive/](archive/)（普段は読まない）。

- 最終更新：2026-10-27（Claude。handoff.md を短くし、lessons.md を Notion の開発ログに移した）

---

## 1. 今の状態

- No.1〜17・20・22・29〜37・41・42 は完了。次は **No.38 磁石**（プランは管理者確認済み・Codex の実装待ち）。残りは No.39 デカ盾・No.40 ハサミ・No.18 シーン・No.19 調整用の値の整理ほか。
- **No.39・No.40 の前に**：`arm_shield_plate.obj`・`arm_hasami_base.obj` に長さ 0 の法線（`vn 0 0 0`）があり読み込めない。`.\tools\export-arm-obj.ps1 shield`／`hasami` で書き出し直す。
- **管理者が遊んで決めること**：触腕の構え（`IDLE_LENGTH`・`ARM_REST_PITCH`）が普段の画面で分かるか／グラップリング・ドリルの取付位置と強さ（各 `arm_〇〇.cpp` の「調整用の値」。No.19 の一覧に入れる）／実物のパッドで十字キー ← →・□ ○ の腕の切り替え。
- マウス操作は止めてある（[decisions.md](decisions.md) の最初の項目）。

## 2. 次にやること：No.38（磁石）

- 管理者が「No.38 を Codex に頼むプロンプト」を Codex に貼り、Codex の保存のあと「No.38 を Claude が確かめるときのプロンプト」を Claude に貼る。
- 順番（管理者が決めた）：No.38 磁石 → No.39 デカ盾 → No.40 ハサミ（最後）。No.18・No.19 はプランがまだ（アームのあとで管理者が選ぶ）。
- 画面の確認のヒント：カメラは敵を見続けるので、腕を横から見たいときは `camera.cpp` の `MakeMatrices` でビューを置きかえた一時ビルドを、OutDir・IntDir を scratchpad に分けて作る（コミットしない。終わったら `git checkout` で戻す）。**ビューの見ている点を `ArmGetShoulder(手)` にすると、肩が画面の真ん中に来て、部品が肩についているか分かる**。

### 新しいアームの仕様（管理者確認済み、2026-10-26。各腕の細かい値は、その腕のプランで決める）

- **全部の腕で同じ**：操作は今と同じ L1 / R1（左右クリック）だけ。腕ごとに「押したとき」と「効いている間にもう一度押したとき」を決める（「操作はシンプルに」）。
  装備は試すための切り替えだけ（十字キー ← → で左手、□ ○ で右手。キーボードは 1〜6 で左手、Shift＋1〜6 で右手）。腕カプセルは作らない。色は今の左右の色と、効いている間の色だけ。
- **作り方**：`src/game/arm.cpp` が左右の装備を持って腕ごとの処理に振り分ける。腕ごとの処理は `src/game/arm_〇〇.cpp`、計算は `src/physics/〇〇calc.h`（DirectX なし・テストする）。今の `tentacle.cpp` は「触腕の処理」として使う。
  描画は今の `ModelLoad` と `TentacleCalcPieceAxes` を使う（そのため OBJ の向きは触腕と同じ -Y が伸びる向きにそろえる）。グラップリングのワイヤーは細い円柱で描く。

| 腕 | 押したとき | もう一度押したとき | 敵への効き方 |
|---|---|---|---|
| 触腕（今のまま） | 伸ばしてつかむ | 投げる | % ＋12・吹っ飛ぶ |
| グラップリング（No.36） | フックを撃ち、刺さった所へ自分が引っぱられて飛ぶ。刺さるのは敵と浮島のふち | 途中で押すと離す | 敵なら着いたときの体当たりで小さく吹っ飛ばす（＋5）。ふちに刺されば場外から戻れる |
| ドリル（No.37） | 伸ばさずに突き出す（近い敵だけ）。当たった敵に 0.5 秒くい込み % が小刻みに大きく増える（＋4 × 5回） | なし（自動で戻る） | 最後に水平に押し出す（上にはあまり飛ばない） |
| 磁石（No.38） | ターゲット（いなければ正面）を自分の前まで引き寄せる（ダメージなし） | 引き寄せている間なら反発して強く吹っ飛ばす | 反発で % ＋8 |
| デカ盾（No.39） | 押している間、前に構えて遅く歩く | 構えている間なら盾で押す | 前の敵を % に関係なく一定の強さで押し出す（% ＋0）。ガードの効き目は No.24 のあと |
| ハサミ（No.40・最後） | 相手の腕を切り落とす | — | 切られた相手は新しい腕を探す。相手が腕を使う No.24 と、腕を拾う No.27（腕カプセル）のあと |

### No.38 のプラン：磁石（管理者確認済み、2026-10-26）

- **やること**：L1 / R1 で、肩から狙いの向きへ見えない「磁力の線」を出し、当たったいちばん近い敵を自分の前まで引き寄せる（ダメージなし）。引き寄せている間・前で止めている間にもう一度押すと、反発して強く吹っ飛ばす（% ＋8）。押さなければ少しして離す。
  腕の部分は `arm_magnet.cpp` を足して `ARM_TABLE` の1行を変えるだけ（`tentacle.cpp`・`arm_grapple.cpp`・`arm_drill.cpp` は触らない）。敵は触腕と同じ `DummyGrab`・`DummyMoveHeld`・`DummyRelease` で動かす（`dummy` は変えない）。
- **動き**（数字は `arm_magnet.cpp` の「調整用の値」。仮の値で、管理者が遊んで決める）：
  1. 押した瞬間（待機中だけ）：向きは触腕と同じ `TentacleCalcAim`（水平。ターゲットがいればその方向、いなければ画面の正面）。肩からその向きへ `RANGE` 10、半径 `BEAM_RADIUS` 0.8 のカプセル（`CollisionCapsuleCapsule`。浮島には当たらない）と重なる `Available` な敵のうち、肩にいちばん近い敵を `DummyGrab` して「引き寄せ中」。だれにも当たらなければ「休み」。
     - ターゲットが浮島の上（同じくらいの高さ）で 10 以内なら、その向きへ線が出るので当たる（`TentacleCalcAim` は高さを捨てるので、ずっと下へ落ちている敵には当たらない）。「いなければ正面」は、画面の正面にいる敵を引き寄せる意味になる。
  2. 引き寄せ中：敵の体の中心を「引き寄せ先」（肩から**キャラの正面**へ `HOLD_FORWARD` 2.0、高さは肩。触腕のつかむ位置と同じ距離）へ、1秒に `PULL_SPEED` 15 でまっすぐ動かす（`DummyMoveHeld`。行き過ぎない）。0.1 以内に着いたら「止めている」。プレイヤーは歩ける（引き寄せ先もついてくる）。
  3. 止めている間：毎フレーム引き寄せ先に置く。`HOLD_SECONDS` 1.5 秒たったら、吹っ飛ばさずに離す（`DummyRelease(番号, false, …)`。その場から落ちる）→「休み」。
  4. 引き寄せ中・止めている間にもう一度押す：**反発**。触腕の投げと同じく画面の正面へ `DummyRelease(番号, true, 画面の正面, REPEL_DAMAGE 8, REPEL_POWER 2.5)`（触腕の投げは ＋12・強さ 2.0）。振動は触腕の投げと同じ値。→「休み」。
  5. 取りやめ：敵が消えた（場外）・プレイヤーが復活した → 吹っ飛ばさずに離して「休み」。
  6. 休み（`COOLDOWN_SECONDS` 0.3 秒）：押しても何もしない。終わったら待機。
  - 引き寄せた瞬間は短く振動（触腕のつかむと同じ値）。色：引き寄せ中・止めている間は `ArmGetColor(手, true)`。
  - **カメラ**：引き寄せ中・止めている間は `IsHoldingHand` を true にする（触腕でつかんでいる間と同じく、ロックオンが外れて自由に回せ、離すとロックオンに戻る。反発の向きを自分で選べる）。
- **見た目**（`ArmLoadModel`・`ArmDrawPiece` を使う。伸ばさない1部品）：
  - `arm_magnet.obj`（長さ 17.42 cm、大きさ 0.08 → 約 1.4）を肩から描く。向きは待機中は正面を `ARM_REST_PITCH` で下げた向き、引き寄せ中・止めている間は肩から敵の中心への水平の向き。`MAGNET_AIM_SECONDS`（0.1 秒）でなめらかに変える（`ArmCalcBlendDirection`。ドリルと同じ作り）。ひねりなし。
  - モデルは肩から少し進んですぐ +Z（ゲームの上）へ約 10 cm 曲がり、先の磁石は肩から約 (-2.9, -17.4, +9.7) cm（腕の線より上に出る）。左右は触腕と同じ `ArmLoadModel`（右手を鏡写し）。**実装のあと Claude が肩を中心にした一時ビルドで画面を撮り、取付位置と向きを管理者に見せる**（ずれていたら調整用の値で直す）。
- **新しいファイル・変えるファイル**：
  - `src/physics/magnetcalc.h/.cpp`（DirectX なし・テストする）：`MAGNET_STATE`（待機／引き寄せ中／止めている／休み）、`MAGNET`（状態・その状態になってからの時間・引き寄せた敵（なしは -1）・敵の中心）、`MAGNET_PARAMS`（`RANGE`・`BEAM_RADIUS`・`PULL_SPEED`・着いたとみなす距離・`HOLD_SECONDS`・`COOLDOWN_SECONDS`）、
    `MagnetCalcReset`・`MagnetCalcBeam`（肩と向きから線のカプセル）・`MagnetCalcStep`（引き寄せる敵を選ぶ・中心を引き寄せ先へ動かす・着く・時間切れで離す・反発・取りやめ・休み。「つかんだ敵」「離した敵」「反発するか」を返す）。
  - `src/game/arm_magnet.cpp`：`ARM_MAGNET_FUNCTIONS`。`arm.cpp` の `ARM_TABLE` の磁石の行を `&ARM_MAGNET_FUNCTIONS` にする（4 / Shift＋4 / Q・E のあと 4 で選べる）。`DummyGrab` が失敗したら（同じフレームにもう一方の腕がつかんだ）待機に戻す（触腕と同じ）。
- **テスト**（`magnetcalc_test.cpp`）：待機中に押すと、線の中のいちばん近い `Available` な敵を引き寄せる／10 より遠い・`Available` でない敵は選ばない・だれもいなければ休み → 0.3 秒で待機／中心は 1秒に 15 で引き寄せ先へ動き行き過ぎない（長いフレームでも）／着いたら止めている・引き寄せ先が動けばついていく／1.5 秒で吹っ飛ばさずに離して休み／引き寄せ中・止めている間に押すと反発して休み／敵が消えた・復活したら吹っ飛ばさずに離す／休み中に押しても何も変わらない／dt が 0 以下なら進まない。
- **確かめること（Claude）**：Debug / Release 警告0、`run-tests.ps1`、`magnetcalc` を壊すと足したテストが失敗する。画面：4 でタイトルバー「左：磁石」、待機で 45° 下に磁石の腕が見える（肩を中心にした一時ビルドで取付位置・向きを拡大して管理者に見せる）／押すとターゲットが前まで飛んできて止まる（% は 0 のまま）／もう一度押すと画面の正面へ吹っ飛び ＋8%／押さなければ 1.5 秒で落ちる／引き寄せ中もカメラを回せる／1 で触腕に戻る。
- **やらないこと**：ほかの腕・腕カプセル・HUD・エフェクト（磁力の線の見た目）。触腕・グラップリング・ドリルの値と動き。`dummy` の変更。プレイヤーの動き。

### No.38 を Codex に頼むプロンプト

```text
Proto1 の No.38「磁石」を実装してください。プランは docs/handoff.md の「No.38 のプラン：磁石（管理者確認済み）」と「新しいアームの仕様」です。プランにないことはしません。

## 始める前に
- CLAUDE.md・AGENTS.md・docs/handoff.md・docs/tasks.md・docs/dev/01_DX11基盤の構成.md と Notion の開発ログ（関係するモジュールの行）を読み、Claude が作業中でないことを確かめる（git status）。
- .\tools\start-work.ps1 arm-magnet
- docs/tasks.md の No.38 を 🔄 にする（Notion を書き換えられなければ、最後に「No.38 を進行中に」と管理者に伝える）。

## いちばん大事なこと
- 磁石の腕は伸びない（arm_magnet.obj の1部品を肩から描くだけ）。敵を引き寄せるのは見えない線（カプセル）で、敵は触腕と同じ DummyGrab・DummyMoveHeld・DummyRelease で動かす。dummy は変えない。
- 腕の部分は arm_magnet.cpp を足して ARM_TABLE の1行を変えるだけ。tentacle.cpp・arm_grapple.cpp・arm_drill.cpp・その計算と値は触らない。
- 計算（敵を選ぶ・引き寄せる・着く・時間切れで離す・反発・取りやめ・休み）は DirectX を使わない src/physics/magnetcalc に置き、テストを書く。arm_drill.cpp・drillcalc の作りをまねる。

## やること
1. src/physics/magnetcalc.h/.cpp：プランの MAGNET_STATE・MAGNET・MAGNET_PARAMS・MagnetCalcReset・MagnetCalcBeam・MagnetCalcStep。当たりは CollisionCapsuleCapsule、敵は TENTACLE_TARGET の配列（Available だけ、肩にいちばん近いもの）。引き寄せは1秒に PULL_SPEED、長いフレームでも行き過ぎない。Doxygen コメント（構造体のメンバーは全部）。
2. src/game/arm_magnet.cpp：ARM_MAGNET_FUNCTIONS（arm.h で宣言済みか確かめる）。調整用の値はプランの数字（RANGE 10・BEAM_RADIUS 0.8・PULL_SPEED 15・着いたとみなす距離 0.1・HOLD_FORWARD 2.0・HOLD_SECONDS 1.5・REPEL_DAMAGE 8・REPEL_POWER 2.5・COOLDOWN_SECONDS 0.3・MAGNET_AIM_SECONDS 0.1・MODEL_SCALE 0.08・長さはモデルの書き出し値 17.424655）。振動は触腕のつかむ・投げると同じ値。
   - 引き寄せ先は肩からキャラの正面へ HOLD_FORWARD（高さは肩）。反発の向きは触腕の投げと同じ画面の正面。DummyGrab が失敗したら待機に戻す。
   - IsHoldingHand は引き寄せ中・止めている間 true。ResetHand は触腕と同じくつかんでいる敵を吹っ飛ばさずに離す。
   - 描画：ArmLoadModel（右手を鏡写し）・ArmDrawPiece。向きは ArmCalcBlendDirection(正面を ArmGetRestPitch() で下げた向き, 肩から敵の中心への水平の向き, まっすぐ度)。引き寄せ中・止めている間は ArmGetColor(手, true)。
   - arm.cpp の ARM_TABLE の磁石の行を &ARM_MAGNET_FUNCTIONS にする。
3. テスト：tests/Proto1Tests/magnetcalc_test.cpp にプランの「テスト」を全部（magnetcalc.cpp をテストのプロジェクトに「既存の項目」で足す）。新しいファイルは .vcxproj と .vcxproj.filters に登録する（フィルター名は既存の登録を読んでそろえる）。
4. C++ は BOM付きUTF-8・CRLF。docs/dev/01_DX11基盤の構成.md に磁石の節を短く書く。
5. Debug のビルドが通ることだけ確かめ、.\tools\save-work.ps1 "No.38 磁石" で保存する。docs/handoff.md の「今の状態」に「No.38 は Codex が実装。Claude の確認待ち」と、迷ったことを短く書く。

## やらないこと
- run-tests.ps1・Release のビルド・わざと壊す確認・画面の確認・PR・マージ（Claude がやる）。
- ほかの腕（盾・ハサミ）・腕カプセル・HUD・エフェクト（磁力の線の見た目）。プレイヤーの動きを変えること。触腕・グラップリング・ドリルの値と動き。dummy の変更。

## 報告すること（短く）
- 足した・変えたファイル
- プランと違うことをしたところ、迷ったところ
```

### No.38 を Claude が確かめるときのプロンプト（モデル：Opus。Codex の保存のあと、管理者が Claude Code の新しいセッションに貼る）

```text
Proto1 の No.38（磁石）を Codex が feature/arm-magnet に push しました。Claude の担当分（確かめる → PR → マージ）をやってください。
- CLAUDE.md の手順どおり handoff.md・tasks.md と Notion の開発ログを読み、Codex が作業中でないことを確かめてから .\tools\start-work.ps1 arm-magnet。
- プラン（handoff.md の No.38）と違うところがないかコードを読む。特に「腕は伸びない1部品」「敵は DummyGrab・DummyMoveHeld・DummyRelease で動かし、dummy は変えていない」「tentacle・grapple・drill の計算と値は前と同じ」「計算が magnetcalc にありテストがある」「IsHoldingHand が引き寄せ中・止めている間だけ true」。
- arm_magnet.obj に vn 0 0 0 がないこと（Notion の開発ログの「モデルの読み込みが失敗していたのに…」）。
- Debug / Release 警告0、.\tools\run-tests.ps1 -Release も含めて全部通る。magnetcalc の新しい計算をまとめて壊すと、足したテストが失敗する。
- 画面を撮る（ゲームが前に出ていることを確かめてから入力を送る。Notion の開発ログ）：
  4・Shift＋4 でタイトルバー「左：磁石」／待機で 45° 下に磁石の腕が見える（肩を中心にした一時ビルドで取付位置・向きを拡大して管理者に見せる。handoff.md の「画面の確認のヒント」）／押すとターゲットが前まで飛んできて止まる（% は 0 のまま）／もう一度押すと画面の正面へ吹っ飛び ＋8%／押さなければ 1.5 秒で落ちる／1 で触腕に戻る。
- 直すところはこのブランチに直接コミットしてよい。tasks.md の No.38 を ✅、Notion の No.38 を「完了」にして PR のリンクを入れ、PR を出してマージする（テストが通っていれば聞かずにマージしてよい）。
- 終わったら handoff.md の「次にやること」を No.39 デカ盾（プランはまだ。先に arm_shield_plate.obj を書き出し直す）にし、管理者に No.39 のプランを出す。
```
