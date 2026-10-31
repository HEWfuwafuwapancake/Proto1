# 引き継ぎメモ（今の状態・次にやること）

作業を始めるときに読み、終えるときに書き直す。**ここには「今の状態」と「次にやること」だけを書く。**
タスクの状態は [tasks.md](tasks.md)（Notion の「[Proto1（腕ダフルアームズ）](https://app.notion.com/p/e5e9f1983bbe4758b06b4adba135a491)」と同じ）。
決まったことは [decisions.md](decisions.md)、腕のモデルの原点と長さは [dev/02_腕のモデル.md](dev/02_腕のモデル.md)、終わったタスクの記録は [archive/](archive/)（普段は読まない）。

- 最終更新：2026-10-31（Claude。Shota の Proto1-pad-support worktree、Visual Studio 2022 Community の PC。No.53 段階3 中。そのあと No.52 の段階1）

---

## 1. 今の状態

- **No.53（ほかのコントローラー）：段階3 中。管理者の実機確認待ち**（No.51・52 は別の作業で使われていたので No.53 に付け替えた）。Codex が段階2（XInput・Switch Pro 形式の入力、共有4枠、XInput の振動、Switch の初期化送信、対応外 VID/PID の Debug 出力、テスト）。Claude が段階3：Switch Pro 形式の合図を「最初の入力が届いたとき」から「つないだとき」に直した（合図まで入力を送らない機種があるため）。Debug / Release 警告0、`run-tests.ps1`（Debug・Release）255件成功、新しい計算を壊すと足した4つのテストが落ちることを確認。**未確認**：実機（GameSir の XInput モード・茶色のコントローラー）。0x3F の Y の向きも実機で見る。
- 作業場所は `C:\Users\shota\source\repos\Proto1-pad-support`、ブランチは `feature/pad-support-plan`。元の `Proto1` は別作業の未コミット変更があったため、管理者の許可で worktree を分けた。段階3 もこの worktree で続ける。
- **No.26 オンライン対戦は完了**（PR #90・#92・#93。127.0.0.1 の2つ起動で同期・切断・ビルド拒否を確認済み）。2026-10-31 に Tailscale で離れた PC（鈴木さん）と2人で遊べた（`tailscale ping` は直接で往復約11ms、ゲームの ping はふだん30ms以下、待ち 0f）。
- No.47 の腕の取付と強さは管理者が遊んで調整する。マウス操作は止めたまま。
- **No.48 ゲーム内のモード選択は完了**。起動の引数なしで、GAME START → モード選択（SOLO・LOCAL VS・ONLINE HOST・ONLINE JOIN）から始められる。2つ起動で HOST・JOIN・試合・ロビーの BACK・ホストを閉じると参加側がモード選択へ、を確認済み。**未確認**：パッドでのメニュー操作（パッドがなかった）、LOCAL VS で試合を始めること・結果の TITLE からモード選択へ戻ること（テストだけ）。
- **No.49 delay の自動設定は完了**。4人の対戦で delay 3 では入力待ちで止まりラグかったため、ホストが試合の開始時に ping から決める（`--delay` を付けたら固定）。
- **No.50 完了**：Tailscale の2人対戦で ping 30ms 以下なのに delay 9 になったので、自動の delay に使う ping を「直近2秒の最大」から「人ごとの 90% の値の、いちばん遅い人の値」にし、同じ `Echo` の再送は測らないようにした（`SessionCalc::GetAutoDelayPing`）。2人（Tailscale）では delay 5、4人では delay 7（ping 33・66・33ms）になった。4人で `--delay 4` にすると待ちが合計 500f を超えた（「待ち」は試合が始まってからの合計）。
- **No.51 DESYNC 対策**：4人の Tailscale 対戦でタイトルバーに DESYNC（PC ごとに試合の中身がずれた）が出た。各自が自分でビルドした exe を使っていたこと・sinf などが CPU で FMA3 の計算に切り替わることが原因と考え、起動時に `_set_FMA3_enable(0)`、exe の中身から作った識別番号を接続時に比べて違う exe は断る（`SESSION_FAILURE_PROGRAM`、通信の版 3）、DESYNC をタイトルバーの先頭に出す、を入れた。テスト・127.0.0.1 の2つ起動だけで確かめた。**未確認**：ホストの Release の exe を全員に配って4人で遊び、DESYNC が出ないか（出たら、ずれた値を記録して比べる仕組みを足す）。

## 2. 次にやること：No.53 ほかのコントローラーへの対応（段階3 テスト）

管理者の依頼（2026-10-31）。プランは管理者確認済み（2026-10-31）。**Switch 2 Pro コンはいったん保留**（USB でも起動の合図を bulk の口に送る必要があり、WinUSB ドライバーの入れ替えが要るため）。
対象は次の2つ。今の `src/input/pad.cpp` はソニーの VID（0x054C）だけを Raw Input の HID で読み、ほかの機器は無視している。作業ブランチは `feature/pad-support-plan`（プランを push 済み）。A と B をまとめて1つの PR にする。

| コントローラー | PC へのつなぎ方と見え方 | 読み方 |
|---|---|---|
| GameSir Super Nova | 2.4G の受信機か有線で **XInput モード**（－と＋を長押しで切り替え。Bluetooth では XInput にならない） | A：XInput |
| 茶色のコントローラー（Switch 用・ジャイロ・連射つき。型番は分からない） | この種類はふつう **Switch の Pro コン（VID 057E・PID 2009）のふり**をする（Bluetooth・有線） | B：Switch Pro コン形式（GameSir を Switch モードにしたときもこれ） |

**全体**
- 番号は今の4つ（`PAD_COUNT`）を、DS4・DualSense・XInput・Switch Pro コンで共有し、つないだ順に空いている番号へ入れる（今と同じ。外れた番号は空ける）。`action.cpp` と `src/game` は変えない（`PAD_BUTTON` で読んでいるので）。
- ボタンは**位置で合わせる**（管理者確認済み）：下→×、右→○、左→□、上→△。つまり Xbox 配置は A→×・B→○・X→□・Y→△、Switch 配置は B→×・A→○・Y→□・X→△。どのパッドでも下のボタンでジャンプ、右のボタンで決定。
- `PAD_MODEL` に `PAD_MODEL_XINPUT`・`PAD_MODEL_SWITCH_PRO` を足す。`pad.h` / `padcalc.h` の `@file` の説明、`docs/dev/01_DX11基盤の構成.md` と `docs/plan/01_企画の要点.md` の対応パッドの記述も直す。
- Debug のとき、対応外のゲームパッド（Raw Input の usage 0x01/0x05）を見つけたら、VID と PID を1回だけ Debug 出力に出す（茶色のものが 057E:2009 でなかったときに調べるため）。

**A：XInput（Xbox 形式）**
- `pad.cpp` に XInput を足す（`<Xinput.h>`、`#pragma comment(lib, "XInput.lib")`。Windows 標準で外部ライブラリなし）。`PAD_DEVICE` に XInput の番号（0〜3、使わないときは -1）を持たせる。
- `PadUpdate` で、割り当て済みの XInput は毎フレーム `XInputGetState`。外れたら（`ERROR_DEVICE_NOT_CONNECTED`）番号を空ける。まだ割り当てていない XInput の番号は、未接続を毎フレーム調べると重いので `XINPUT_POLL_INTERVAL = 1.0f` 秒ごとに調べる（「調整用の値」）。ウィンドウが選ばれていないときは今と同じく「何も押していない」。
- 変換は `padcalc` に Windows を使わない関数 `PadCalcParseXInput(unsigned int buttons, unsigned char leftTrigger, unsigned char rightTrigger, short lx, short ly, short rx, short ry, PAD_STATE& state)` を作る（XInput のビットの値は padcalc.cpp に自前の定数で書く）。スティックは -32768〜32767 → -1〜1（上がプラスのまま）。LB/RB→L1/R1、LT/RT→L2/R2（`PAD_TRIGGER_THRESHOLD`。0〜255 のうちこれを超えたら押した扱い。DualSense と同じ考え）、View(Back)→SHARE、Menu(Start)→OPTIONS、スティック押し込み→L3/R3、十字→十字。Guide は XInput では読めないので PS はなし。
- 振動は `XInputSetState`（左モーター＝大きい、右モーター＝小さい、0〜65535）。今の `SendOutput` と同じく、値が変わったときだけ送り、非選択中・外れたときは 0 にする。ライトバーはないので何もしない。
- XInput のパッドは Raw Input の HID にも出るが、VID がソニーでないので今の `AddDevice` では読まれない（二重にならない）。

**B：Switch の Pro コン形式（057E:2009）を Raw Input の HID で読む**
- `PadCalcIdentifyModel` に 057E:2009 → `PAD_MODEL_SWITCH_PRO` を足す。
- つないで最初の入力が届いたら、出力用のハンドルを開き（今の `OpenOutputDevice` と同じ。長さは HID の `OutputReportByteLength`）、次を送る（作るのは padcalc の関数にしてテストする）。
  1. USB 用の合図：`0x80 0x02`（ハンドシェイク）→ `0x80 0x04`（USB の HID だけで話す）。Bluetooth では失敗するが、無視してよい（警告も出さない）。
  2. 入力の形を「フル」にするサブコマンド：`[0]=0x01 [1]=カウンター(0〜15) [2..9]=振動なし（00 01 40 40 00 01 40 40） [10]=0x03 [11]=0x30`。これで USB でも Bluetooth でも 0x30 のレポートが来る。
  失敗しても入力の読み取りは続ける（安物は返事をしないこともある）。
- 入力は2種類読む（どちらも padcalc にしてテストする）：
  - **0x30（フル）**：[3] Y(0x01) X(0x02) B(0x04) A(0x08) R(0x40) ZR(0x80)、[4] －(0x01) ＋(0x02) Rスティック押し(0x04) Lスティック押し(0x08) HOME(0x10)、[5] 下(0x01) 上(0x02) 右(0x04) 左(0x08) L(0x40) ZL(0x80)。スティックは12ビット：左 X=[6]|([7]&0x0F)<<8、Y=([7]>>4)|[8]<<4、右は [9]〜[11] で同じ。中心 2048、上がプラス。`SWITCH_STICK_RANGE = 1600`（中心からこれだけ倒したら 1。超えたら 1 に切る）。
  - **0x3F（簡易。Bluetooth で合図が効かなかったとき）**：[1] B(0x01) A(0x02) Y(0x04) X(0x08) L(0x10) R(0x20) ZL(0x40) ZR(0x80)、[2] －(0x01) ＋(0x02) Lスティック押し(0x04) Rスティック押し(0x08) HOME(0x10)、[3] 方向（0=上から時計回りに 0〜7、8=中立）、[4]〜[11] 左X・左Y・右X・右Y（16ビット、リトルエンディアン、中心 0x8000）。**Y の向きは実機で確かめる**（下がプラスなら逆にする）。
- 割り当て：L→L1、R→R1、ZL→L2、ZR→R2、－→SHARE、＋→OPTIONS、HOME→PS、スティック押し→L3/R3。
- 振動（HD 振動の形式が難しい）・ジャイロ・プレイヤーランプは作らない。

**テスト**（`tests/Proto1Tests/padcalc_test.cpp` に足す。主な動きだけ）：XInput の A・B・LT と左スティックの上、Switch 0x30 の B・A・ZL・左スティックの上、0x3F の B・A と方向、`PadCalcIdentifyModel` の 057E:2009、サブコマンドのバイト列。

**確かめ方（段階3）**：Debug / Release 警告 0、`run-tests.ps1`（Debug・Release）、新しい計算をまとめて壊してテストが落ちること。見た目は変わらないので画面撮りはしない。実機は管理者に頼む：GameSir（受信機・有線、XInput モード）と茶色のコントローラー（Bluetooth・有線）で、移動・カメラ・ジャンプ・腕（L1/R1）・メニュー（十字・×・○）・振動（GameSir だけ）。DS4・DualSense と混ぜても番号がずれないこと、抜き差しで番号が空くこと。茶色のものが動かなければ、Debug 出力の VID/PID を見る。

**段階2 のプロンプト（Codex）**
> CLAUDE.md・AGENTS.md の手順でセッションを始め、`feature/pad-support-plan` で No.53 の段階2（コーディング）をしてください。プランは `docs/handoff.md` の「次にやること」です。AGENTS.md の「コードを書くだけ」のとおり、Debug のビルドが通るところまで確かめて `save-work.ps1` で保存し、「今の状態」を「段階2 済み。段階3 待ち」にしてください。プランと違うことをしたくなったら、書く前に聞いてください。

**段階3 のプロンプト**
> CLAUDE.md・AGENTS.md の手順でセッションを始め、`feature/pad-support-plan` で No.53 の段階3（テスト）をしてください。プランは `docs/handoff.md` の「次にやること」です。AGENTS.md の段階3 の順で確かめてください。実機での確認は管理者に頼み、結果を「今の状態」に書いてください。

### そのあと：No.52 の段階1（プラン。オンラインの delay を減らす）

`docs/tasks.md` の No.52 のプランを書き、管理者の確認を待つ。調べて分かっていること：
- ゲームの ping が回線（`tailscale ping`：49・8・20ms）より 15〜25ms 大きい。受け取りが 1/60 秒に1回（`NetSession::Update`）なので、届いたパケットが次のフレームまで待つ（ホストを通ると2回）。
- 案：`main.cpp` のフレームのすき間（`Sleep(1)` の所）でも受け取り、ホストはすぐ配り直す（入力の採取は今のまま 1/60 秒に1回）。ほかに、FPS とは別に「1秒に何回ゲームを進めたか」をタイトルバーに出す（FPS は通信待ちでも描画の回数で 60 と出る）。
- それでも足りなければ、参加者どうしが直接送る・ロールバック。No.19 はそのあと。

---

- 順番（管理者が決めた）：No.53 → No.52 → No.19。No.40 ハサミは最後。No.24・No.27 が未完了なので実装へ進まない。
- 画面を撮る方法（段階3）：画面操作のツールがなくても、PowerShell で撮れる。ゲームを `Start-Process -WorkingDirectory <リポジトリ>` で起動し、user32 の `SetForegroundWindow`（`Alt` を押したまま呼ぶ。下を参照）・`keybd_event`・`mouse_event`、`System.Drawing` の `CopyFromScreen` でウィンドウを撮る。キーや クリックを送る前に毎回 `GetForegroundWindow()` がゲームか確かめ、違えば止める。日本語を書いた .ps1 は BOM付きUTF-8。撮った画像は結果を書く前に必ず見る。Alt を押して離すとゲームのシステムメニューが開いて Enter を吸うので、Alt を押したまま前に出し、F13 を1回押してから Alt を離す。ほかのウィンドウが上に出ることがあるので、撮る直前にもう一度前に出す。Debug 出力は DBWIN_BUFFER を読む一時スクリプトで取れる。
- 画面の確認のヒント：カメラは敵を見続けるので、腕を横から見たいときは `camera.cpp` の `Camera::MakeMatrices` でビューを置きかえた一時ビルドを、OutDir・IntDir を scratchpad に分けて作る（コミットしない。終わったら `git checkout` で戻す）。**ビューの見ている点を肩（`ArmCalcShoulder` に `Player` の位置・向きを渡した点）にすると、肩が画面の真ん中に来て、部品が肩についているか分かる**。真横（キャラの左へ 2.5、前へ 0.6、肩と同じ高さ）から見ると、部品が上下どちらに曲がっているか分かる。

### 新しいアームの仕様（管理者確認済み、2026-10-26。各腕の細かい値は、その腕のプランで決める）

- **全部の腕で同じ**：操作は今と同じ L1 / R1（左右クリック）だけ。腕ごとに「押したとき」と「効いている間にもう一度押したとき」を決める（「操作はシンプルに」）。
  装備は試すための切り替えだけ（キーボードの Q / E で左手・右手の切り替えモード → 数字キー 1〜6。2026-10-29 にパッドでの切り替えはなくした）。腕カプセルは作らない。色は今の左右の色と、効いている間の色だけ。
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

