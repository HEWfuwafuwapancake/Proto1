# 引き継ぎメモ（今どこまで進んだか・次に何をするか）

別の PC（メインPC・ノートPC）や、別の日の Claude Code・Codex で作業を続けるためのメモです（Claude と Codex のどちらでも続きから始められる）。
**作業を始めるときに読み、作業を終えるときに更新します。** タスクの一覧は [tasks.md](tasks.md)、同じ内容を Notion の
「[Proto1（腕ダフルアームズ）](https://app.notion.com/p/e5e9f1983bbe4758b06b4adba135a491)」データベースでも管理しています。

- 最終更新：2026-10-23（Visual Studio Enterprise の PC の Claude Code。No.14 まで・No.15 のプラン承認済み）

---

## 1. 今の状態

タスクごとの状態は [tasks.md](tasks.md) を見る（No.1〜14 は完了。No.6 だけ実物のパッドでの確認が残っている）。

- ゲーム：浮島の上でプレイヤーが歩く・ジャンプ・触腕（左右クリック／L1・R1）でダミー3体をつかんで投げる。% が高いほど遠くへ飛び、場外のダミーは 2 秒後に 0% で戻る。カメラは TPS。
  試合は 90 秒。場外に落とすと +1、時間切れでプレイヤー・ダミー・触腕が止まり、OPTIONS / R でいつでもやり直せる。残り時間とスコアはタイトルバーに出る（仮。No.15 で画面に出す）。
- 単体テストは 111 個すべて通る。Debug / Release とも警告 0。
- 人が操作して見る確認が残っているもの：
  - No.6：DUALSHOCK 4 を USB でつなぎ、×・L1・R1・□・OPTIONS で出力ウィンドウに「行動：〇〇」が出る・スティックで歩く／回る・抜いても落ちない。確かめたら `tasks.md`・Notion を完了にし、`action.cpp` の「行動：〇〇」の出力を消す
  - 触腕の左右の連続操作・場外からの復活・吹っ飛びの数字（仮。No.19 で調整）
- **メインPC でまだやっていないこと**：Claude に許可を足す設定（下の「3. 決まったこと」の「Claude の許可」）。

## 2. 次にやること：No.15 画面表示（プラン承認済み。実装する）

2026-10-23 に管理者が承認した（決めることはすべておすすめの案）。作業名は `hud-display`（`.\tools\start-work.ps1 hud-display`）。

### 決まったこと
- 残り時間は**秒だけ**（`90`→`0`）。分:秒にはしない
- ダミーの % は**各ダミーの頭の上**に出す（3体とも同じ色なので、画面の下に並べるとどれか分からない）
- 「時間切れ」と「R で再開」は、**英字を 5×7 のドット**で出す（使う文字だけ。`TIME UP`・`PRESS R / OPTIONS`）。フォントは読み込まない
- 見本の `polygon2D`（左上の半透明の黒い四角）は**消す**（ファイルも）。新しい 2D の部品が見本の代わりになる
- 色は白（時間切れのおおいだけ半透明の黒）。残り時間で色を変えるなどの演出は入れない

### 画面
- 上の真ん中：残り秒数（大きい 7 セグ）／左上：スコア（7 セグ）
- 各ダミーの頭の上：`12%`（数字は 7 セグ、`%` はドット）。復活待ちのときと、カメラの後ろにいるときは出さない
- 時間切れ：画面全体に半透明の黒 → 真ん中に `TIME UP`、その下に大きくスコア、いちばん下に `PRESS R / OPTIONS`。上の秒数は `0` のまま出す

### ファイルごとの作業
1. **新規 `src/render/glyphcalc.h / .cpp`**（DirectX なし・単体テストあり。数字と文字を「点ける四角の一覧」に変える）
   ```cpp
   struct RECT2D { float X, Y, Width, Height; };   // 画面の四角（左上が原点、ピクセル）
   enum GLYPH_ALIGN { GLYPH_ALIGN_LEFT, GLYPH_ALIGN_CENTER, GLYPH_ALIGN_RIGHT };
   int  GlyphCalcSplitDigits(int value, int minDigits, int digits[], int maxDigits); // 桁に分ける。負は 0、入りきらなければ全部 9。桁数を返す
   unsigned char GlyphCalcSegments(int digit);      // 0〜9 → 点ける 7 セグ（bit0=a 上, b 右上, c 右下, d 下, e 左下, f 左上, bit6=g 真ん中）。範囲外は 0
   RECT2D GlyphCalcSegmentRect(int segment, float x, float y, float digitWidth);   // 1桁の左上 (x,y) から見た1本の棒の四角
   int  GlyphCalcNumberRects(int value, int minDigits, float x, float y, float digitWidth,
                             GLYPH_ALIGN align, RECT2D out[], int maxRects);       // 数字全体の四角。数を返す（maxRects を超えて書かない）
   unsigned char GlyphCalcLetterRow(char c, int row);  // 5×7 ドットの1行（下位5ビット、bit4 が左端）。A E I M N O P R S T U % / と空白。それ以外は空白
   int  GlyphCalcTextRects(const char* text, float x, float y, float dotSize,
                           GLYPH_ALIGN align, RECT2D out[], int maxRects);         // 文字列の四角。数を返す
   int  GlyphCalcDisplayPercent(float percent);        // 表示する %（四捨五入、0〜999）
   ```
   - 1桁は「幅 W・高さ 2W・棒の太さ 0.2W・桁の間 0.3W」。比は「調整用の値」。文字は 5×7 ドットで、文字の間は1ドット。`x` は align に応じて左端・真ん中・右端
2. **新規 `src/render/number2d.h / .cpp`**（描画。`polygon2D.cpp` が見本）
   - `Number2DInitialize / Finalize`：動的な頂点バッファ（四角 512 個 × 6 頂点の三角形リスト）
   - `Number2DDrawRects(const RECT2D rects[], int count, const XMFLOAT4& color)`：Map（DISCARD）→ 頂点を書く → `Draw` 1回
   - `Number2DDrawNumber(int value, int minDigits, float x, float y, float digitWidth, GLYPH_ALIGN align, const XMFLOAT4& color)`
   - `Number2DDrawText(const char* text, float x, float y, float dotSize, GLYPH_ALIGN align, const XMFLOAT4& color)`
   - `Number2DDrawBox(const RECT2D& rect, const XMFLOAT4& color)`（時間切れのおおい用）
   - 描く前に `Shader_Set2DMatrix()`、マテリアルは白、色は頂点カラー。三角形は時計回り（カリングがある）
3. **`src/game/camera.h / .cpp`**：`bool CameraWorldToScreen(const XMFLOAT3& world, XMFLOAT2* screen)` を足す（画面のピクセル位置。カメラの後ろ（w ≤ 0）なら false）。
   ビュー・プロジェクション行列を作る処理は `CameraDraw` と共通の static 関数にまとめる
4. **`src/game/dummy.h / .cpp`**：`float DummyGetPercent(int index)` を足す（無効な番号は 0）。
   頭の位置は今ある `DummyGetTarget(i).Body` から（`max(Start.y, End.y) + Radius`）、見えるかは `DummyIsActive`。この2つは足さない
5. **新規 `src/game/hud.h / .cpp`**：`HudInitialize / Finalize / Update / Draw`（Update は空）。置き場所・大きさ・色は「調整用の値」
   （例：秒の桁の幅 40、スコア 28、% 14、頭の上 0.4、おおい `{0,0,0,0.6}`）。値は `MatchGetRemainingSeconds()`・`MatchGetScore()`・`MatchGetState()`・`DummyGetPercent()`
6. **`src/core/main.cpp`**：`Polygon2D〇〇` を `Number2DInitialize` → `HudInitialize`（終了は逆の順）と `HudDraw` に置き換える。
   `RefreshTitle` は FPS だけに戻す（残り時間・スコア・R で再開を消す。要らなくなった include も消す）
7. **削除 `src/render/polygon2D.h / .cpp`**
8. **プロジェクト**：`Proto1.vcxproj` / `.filters` に足す・polygon2D を消す（フィルター名は `src\render`・`src\game`）。
   `tests/Proto1Tests/Proto1Tests.vcxproj` に `..\..\src\render\glyphcalc.cpp` と `glyphcalc_test.cpp`
9. **ドキュメント**：`docs/dev/01_DX11基盤の構成.md` のファイルの役割の表（polygon2D の行を差し替え）・「描画の順番」・「試合の流れ」の最後の行（タイトルバーの話）を直し、
   「画面表示（No.15）」の節を最後に足す

### テスト（`tests/Proto1Tests/glyphcalc_test.cpp`）
- 桁：`0`→[0]、`90`→[9,0]、`7` を2桁→[0,7]、負→[0]、`1234` を3桁まで→[9,9,9]
- 7 セグ：`8`=7本、`1`=b・c、`7`=a・b・c、`0` は g が消えている、範囲外は 0
- 棒の位置：a は一番上で横長、g は縦の真ん中、d の下の端は 2W
- 数字の四角：`90` は 12 個。右寄せは一番右の四角の右端が x、中央寄せは全体の幅の半分だけ左。maxRects を超えない
- 文字：`T` の1行目は 0b11111、知らない文字は 0、`"TIME UP"` の四角の数はドットの数の合計と同じ
- %：12.4→12、12.5→13、-1→0、5000→999
- わざと壊す確認は `glyphcalc.cpp` だけを1回で（`8` のマスク・四捨五入・右寄せのずれを壊す → 失敗を確かめて戻す）

### 画面を撮る確認（3場面）
1. 始めたところ：上に `90`、左上に `0`、ダミー3体の頭に `0%`
2. つかんで投げたあと：投げたダミーの % が増え、1体を場外に落としてスコア `1`、秒数が減っている
3. 時間切れ：90 秒待つ（制限時間を一時的に短くはしない。間違えてコミットしないため）。おおい・`TIME UP`・スコア・`PRESS R / OPTIONS`。R で `90` に戻る

No.14 の記録は `docs/dev/01_DX11基盤の構成.md` の「試合の流れ」。

## 3. これまでに決まったこと（会話の中で決めたこと）

- **共有フォルダ**：Claude と Codex は同じ `Proto1` を交代で使う。同時には作業しない。切り替える前にコミット・push と引き継ぎメモ更新を行う（2026-10-23、管理者の指示）。
- **AI の役割**：Notion の委譲先・レビュー担当は当面無視し、担当欄は変更しない。実装とレビューの分担は管理者が指定したときに適用する。
- **進め方**：`tasks.md` の優先度：高から順に、1つずつ「プラン → 管理者の確認 → 実装 → テスト → 完了チェック」。1タスク＝1フィーチャーブランチ＝1 PR。
- **マージ**：Claude・Codex がこの流れで作った PR は、テストとビルドが通れば、管理者に聞かずにその AI が `gh pr merge <番号> --merge --admin --delete-branch` でマージしてよい（2026-10-23、管理者が「基本マージして問題ない」と決めた。Codex も同じと 2026-10-23 に決めた）。他の人の PR はマージしない。
- **TPS カメラ**：プレイヤーが歩いても、カメラは自動で後ろに回り込まない（向きはマウス・右スティックで自分で変える。触腕をカメラの向きへ伸ばすので、勝手に回ると狙いにくい。2026-10-23、No.9 で決めた）。
- **試合の流れ**（2026-10-23、No.14 で決めた）：時間切れのあとはプレイヤー・ダミー・触腕が止まる（カメラは回せる）／やり直しはいつでも／プレイヤーが落ちても減点なし／カウントダウンなし（シーンの流れは No.18）／No.15 までの表示はタイトルバー。
- **ダミー**：3体・その場に立つだけ。プレイヤーがぶつかると押せる（押して落とせば「場外 → 復活」を試せる。2026-10-23、No.11 で決めた）。場外に落ちたら 2 秒後に最初の場所に戻る。
- **ゲームに関係ないサンプルは消す**：回る立方体（`cube`）は No.8 で消した（2026-10-23、管理者の指示）。地形（`field.cpp`）は No.7 で使うのをやめたが、ファイルは残してある。
- **Claude の許可**：Claude Code の自動モードの安全チェックが、マージや `start-work.ps1`（中で develop を `git merge` する）、Notion の更新を止めることがある。
  そのため、管理者の PC だけ `Proto1\.claude\settings.local.json` の `permissions.allow` に次を入れる（GitHub には送らない。`.gitignore` 済み）。
  - `PowerShell(gh pr merge:*)`・`Bash(gh pr merge:*)`：PR のマージ
  - `PowerShell(.\tools\start-work.ps1:*)`：作業を始めるスクリプト
  - Notion の `notion-update-page` ツール（名前の `mcp__〇〇__` の部分は PC ごとに違うので、その PC のツール名で入れる）
  Visual Studio Enterprise の PC は設定済み。**メインPC はまだ**（メインPC の Claude は、管理者に確認してから作る）。
  許可を足しても、ブランチの削除などの「先に確認する」決まりは変わらない。
- **状態の記録**：タスクの状態は `tasks.md` と Notion の両方をそろえる（Notion はステータスと PR のリンクだけ）。プランは、このファイルの「次にやること」だけに書く。
- **確認と記録を軽くする**（2026-10-23、管理者が決めた。トークン節約）：画面を撮るのは見た目が変わるタスクだけ／わざと壊す確認は新しい計算のファイルだけを1回で／PR の説明は短く／このファイルの「今の状態」は短い要約だけ。詳しくは `CLAUDE.md` の「確認と記録を軽くする」。
- **プロトのルール**（`docs/plan/01_企画の要点.md`）：制限時間 90 秒・ダミー3体・場外で +1・L1 / R1 で左右の触腕（キーボードは左右クリック）・□ ではたく（E）・OPTIONS でやり直し（R）。
- **PS4 コントローラー**は外部ライブラリなしで読む（Raw Input と HID）。No.6 のテストのときに USB でつなぐ。
- **Maya のモデル**は No.22 まで使わない。そのときに FBX か OBJ で書き出したものをもらう。
- **時間**：動きは「1秒にどれだけ × `GameTimeGetDeltaTime()`」で書く。1フレームは 1/60 秒で固定（No.4 で決めた。`docs/dev/01_DX11基盤の構成.md` の「時間の使い方」）。
- **ビルド**：MSBuild の場所は PC によって違う（Community の PC と Enterprise の PC がある）。`vswhere.exe` で探してから使う（`docs/lessons.md` 参照）。
- **作業場所**：Claude が確かめのために一時フォルダ（%TEMP%）の worktree でビルドするときは `/t:Rebuild` を付ける（`docs/lessons.md` 参照）。

## 4. 別の PC・新しいセッションで始めるとき（Claude・Codex がやること）

Claude（Claude Code）でも Codex でも、共有の `Proto1` フォルダで同じ手順で続きから始める。もう一方の AI が作業中でないことを確認する（`AGENTS.md`）。

1. `CLAUDE.md`（Codex は `AGENTS.md` も）・`docs/lessons.md`・このファイル・`docs/tasks.md`・`docs/dev/01_DX11基盤の構成.md` を読む
2. `git status` だけを先に見る（自分のものではない変更があればユーザーに聞く）
3. `.\tools\fetch-latest.ps1` で develop を最新にし、`gh pr list` で開いている PR を確かめる
4. Notion の「Proto1（腕ダフルアームズ）」で、`tasks.md` と状態がそろっているか確かめる（ずれていたらユーザーに聞く。Notion を読めない AI は飛ばして、最後に管理者に伝える）
5. 「次にやること」の続きから始める（プランが確認待ちなら、ユーザーに確認を取ってから実装する）

## 5. 作業を終えるとき

- このファイルの「今の状態」「次にやること」「最終更新」を書き直す（どの PC の、どの AI（Claude／Codex）で更新したかも）
- `tasks.md` と Notion の状態をそろえる（Notion を書き換えられない AI は、管理者に「Notion の No.〇 を〇〇に」と伝える）
- 次にやるタスクのプランをまだ出していないときは、「次にやること」に「プランに入れること」のメモを残す（次の AI がそこから始められるように）
- 途中で終わるときは、作業中のブランチを `save-work.ps1` で GitHub に送っておく（別の PC で `start-work.ps1 <同じ作業名>` すると続きから始められる）
