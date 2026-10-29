# 引き継ぎメモ（今の状態・次にやること）

作業を始めるときに読み、終えるときに書き直す。**ここには「今の状態」と「次にやること」だけを書く。**
タスクの状態は [tasks.md](tasks.md)（Notion の「[Proto1（腕ダフルアームズ）](https://app.notion.com/p/e5e9f1983bbe4758b06b4adba135a491)」と同じ）。
決まったことは [decisions.md](decisions.md)、腕のモデルの原点と長さは [dev/02_腕のモデル.md](dev/02_腕のモデル.md)、終わったタスクの記録は [archive/](archive/)（普段は読まない）。

- 最終更新：2026-10-29（Claude。Shota の Proto1、Visual Studio Community の PC。No.18 の段階1 プランを書いた。段階2 は Codex、段階3 は別セッションの Claude）

---

## 1. 今の状態

- **No.18（シーン：タイトル → 試合 → 結果）は段階1 済み**（プランは下。管理者が 2026-10-29 に 3 つの選択を決めた）。作業ブランチは `feature/scene-flow`。**段階2（Codex のコーディング）待ち**。
- 今回は **段階2 を Codex、段階3（確かめる・PR・マージ・記録）を別セッションの Claude** が行う（管理者の指定）。
- 管理者が遊んで決めること：全種類の構えが普段の画面で分かるか／グラップリング・ドリル・磁石の取付位置と強さ（No.19）／実物のパッドでの腕切り替え。マウス操作は止めたまま（[decisions.md](decisions.md)）。

## 2. 次にやること：No.18 の段階2（Codex がプランどおりにコードを書く）→ 段階3（Claude が確かめる）

### No.18 のプラン（管理者確認済み、2026-10-29）

**流れ**：起動 → タイトル（START を決定）→ 試合 → 時間切れ（今の TIME UP の画面）→（決定）→ 結果（RETRY か TITLE を選んで決定）→ もう一回 ／ タイトルへ。
試合中の R / OPTIONS のやり直しは今までどおり「いつでも」。

| 管理者が決めたこと | 内容 |
|---|---|
| 時間切れ → 結果 | **ボタンで移る**（TIME UP の画面で決定）。自動では移らない |
| 結果画面 | **もう一回とタイトル**の2つを**選ぶメニュー** |
| タイトル画面 | **文字と操作案内だけ**＋ START の1項目のメニュー。後ろは止まった浮島（初期配置）を描き、半透明の黒でおおう |
| メニューの操作（2026-10-29 追加） | **パッドだけで操作できる**：左スティックの上下で選び、**○で決定**。今どれを選んでいるか分かるように、選んでいる項目を目立たせる（ホバー）。キーボードは W / S で選び Enter で決定 |

**入力**（`src/input/action.h` / `action.cpp`）
- `ACTION_DECIDE`（決定：Enter / ○）を足す。`ACTION_PAD_BUTTONS` の大きさが `ACTION_TARGET_LEFT` なので、**`ACTION_TARGET_LEFT` より前**（`ACTION_EQUIP_RIGHT_NEXT` のあと）に入れ、`ACTION_KEYS`・`ACTION_PAD_BUTTONS` の並びもそろえる。
- ○ は試合中の `ACTION_EQUIP_RIGHT_NEXT`（右手の腕の切り替え）と同じボタン。タイトル・結果では腕を更新しないのでぶつからない。
  **TIME UP の画面では腕の切り替えを止める**（今は「装備切り替えは時間切れのあとも受け付け」だが、○で結果へ進むときに右手の腕まで変わらないように。`m_Arms.Update` は試合中だけ呼ぶ）。
- OPTIONS を決定にしないのは、TIME UP の画面で「やり直し（いつでも）」と「結果へ」が同じボタンになってしまうため。
- メニューの上下は、今の `ActionGetMove().Y`（左スティック・W / S。前がプラス）を使う。新しい行動は足さない。

**シーンの計算**（新しいファイル `src/physics/scenecalc.h` / `.cpp`。DirectX なし。`matchcalc` と同じ置き方）
- `enum SCENE { SCENE_TITLE, SCENE_MATCH, SCENE_RESULT }`
- `enum SCENE_EVENT { SCENE_EVENT_NONE, SCENE_EVENT_START_MATCH, SCENE_EVENT_SHOW_RESULT, SCENE_EVENT_BACK_TITLE }`
- `enum RESULT_ITEM { RESULT_ITEM_RETRY, RESULT_ITEM_TITLE, RESULT_ITEM_COUNT }`（結果のメニューの項目。タイトルのメニューは START だけなので enum は作らない）
- `struct SCENE_INPUT { bool Decide; bool Restart; float MoveY; }`（Decide・Restart は押した瞬間か、MoveY は `ActionGetMove().Y`。メンバーは全部 Doxygen で説明）
- `class SceneCalc`：`void Start()`（タイトルにする）／`SCENE_EVENT Step(const SCENE_INPUT& input, bool matchFinished)`／`SCENE GetScene() const`／`int GetCursor() const`（結果で選んでいる項目）
  - **スティックの弾き**：`MoveY` が `MENU_STICK_THRESHOLD`（0.6）を超えた瞬間に1つ動かし、`MENU_STICK_DEADZONE`（0.3）より内側に戻るまで次を受け付けない（倒しっぱなしで流れない。今の `TARGET_PAD_DEADZONE / THRESHOLD` と同じ考え方）。上（プラス）で前の項目、下で次の項目。端で止める（ループしない）。値は `scenecalc.cpp` の「調整用の値」。
  - タイトル：Decide → 試合（START_MATCH）
  - 試合：Restart → START_MATCH（いつでも。試合中のまま）／ matchFinished かつ Decide → 結果（SHOW_RESULT）。時間切れ前の Decide は何もしない
  - 結果に入るとき、カーソルは RETRY。結果：Decide → カーソルが RETRY なら START_MATCH、TITLE なら BACK_TITLE ／ Restart（R / OPTIONS）→ START_MATCH（近道として残す）
  - スティックが倒れたまま結果に入っても、一度戻すまで動かない（入った瞬間に勝手に動かない）
- テスト `tests/Proto1Tests/scenecalc_test.cpp`（主な動きだけ、5件くらい）：タイトル→決定で試合／試合中の決定は無視・時間切れ後の決定で結果／試合中のやり直し／結果で下に弾いて TITLE→決定でタイトル（RETRY のまま決定でもう一回）／倒しっぱなしでは1つしか動かない。

**Game**（`src/game/game.h` / `game.cpp`）
- `SceneCalc m_Scene` を持つ。`Initialize` の最後に `m_Scene.Start()`（起動はタイトル）。
- `Update`：まず `SCENE_INPUT` を `ActionIsTrigger(ACTION_DECIDE / ACTION_RESTART)` と `ActionGetMove().Y` で作り、
  - **試合のシーン**のときだけ今の `Update` の中身（ステージ・プレイヤー・ダミー・カメラ・腕・試合）を動かす。腕は `m_Match.IsPlaying()` のときだけ `m_Arms.Update(true)`。今の `if (ActionIsTrigger(ACTION_RESTART)) Restart(); else m_Match.Update();` は、`m_Scene.Step` の結果で置きかえる（START_MATCH なら `Restart()`、それ以外は `m_Match.Update()`）。
  - タイトル・結果では、プレイヤー・ダミー・カメラ・腕・試合を更新しない（止まった絵）。`m_Hud.Update()` は毎フレーム呼ぶ。
  - イベントの処理：START_MATCH と BACK_TITLE は `Restart()`（タイトルの後ろも初期配置にする）、SHOW_RESULT は何もしない（結果は Match の得点を読むだけ）。
  - Debug 出力：シーンが変わったら `hal::dout` に「シーン：タイトル／試合／結果」。`Restart()` の「試合：やり直し」はそのまま。
- `Draw` は今のまま（3D はどのシーンでも描く）。
- 時間切れのパッドのライト（赤）は、結果からもう一回・タイトルへのときの `Restart()` → `Match::Reset` で戻るので、追加は要らない。

**画面表示**（`src/game/hud.h` / `hud.cpp`）
- `Hud::Initialize` に `const SceneCalc& scene` を足して受け取る（`Game::Initialize` から `m_Scene` を渡す）。
- **選んでいる項目（ホバー）**：項目の文字の後ろに、文字より少し大きい四角（`MENU_HOVER_COLOR`。半透明の白など）を `Number2DDrawBox` で描き、文字の色を `MENU_SELECTED_COLOR`（黄色など）にする。選んでいない項目は `MENU_TEXT_COLOR`（少し暗い白）。タイトルの START は1つだけだが、同じ見た目で選ばれている状態に描く。
- タイトル：おおい（今の `COVER_COLOR`）＋ 大きな「PROTO」、その下にメニュー「START」（選ばれている見た目）、下に小さく操作案内：「SELECT  STICK / W S」「OK  O / ENTER」と、遊び方の3行「MOVE  WASD / L STICK」「ARMS  L1 R1 / CLICK」「JUMP  SPACE / X」。残り秒数・スコア・頭の上の % は出さない。
- 試合：今のまま。ただし TIME UP の画面の案内を「PRESS R / OPTIONS」から **「NEXT  O / ENTER」** に変える（R / OPTIONS のやり直しは今も効くが、案内は結果へ進むほうだけ）。
- 結果：おおい ＋「RESULT」・大きなスコア（今の `FINAL_SCORE_*` を使う）・縦に並べたメニュー「RETRY」「TITLE」（`GetCursor()` の項目をホバーの見た目に）・下に小さく「SELECT  STICK / W S」「OK  O / ENTER」。残り秒数・頭の上の % は出さない。
- 置き場所・大きさ・色は `hud.cpp` の「調整用の値」に足す（タイトル・結果の各行の Y、ドットの大きさ、メニューの行の間隔、ホバーの四角の余白と色）。
- ○ は文字の「O」で表す（○の形の文字は作らない）。

**文字**（`src/render/glyphcalc.cpp`）
- 足りない英字を 5×7 ドットで足す：**C・D・J・K・L・V・W・X・Y と数字の 1**（上の文字列に出てくるもの。英字の中の「L1 R1」の 1 は `I` で代用せず、ドットの 1 を足す）。足したあと、上の文字列の全文字が表にあるか確かめる。
  `glyphcalc.h` の `GlyphCalcLetterRow` のコメントの文字の一覧も直す。英字のテストは足さない（今のテストが通ればよい）。

**ドキュメント**
- `docs/decisions.md` に1行：「シーンの流れ（2026-10-29、No.18）：起動はタイトル／タイトル・結果はメニュー（左スティックか W・S で選び、○か Enter で決定。選んでいる項目は目立たせる）／TIME UP から結果へは○・Enter（TIME UP の間は腕の切り替えを止める）／結果は RETRY と TITLE（R・OPTIONS でももう一回）／試合中のやり直しはいつでも」。「試合の流れ」の行の「カウントダウンなし（シーンの流れは No.18）」はそのまま。
- `docs/dev/01_DX11基盤の構成.md` の `Game::Update` の順番の説明があれば、シーンで分けたことを足す。
- `docs/plan/01_企画の要点.md` の操作の表に、メニューの操作（左スティック・○ / W・S・Enter）を足す（表があれば）。

**触るファイル**：`src/input/action.h`・`action.cpp`／`src/physics/scenecalc.h`・`.cpp`（新規）／`src/game/game.h`・`game.cpp`／`src/game/hud.h`・`hud.cpp`／`src/render/glyphcalc.h`・`.cpp`／`tests/Proto1Tests/scenecalc_test.cpp`（新規）／`Proto1.vcxproj`・`.filters`／`tests/Proto1Tests/Proto1Tests.vcxproj`・`.filters`（`scenecalc.cpp` を「既存の項目」で）／上のドキュメント。

**確かめること（段階3）**：Debug / Release 警告0／`run-tests.ps1`（Debug・Release）／`scenecalc.cpp` の遷移とスティックの弾きをまとめて壊してテストが落ちる→戻す／画面（タイトル・試合・TIME UP・結果で RETRY 選択・結果で TITLE 選択の5枚）で文字が欠けていないか、ホバーが分かるか、タイトル→試合→TIME UP→結果→もう一回→…→結果→タイトル が回るか。

---

- 順番（管理者が決めた）：No.18 のあとは No.19。No.40 ハサミは最後。No.24・No.27 が未完了なので実装へ進まない。
- 画面を撮る方法（段階3）：画面操作のツールがなくても、PowerShell で撮れる。ゲームを `Start-Process -WorkingDirectory <リポジトリ>` で起動し、user32 の `SetForegroundWindow`（前に `Alt` を一瞬押す）・`keybd_event`・`mouse_event`、`System.Drawing` の `CopyFromScreen` でウィンドウを撮る。キーや クリックを送る前に毎回 `GetForegroundWindow()` がゲームか確かめ、違えば止める。日本語を書いた .ps1 は BOM付きUTF-8。撮った画像は結果を書く前に必ず見る。
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

