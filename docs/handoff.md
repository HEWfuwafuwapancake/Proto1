# 引き継ぎメモ（今の状態・次にやること）

作業を始めるときに読み、終えるときに書き直す。**ここには「今の状態」と「次にやること」だけを書く。**
タスクの状態は [tasks.md](tasks.md)（Notion の「[Proto1（腕ダフルアームズ）](https://app.notion.com/p/e5e9f1983bbe4758b06b4adba135a491)」と同じ）。
決まったことは [decisions.md](decisions.md)、腕のモデルの原点と長さは [dev/02_腕のモデル.md](dev/02_腕のモデル.md)、終わったタスクの記録は [archive/](archive/)（普段は読まない）。

- 最終更新：2026-10-27（Claude。Shota の共有 Proto1。No.45 は Codex がまだ push していない。No.44 段階 C のプランとプロンプトを詳しくした（管理者確認済み）。次は No.45 → No.44 段階 C）

---

## 1. 今の状態

- **No.44 段階 A・B はマージ済み**。腕は基本クラス `Arm` と派生クラス `TentacleArm`・`GrappleArm`・`DrillArm`（片手1インスタンス）。`arm.cpp` は腕の種類 → 生成関数の表と、左右全種類のインスタンスを持つ。グラップリングの左右の所有権は `ConnectOtherArm`・`m_Pulling`。
- 段階 B の確認：Debug / Release 警告 0、テスト 259 件（Debug / Release）。画面で触腕のつかむ・投げる（+12%）、グラップリングで飛ぶ（+5%）、ドリル（+20% で押し出し）、左右グラップリング、切り替え・やり直しのあと歩けることを確認。時間切れ（90 秒）は待っていない（やり直しと同じ `ArmReset`）。
- 段階 D で直す古い説明：`docs/dev/01_DX11基盤の構成.md` の `ARM_FUNCTIONS`・`TentacleInitialize`・`TentacleUpdateHand`。
- **管理者が遊んで決めること**：全種類の構え（共通の `SHOULDER_FORWARD`・`ARM_REST_SPREAD`・`ARM_REST_PITCH`、触腕の `IDLE_LENGTH`）が普段の画面で分かるか／グラップリング・ドリルの取付位置と強さ（各 `arm_〇〇.cpp` の「調整用の値」。No.19 の一覧に入れる）／実物のパッドで十字キー ← →・□ ○ の腕の切り替え。
- マウス操作は止めてある（[decisions.md](decisions.md) の最初の項目）。

## 2. 次にやること：No.45（テストを減らす）→ No.44 段階 C

- **直近は No.45**：管理者が下の「No.45 を Codex に頼むプロンプト」を Codex に貼る。Codex の保存のあと「No.45 を Claude が確かめるときのプロンプト」を Claude に貼る。マージしたら No.44 段階 C へ。
- **そのあと No.44 段階 C**：No.45 のマージのあと、管理者が「No.44 段階 C を Codex に頼むプロンプト」を Codex に貼る（段階 C のプランは管理者確認済み）。Codex の保存のあと「No.44 段階 C を Claude が確かめるときのプロンプト」を Claude に貼る。

### No.45 のプラン：いらないテストを消す（管理者確認済み、2026-10-27。「中くらい」に減らす）

- **ねらい**：259 件のテストのうち、端っこ・おかしな入力だけを見るテストと、同じことを何度も確かめているテストを消し、テストを読む・直す手間を減らす。**ゲームのコード（`src/`）は変えない**。
- **消すテスト（73 件 → 残り 186 件）**。ここにないテストは消さない。消したテストだけが使っていた補助の関数・定数も消す。
  - `actioncalc_test.cpp`（4）：`MoveFromKeys_NoKeysIsZero`・`ClampLength_KeepsShortAxis`・`CombineAxis_ZeroUsesOther`・`EquipKey_ModeKeyAndNumberSameFrame`
  - `armcalc_test.cpp`（12）：`Rest_ZeroSpreadKeepsPreviousPitchDirection`・`Pitch_ZeroKeepsHorizontalForward`・`Pitch_NormalizesHorizontalForward`・`Pitch_IgnoresForwardHeight`・`Pitch_NoHorizontalForwardUsesPositiveZ`・`Blend_ClampsRatioAndNeverReturnsZeroForZeroInputs`・`Roll_PreservesUnitLengthsAndOrthogonality`・`Cycle_SkipsUnavailableAtBothEnds`・`Cycle_OnlyCurrentAvailableDoesNotChange`・`Cycle_NoAvailableReturnsMinusOne`・`Select_OutOfRangeReturnsMinusOne`・`InvalidArgumentsReturnMinusOne`
  - `cameracalc_test.cpp`（6）：`Clip_TargetInsideKeepsCamera`・`SmoothDamp_ZeroDtKeepsValueAndVelocity`・`SmoothDampAngle_ChangingGoalCarriesAngularVelocity`・`Focus_WrapsAngleAndIgnoresCoincidentTarget`・`NextTarget_CrossesPiInBothDirections`・`NextTarget_InvalidOrUnavailableCurrentReturnsNone`
  - `collision_test.cpp`（4）：`ClosestPointOnSegment_ZeroLength`・`ClosestPointsOfSegments_EndToEnd`・`SphereSphere_SameCenterPushesUp`・`HitRecord_FullRejects`
  - `drillcalc_test.cpp`（5）：`Reset_ClearsTargetTimeAndTicks`・`Strike_CapsuleRadiiAlsoCountAtTip`・`Strike_UnavailableEnemyDoesNotHit`・`RepeatedTriggersDoNotRestartActiveStatesOrChangeDirection`・`ZeroOrNegativeTime_DoesNotStartOrAdvance`
  - `dummycalc_test.cpp`（1）：`Push_ApartDoesNothing`
  - `gametime_test.cpp`（2）：`NegativeScale_BecomesZero`・`Initialize_ResetsElapsedAndScale`
  - `glyphcalc_test.cpp`（4）：`SplitDigits_NegativeAndOverflow`・`SegmentRect_Positions`・`NumberRects_MaxRects`・`TextRects_CountMatchesDots`
  - `grapplecalc_test.cpp`（2）：`ModelPoint_ZeroRollUsesHorizontalXForwardMinusYAndDownMinusZ`・`ModelPoint_StraightElbowMatchesFormerBaseForBothHands`
  - `mathutil_test.cpp`（3）：`Saturate_KeepsValueInRange`・`SmoothStep01_SlowNearEnds`・`Lerp_WorksWhenBIsSmaller`
  - `padcalc_test.cpp`（12）：`StickValue_EndsAndCenter`・`DeadZone_InsideIsZero`・`Parse_SticksUpIsPlus`・`Parse_OtherButtons`・`Parse_DpadNeutralAndReservedValues`・`Output_EndpointsAndRounding`・`Output_UsesGivenLength`・`Output_RejectsInvalidBuffer`・`Rumble_StrengthAndTimeAreIndependent`・`Rumble_ZeroAndNegativeDeltaDoNotAdvance`・`Rumble_NonPositiveDurationStops`・`Rumble_DiscardsExpiredStrength`
  - `playercalc_test.cpp`（5）：`Facing_CrossesPiByShortestPathInBothDirections`・`Facing_ZeroDtKeepsWalkingYawButAppliesArmYaw`・`Facing_TargetKeepsTurnLimitAndZeroDt`・`Facing_TargetCrossesPiByShortestPath`・`Step_StandingStill`
  - `stagecalc_test.cpp`（1）：`OnPlatform_CornerOutside`
  - `tentaclecalc_test.cpp`（9）：`Wiggle_VerticalAndZeroLength`・`Aim_TargetDirectlyAboveBelowOrAtShoulderUsesFallback`・`Aim_NearlyZeroHorizontalDistanceUsesFallback`・`Aim_FallbackIsHorizontalAndNormalized`・`Aim_DegenerateFallbackUsesPositiveZ`・`Aim_EachShoulderPointsToSameTarget`・`ExtendRatio_ZeroWhenNotExtending`・`ExtendRatio_ClampsToOne`・`TimeStopped_DoesNotExtendOrThrow`
  - `timer_test.cpp`（3）：`Update_ZeroDtDoesNotChange`・`Start_NegativeSecondsFinishesImmediately`・`ZeroInitialized_IsFinished`
  - `knockbackcalc`・`matchcalc`・`objcalc` は消さない（`objcalc` の `AssetModels_AllObjFilesParse`・`BrokenLines_FailAndClearOutput` など、前に不具合を見つけたテストは残す）。
- **確認**：Debug / Release 警告 0、`run-tests.ps1`（Debug / Release）が全部通り、件数が 186。わざと壊す確認・画面は要らない（計算は変えない）。

### No.45 を Codex に頼むプロンプト

```text
Proto1 の No.45「いらないテストを消す」を実装してください。プランは docs/handoff.md の「No.45 のプラン：いらないテストを消す（管理者確認済み）」です。プランにないことはしません。

## 始める前に
- CLAUDE.md・AGENTS.md・docs/handoff.md・docs/tasks.md と Notion の開発ログ（テスト・文字コード・C++ の行）を読み、Claude が作業中でないことを確かめる（git status）。
- .\tools\start-work.ps1 test-trim
- docs/tasks.md の No.45 を 🔄 にする（Notion を書き換えられなければ、最後に「No.45 を進行中に」と管理者に伝える）。

## いちばん大事なこと
- 消すのはプランの一覧にある 73 件の TEST_METHOD だけ。一覧にないテストは消さない・書き換えない。src/ は触らない。
- 消したテストだけが使っていた補助の関数・定数・#include も消す（ほかのテストが使っているものは残す）。

## やること
1. tests/Proto1Tests/ の各ファイルから、一覧のテストを消す。テストのファイルは BOM付きUTF-8・CRLF のまま（BOM のある先頭行はパッチの一致条件にしない）。
2. 消したあと、各ファイルの TEST_METHOD の数を数え、合計が 186 になることを確かめる。
3. テストのプロジェクトの Debug ビルドが通ることだけ確かめ、.\tools\save-work.ps1 "No.45 いらないテストを消す" で保存する。docs/handoff.md の「今の状態」に「No.45 は Codex が実装。Claude の確認待ち」と、迷ったことを短く書く。

## やらないこと
- run-tests.ps1・Release のビルド・PR・マージ（Claude がやる）。
- テストの書き換え・まとめ直し・新しいテスト。src/ の変更。

## 報告すること（短く）
- 変えたファイルと、ファイルごとの消した数
- プランと違うことをしたところ、迷ったところ
```

### No.45 を Claude が確かめるときのプロンプト（モデル：Sonnet でよい。Codex の保存のあと、管理者が Claude Code の新しいセッションに貼る）

```text
Proto1 の No.45（いらないテストを消す）を Codex が feature/test-trim に push しました。Claude の担当分（確かめる → PR → マージ）をやってください。
- CLAUDE.md の手順どおり handoff.md・tasks.md と Notion の開発ログを読み、Codex が作業中でないことを確かめてから .\tools\start-work.ps1 test-trim。
- 差分を読み、消えたのがプランの 73 件の TEST_METHOD とその補助だけで、src/ と残りのテストが変わっていないことを確かめる。
- Debug / Release 警告 0、.\tools\run-tests.ps1 と .\tools\run-tests.ps1 -Release が全部通り、件数が 186。画面・わざと壊す確認は要らない。
- 直すところはこのブランチに直接コミットしてよい。tasks.md の No.45 を ✅、Notion の No.45 を「完了」にして PR のリンクを入れ、PR を出してマージする（テストが通っていれば聞かずにマージしてよい）。
- 終わったら handoff.md の「次にやること」を No.44 段階 C にし、No.45 のプランとプロンプトを消す。
```

- 順番（管理者が決めた、2026-10-27）：**No.44 を No.38 より先にやる**。段階 A → B → C → D を1つずつ「実装 → Claude の確認 → PR → マージ」で進める（1段階＝1ブランチ＝1 PR）。
- 段階 A・B はマージ済み。残りは段階 C（Codex が実装）と段階 D（Claude が書く）。
- 全部マージしたら、Claude が下の「No.38 のプラン」とプロンプトを新しい作り（`MagnetArm` クラス）に書き直してから、Codex に頼む。

### No.44 のプラン：構造体のクラス化（管理者確認済み、2026-10-27）

- **ねらい**：状態を外から直接書き換える「構造体＋関数」と、ファイルの中のグローバル変数・関数ポインタの表をやめ、状態を `private` に持つクラスのメンバー関数で動かす。腕は基本クラスから派生させて足す。
- **どの段階でも、ゲームの動き・調整用の値・計算の結果は変えない**（作りを変えるだけ。見つけた不具合は直さずに報告する）。
- **変えないもの**：
  - データだけの構造体は `struct` のまま：`VERTEX_3D`・`MATERIAL`・`LIGHT`（`shaderCommon.hlsli` と並びをそろえる）、`VEC3`・`SPHERE`・`CAPSULE`・`BOX`・`HIT_RESULT`・`STAGE`、`〇〇_PARAMS`（`static constexpr` の調整用の値）、計算が返す結果（`〇〇_EVENT`・`TENTACLE_TARGET`・`TENTACLE_PIECE_AXES`・`DRILL_GRIP`・`DRILL_AXIS`・`CAMERA_FOCUS_TARGET`）。
  - 状態を持たない計算の関数（`collision`・`vec3`・`knockbackcalc`・`cameracalc`・`armcalc`・`stagecalc`、`TentacleCalcAim` などの状態を使わない関数）は今の自由関数のまま。
  - `src/core`・`src/input`・`src/render`（授業のひな形の部分。`TIMER`・`PAD_STATE` などもそのまま）、`field.cpp`（使っていない）、enum。
- **名前の付け方**（段階 A で `CLAUDE.md` の「コードの約束」に足す）：クラスは `TentacleCalc`・`Player` のような形、メンバー変数は `private` で `m_` を頭に付け、読むのは `GetXxx() const`。授業のひな形の流れに合わせ `Initialize / Finalize / Update / Draw` はメンバー関数の名前で残す。クラスと `public` なメンバー関数・メンバー変数は全部 Doxygen の日本語コメントを付ける。

#### 段階 A：計算のクラス化（`src/physics/` とテスト）

| 今 | 変えたあと |
|---|---|
| `TENTACLE`＋`TentacleCalcReset`・`TentacleCalcStep`・`TentacleCalcExtendRatio` | `class TentacleCalc`（`Reset`・`Step`・`GetExtendRatio`・状態を読む `Get〇〇`） |
| `GRAPPLE`＋`GrappleCalcReset`・`GrappleCalcStep` | `class GrappleCalc` |
| `DRILL`＋`DrillCalcReset`・`DrillCalcStep` | `class DrillCalc` |
| `MATCH`＋`MatchCalcStart`・`MatchCalcStep` | `class MatchCalc` |
| `DUMMY_LIFE`＋`DummyCalcResetLife`・`DummyCalcUpdateLife` | `class DummyLifeCalc` |
| `PLAYER_BODY`＋`PlayerCalcStep` | `class PlayerBodyCalc`（位置・速度・着地を読む・位置を設定する関数つき） |
| `HIT_RECORD`＋`HitRecordClear`・`HitRecordContains`・`HitRecordTryAdd` | `class HitRecord` |

- 引数・戻り値・計算の中身は今の関数と同じにする。`game/` の呼んでいるところは、新しい呼び方に置きかえるだけ。
- テストは同じ内容のまま新しい呼び方に直す（テストの数を減らさない。`run-tests.ps1` の数が前と同じ）。
- 確認：Debug / Release 警告0、`run-tests.ps1`（Debug / Release）。わざと壊す確認は要らない（新しい計算はない）。画面は撮らない。

#### 段階 B：腕を基本クラスと派生クラスにする（`src/game/arm*`・`tentacle*`）

- `arm.h` に基本クラス `class Arm`（純粋仮想関数：`Update(bool trigger)`・`Draw()`・`Reset()`・`IsActive() const`・`IsHolding() const`、仮想デストラクタ）。1つのインスタンスが1本の手（左 0・右 1）を受け持つ。
- `TentacleArm`（`tentacle.cpp`）・`GrappleArm`（`arm_grapple.cpp`）・`DrillArm`（`arm_drill.cpp`）が `Arm` を派生する。今の `g_〇〇[2]` はメンバー変数にする。モデルは左右で別に読むので、今どおりインスタンスごとに持つ。
- `arm.cpp` の `ARM_FUNCTIONS`・`ARM_TABLE`・`ARM_〇〇_FUNCTIONS` をなくし、腕の種類から作る表（`ARM_TYPE` → 作る関数、未実装は `nullptr`）にする。左右それぞれ、全種類のインスタンスを起動時に作って持ち、切り替えは今と同じく前の腕を `Reset()` してから変える。
- `ArmGetShoulder`・`ArmGetRestDirection`・`ArmGetColor`・`ArmDrawPiece`・`ArmLoadModel` などの共通の部品と、`ArmUpdate`・`ArmIsHolding` などの外への関数は、段階 C まで今のまま。
- 確認：Debug / Release 警告0、`run-tests.ps1`。画面：1〜3 と Shift＋1〜3 で全部の腕が前と同じに構える・動く（触腕でつかんで投げる／グラップリングで飛ぶ／ドリルで % が増えて押し出す）。

#### 段階 C：ゲームの部品と `Game` クラス（`src/game/`・`main.cpp`）（管理者確認済み、2026-10-27 に詳しくした）

- `Stage`・`Player`・`DummyManager`・`Camera`・`ArmManager`（左右の装備）・`Match`・`Hud` をクラスにし、ファイルの中のグローバル変数（`g_〇〇`）をメンバー変数（`m_〇〇`）にする。ファイルは今のまま（`stage.h/.cpp` に `Stage` など）。`field.cpp` は触らない。
- **関数の名前**：`〇〇Initialize / Finalize / Update / Draw` → メンバー関数 `Initialize / Finalize / Update / Draw`。`〇〇Get△△`・`〇〇Is△△` などは頭の `〇〇` を取ったメンバー関数（`PlayerGetPosition` → `Player::GetPosition`、`StageGet` → `Stage::GetStage`、`DummyGetTarget` → `DummyManager::GetTarget`、`ArmGetType` → `ArmManager::GetType`、`PlayerUpdateFacing` → `Player::UpdateFacing`）。読むだけの関数は `const`。ファイルの中の `static` 関数（`camera.cpp` の `PlayerHead`・`SnapToPlayer` など、`dummy.cpp` の `PlaceAtSpawn`）は `private` のメンバー関数にする。調整用の値（`static constexpr`）と `DUMMY_COUNT` はそのまま。`dummy.cpp` の `DUMMY` はデータだけの struct として `DummyManager` の中（`private`）に置いてよい。
- **お互いの参照**：今は関数を呼び合っている（プレイヤー ↔ カメラ ↔ ダミー ↔ 腕）ので、`Initialize` で相手の参照を受け取り、ポインタのメンバー（例：`Camera* m_Camera`。読むだけなら `const Camera*`）に持つ。`Initialize` の中では、先に初期化された相手だけを使う（今の初期化の順と同じ）。
  | クラス | `Initialize` で受け取る相手 |
  |---|---|
  | `Stage` | なし |
  | `Player` | `Stage`・`Camera`・`DummyManager`・`ArmManager` |
  | `DummyManager` | `Stage`・`Player`（押されたときに位置を直す）・`Camera` |
  | `Camera` | `Stage`・`Player`・`DummyManager`・`ArmManager`・`Match` |
  | `ArmManager` | `Stage`・`Player`・`DummyManager`・`Camera`（腕を作るときに渡す） |
  | `Match` | `DummyManager`・`ArmManager` |
  | `Hud` | `Camera`・`DummyManager`・`Match` |
- **やり直し**：各クラスに、今の `〇〇Initialize` の中身（状態を最初に戻す）を `Reset()` として持たせ、`Initialize` は参照を持ってから `Reset()` を呼ぶ。R のやり直しは `Match` から `Game` に移す：`Game::Update` で今 `MatchUpdate()` を呼んでいる所を「`ACTION_RESTART` なら `Restart()`（腕 → プレイヤー → ダミー → カメラ → 試合の順に `Reset()`。デバッグ出力も移す）、そうでなければ `m_Match.Update()`」にする（呼ぶ順と動きは今と同じ）。時間切れの `ArmReset` は `Match` が `ArmManager::Reset()` を呼ぶ。
- **腕**（段階 B の作りに足す）：
  - 腕のクラスが使う相手は、データだけの struct `ARM_LINKS`（`Player*`・`DummyManager*`・`const Camera*`・`const Stage*`。メンバーは全部 Doxygen で説明）にまとめ、`Arm` のコンストラクタ `Arm(int hand, const ARM_LINKS& links)` で受け取る。派生クラスは `protected` の `GetPlayer()`・`GetDummies()`・`GetCamera()`・`GetStage()` で使う。作る表は `CreateArm<T>(int hand, const ARM_LINKS& links)` にする。
  - 共通の部品 `ArmGetShoulder`・`ArmGetRestDirection`・`ArmGetColor`・`ArmDrawPiece`・`ArmLoadModel` は `Arm` の `protected` メンバー関数 `GetShoulder()`・`GetRestDirection(pitchOffset)`・`GetColor(active)`・`DrawPiece(…)`・`LoadModel(path)` にする（手は `m_Hand` を使う。値と計算は `arm.cpp` のまま）。`ArmGetName` は状態を使わないので自由関数のまま。
  - `ArmUpdate` などの外への関数は `ArmManager` のメンバー（`Update(bool playing)`・`Draw`・`Reset`・`IsActive`・`IsHolding`・`GetType`）。`g_Types`・`g_Arms` はメンバーに、`ConnectOtherArm` は今のまま。
- **`Game`**（新しい `src/game/game.h/.cpp`）：上の7つをメンバーに持ち、`Initialize` は今の `main.cpp` と同じ順（ステージ → プレイヤー → ダミー → カメラ → 腕 → 試合 → HUD）、`Finalize` はその逆。`Update` は今の `main.cpp` の `StageUpdate()` から `HudUpdate()` まで（`MatchIsPlaying()` での止め方も同じ）、`Draw` は今の `main.cpp` の `SetDepthEnable(true)` から `HudDraw()` まで（ライトの設定も含む）。タイトルバー用に `const ArmManager& GetArms() const`。
- **`main.cpp`**：`static Game g_Game;` を1つだけ持つ。DX11・シェーダー・時間・入力・`Shape`・`Model`・`Number2D` の初期化のあとに `g_Game.Initialize()`、終了は `g_Game.Finalize()` のあとに基盤を逆順。`Update` は時間・入力・`ActionUpdate`・振動の切り替えのあと `g_Game.Update()`、`Draw` は `Shader_Begin`・`Clear` → `g_Game.Draw()` → `Present`。`RefreshTitle` と装備の変化の確認は `g_Game.GetArms().GetType(手)` を使う。クラスのコンストラクタでは DirectX を使わない（`g_Game` は `WinMain` より前に作られる）。
- **なくすもの**：`src/game/` の `g_〇〇`（`field.cpp` 以外）と、`PlayerGetPosition()` のような外から呼ぶ自由関数（`ArmGetName` は残す）。
- 確認：Debug / Release 警告0、`run-tests.ps1`（テストは変えない）。画面：段階 B と同じ（1〜3 と Shift＋1〜3 の全部の腕・触腕でつかんで投げる・グラップリングで飛ぶ・ドリル）＋ 試合の流れ（残り時間が減る・ダミーを場外に出すと +1・R でやり直すと全部最初に戻る）、ダミーの頭の上の %、ロックオン・カメラ（つかんでいる間は外れて、離すと戻る）。

#### 段階 D：決まりの書き直し（Claude）

- `docs/dev/01_DX11基盤の構成.md` の「オブジェクトの足し方」と腕の足し方を、クラスを足す手順に書き直す。`CLAUDE.md`・`AGENTS.md` の古い書き方（`〇〇Initialize` の関数・`ARM_FUNCTIONS`）を直す。`docs/decisions.md` の「作り方」に1行足す。
- No.38 のプランとプロンプトを `MagnetArm` に書き直す（`MAGNET` も `class MagnetCalc` にする）。

### No.44 段階 C を Codex に頼むプロンプト（No.45 のマージのあと）

```text
Proto1 の No.44「構造体のクラス化」の段階 C（ゲームの部品と Game クラス）を実装してください。プランは docs/handoff.md の「No.44 のプラン：構造体のクラス化（管理者確認済み）」の「段階 C」です。プランにないことはしません。

## 始める前に
- CLAUDE.md・AGENTS.md・docs/handoff.md・docs/tasks.md・docs/dev/01_DX11基盤の構成.md と Notion の開発ログ（C++・文字コード・ビルドの行）を読み、Claude が作業中でないことを確かめる（git status）。
- 段階 B と No.45 がマージ済みか確かめてから .\tools\start-work.ps1 class-c

## いちばん大事なこと
- ゲームの動き・調整用の値・計算の結果・Update と Draw で呼ぶ順を変えない。作りを変えるだけ。不具合を見つけても直さずに報告する。
- お互いの参照はプランの表のとおり Initialize で受け取ってポインタのメンバーに持つ。Initialize の中で、まだ初期化していない相手を使わない。クラスのコンストラクタで DirectX を使わない（main.cpp の g_Game は WinMain より前に作られる）。
- 触らないもの：src/core（main.cpp 以外）・src/input・src/render・src/physics・tests・field.cpp・調整用の値。

## やること
1. src/game の Stage・Player・DummyManager・Camera・ArmManager・Match・Hud をクラスにする（ファイルは今のまま）。g_〇〇 はメンバー変数 m_〇〇、ファイルの中の static 関数は private のメンバー関数、外から呼ぶ自由関数はメンバー関数（プランの「関数の名前」）。今の 〇〇Initialize の中身は Reset() にし、Initialize は参照を持ってから Reset() を呼ぶ。
2. 腕：ARM_LINKS を足し、Arm のコンストラクタ・CreateArm・各腕のコンストラクタで受け取る。ArmGetShoulder などの共通の部品は Arm の protected メンバー関数にする（プランの「腕」）。tentacle.cpp・arm_grapple.cpp・arm_drill.cpp は、呼び方を置きかえるだけ。
3. src/game/game.h/.cpp に Game を作り、main.cpp を「基盤の初期化 → g_Game.Initialize()」「入力 → g_Game.Update()」「Shader_Begin・Clear → g_Game.Draw() → Present」にする。R のやり直しは Game::Restart() に移す（プランの「やり直し」）。
4. メンバー変数は private で m_ を頭に付け、クラス・public のメンバー・ARM_LINKS のメンバーは全部 Doxygen の日本語コメント（bool を受け取る関数は「〜を設定する（true：〜、false：〜）」）。新しいファイルは .vcxproj と .vcxproj.filters に登録する（フィルター名は既存の登録を読んでそろえる）。C++ は BOM付きUTF-8・CRLF。
5. src/game に g_ が field.cpp の分しか残っていないこと（Select-String で確かめる）と、Debug のビルドが通ることを確かめ、.\tools\save-work.ps1 "No.44 段階 C クラス化" で保存する。docs/handoff.md の「今の状態」に「No.44 段階 C は Codex が実装。Claude の確認待ち」と、迷ったことを短く書く。

## やらないこと
- run-tests.ps1・Release のビルド・画面の確認・PR・マージ（Claude がやる）。
- 段階 D（docs/dev・CLAUDE.md・AGENTS.md の書き直し）、磁石・ほかの腕の追加、調整用の値の変更、プランにないファイルの整理。

## 報告すること（短く）
- 足した・変えたファイル
- プランと違うことをしたところ、迷ったところ（特に、参照の表にない相手が要ったところ）
```

### No.44 段階 C を Claude が確かめるときのプロンプト（モデル：Opus。Codex の保存のあと、管理者が Claude Code の新しいセッションに貼る）

```text
Proto1 の No.44 段階 C を Codex が feature/class-c に push しました。Claude の担当分（確かめる → PR → マージ → 段階 D）をやってください。
- CLAUDE.md の手順どおり handoff.md・tasks.md と Notion の開発ログを読み、Codex が作業中でないことを確かめてから .\tools\start-work.ps1 class-c。
- プラン（handoff.md の No.44 段階 C）と違うところがないか差分を読む。特に「動き・調整用の値・計算の中身・Update と Draw の呼ぶ順が変わっていない」「Initialize で、まだ初期化していない相手を使っていない」「やり直しの順（腕 → プレイヤー → ダミー → カメラ → 試合）」「src/game に g_ が field.cpp の分しか残っていない」「core / input / render / physics・tests を触っていない」「Doxygen コメントと m_」。
- Debug / Release 警告0、.\tools\run-tests.ps1 と .\tools\run-tests.ps1 -Release が全部通る（件数は No.45 のあとと同じ）。
- 画面を撮る（ゲームが前に出ていることを確かめてから入力を送る。Notion の開発ログ）：プランの段階 C の「確認」のとおり。
- 直すところはこのブランチに直接コミットしてよい。PR を出してマージする（テストが通っていれば聞かずにマージしてよい）。
- そのあと段階 D を自分でやり、tasks.md の No.44 を ✅、Notion の No.44 を「完了」にして PR のリンクを入れる。
- 終わったら handoff.md の「今の状態」「次にやること」を No.38 磁石（プランとプロンプトを新しい作りに書き直す）にし、No.44 のプランとプロンプトを消す。
```

## 3. そのあと：No.38（磁石）の実装

- No.44 のあと、Claude が下のプランとプロンプトを新しい作りに書き直してから、管理者が Codex に貼る（下のプロンプトは今の `ARM_FUNCTIONS` の作りのままなので、**そのまま貼らない**）。
- 順番（管理者が決めた）：No.44 クラス化 → No.38 磁石 → No.39 デカ盾 → No.40 ハサミ（最後）。No.18・No.19 はプランがまだ（アームのあとで管理者が選ぶ）。
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
  - `arm_magnet.obj`（長さ 17.42 cm、大きさ 0.08 → 約 1.4）を共通の `ArmGetShoulder(hand)` から描く。向きは待機中は `ArmGetRestDirection(hand)`（上下 -45°、外へ片側15°）、引き寄せ中・止めている間は肩から敵の中心への水平の向き。`MAGNET_AIM_SECONDS`（0.1 秒）でなめらかに変える（`ArmCalcBlendDirection`。ドリルと同じ作り）。ひねりなし。
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
   - 描画：ArmLoadModel（右手を鏡写し）・ArmDrawPiece。始点は共通の ArmGetShoulder(手)、向きは ArmCalcBlendDirection(ArmGetRestDirection(手), 肩から敵の中心への水平の向き, まっすぐ度)。引き寄せ中・止めている間は ArmGetColor(手, true)。
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
- 終わったら handoff.md の「次にやること」を No.39 デカ盾（プランはまだ。arm_shield_plate.obj はモデル読み込み対策で書き出し直し済み）にし、管理者に No.39 のプランを出す。
```
