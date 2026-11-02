# 引き継ぎメモ（今の状態・次にやること）

作業を始めるときに読み、終えるときに書き直す。**ここには「今の状態」と「次にやること」だけを短く書く**（細かい記録は [archive/](archive/) へ移す）。
タスクの状態は [tasks.md](tasks.md)（Notion の「[Proto1（腕ダフルアームズ）](https://app.notion.com/p/e5e9f1983bbe4758b06b4adba135a491)」と同じ）、決まったことは [decisions.md](decisions.md)。
必要なときだけ読むもの：腕のモデルは [dev/02_腕のモデル.md](dev/02_腕のモデル.md)、画面を撮る方法は [dev/04_画面確認の方法.md](dev/04_画面確認の方法.md)、腕の仕様は [plan/03_新しいアームの仕様.md](plan/03_新しいアームの仕様.md)。

- 最終更新：2026-11-02（Codex。Shota の Proto1、Visual Studio 2022 Enterprise の PC。No.57 段階2の実装・Debug ビルド済み。段階3待ち）

---

## 1. 今の状態

- **No.57 全画面**：ブランチ `feature/fullscreen`。**Codex が実装。Claude の確認待ち（段階2 済み。段階3 待ち）**。枠なし全画面・F11 / Alt+Enter・表示用の引数・デバッグ用ウィンドウ・オンラインの左上警告・関連ドキュメントを実装。Debug（x64）のビルド成功、警告の出力なし。Release・単体テスト・画面確認・PR は未実施。段階3は AGENTS.md の当面の分担に従い Codex が担当する。
- No.56 でオンラインは「同じ develop の版を各自でビルド」でつなげる（VS の版が違うとタイトルバーに警告。ずれたら版をそろえる）。`ModelLoad` は同じ引数（パス・左右反転・色替え材質）なら読み直さず同じ番号を返す。No.54 Switch 2 Pro コン（USB・振動）は完了（PR #104）。No.19 は優先度：低へ下げた。
- 実機での確認待ち（詳しくは [archive/handoff_2026-11.md](archive/handoff_2026-11.md)）：
  - No.53 ほかのコントローラー（XInput・Switch Pro 形式）：メンバーの実機確認。動かなければ Debug の出力の「対応外のゲームパッド VID=… PID=…」を見る。
  - No.53・54：DS4 / DualSense / XInput と混ぜてつないだときの番号。
  - No.51・52：Tailscale 4人での ping・自動 delay・待ち・DESYNC。
  - No.48：パッドでのメニュー操作、LOCAL VS で試合を始めて結果の TITLE からモード選択へ戻ること。
- No.47 の腕の取付と強さは管理者が遊んで調整する。マウス操作は止めたまま。

## 2. 次にやること：No.57 全画面の段階3（テスト）

### プラン（管理者の確認済み）

今は枠ありの 1280×720 のウィンドウ（`main.cpp` の `CreateWindow`）で、スワップチェーンは `Windowed = TRUE` の 1280×720。
3D のビューポート・カメラ・HUD は全部 `SCREEN_WIDTH / SCREEN_HEIGHT`（1280×720）で描いている。また今は DXGI の標準の動きで、Alt+Enter で排他フルスクリーン（解像度が変わる）になってしまう。

**やり方**：枠なしのウィンドウ（`WS_POPUP`）を、今いるモニターの大きさに広げる。**バックバッファは 1280×720 のまま**にし、`Present` が画面いっぱいに引き伸ばすのに任せる。
`SCREEN_WIDTH` を使っている `src/game`・`shader.cpp` は変えない（モニター解像度で描く案は、ぼやけが気になったら別のタスクにする）。

1. **`src/core/dx11.cpp`**：スワップチェーンを作ったあと、`IDXGIDevice` → `IDXGIAdapter` → `IDXGIFactory` をたどって `MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER)` を呼ぶ（DXGI の排他フルスクリーンを止める）。使ったものは解放する。バックバッファの大きさは変えない。
2. **`src/core/main.cpp`**：
   - 全画面とウィンドウを切り替える `static void SetFullscreen(HWND hWnd, bool fullscreen)` を作る。全画面：今のウィンドウの位置を覚えておき、スタイルを `WS_POPUP | WS_VISIBLE` にして、`MonitorFromWindow` → `GetMonitorInfo` の `rcMonitor` に `SetWindowPos`（`HWND_TOP`・`SWP_FRAMECHANGED`）。ウィンドウ：今のスタイル（`WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX`。`CreateWindow` と同じ定数を使い回す）と覚えておいた位置に戻す。
   - 起動したときは全画面。起動の引数に `--windowed` があればウィンドウで起動する。`--windowed` は `main.cpp` で引数の文字列から取りのぞいてから `SessionCalcParseOptions` に渡す（`NET_OPTIONS` は通信の設定なので足さない）。`--host` / `--join` と一緒に使える。引数のエラーの MessageBox の案内にも `--windowed` を足す。
   - **F11**（`WM_KEYDOWN`）と **Alt+Enter**（`WM_SYSKEYDOWN` の `VK_RETURN`。キーを押しっぱなしのくり返し（lParam の bit30）は無視）で切り替える。ほかの `WM_SYSKEYDOWN` は今までどおり `DefWindowProc` へ。
   - 全画面の間は、ウィンドウの中でカーソルを隠す（`WM_SETCURSOR` で `HTCLIENT` なら `SetCursor(nullptr)` して TRUE を返す）。
   - Esc の終了確認（MessageBox）は今のままでよい（枠なしのウィンドウなので上に出る）。出なければ直す。
3. **デバッグ用のウィンドウ**（全画面ではタイトルバーが見えなくなるため。2026-11-02、管理者が「プラグイン（ImGui など）ではなく、プログラムで2つ目のウィンドウ」と決めた）：
   - **Debug ビルドでは起動のときにいつも出す**。Release では起動の引数 `--debugwindow` を付けたときだけ出す（`--windowed` と同じく `main.cpp` で取りのぞく）。
   - Win32 の普通のウィンドウ（枠あり・大きさを変えられる）を1つ作り、中に読み取り専用の複数行の `EDIT`（またはウィンドウに `DrawText`）を置いて、**今 `RefreshTitle` で作っている文字列をそのまま**（日本語も）出す。長い行は見やすいように区切りで改行してよい（FPS・進行／装備・振動／接続・ping・待ち／DESYNC・VS の版）。更新は `RefreshTitle` を呼ぶときと同じ（1秒ごと・装備が変わったとき）。タイトルバーへの表示も今のまま残す。
   - ウィンドウの作りは `src/core/` に新しいファイル（例：`debugwindow.h / .cpp`、`DebugWindowInitialize / Finalize / SetText`。ひな形と同じ書き方）を足す。`.vcxproj`・`.vcxproj.filters`（`src\core`）と `docs/dev/01_DX11基盤の構成.md` にも追加する。
   - 置き場所：モニターが2枚以上なら、ゲームと違うモニターの左上に出す。1枚ならゲームのウィンドウの右など、適当な位置でよい（全画面のゲームに隠れたら F11 でウィンドウに戻して見る）。`WS_EX_TOPMOST` にはしない。
   - デバッグ用のウィンドウを閉じても（×）ゲームは終わらない（隠すだけ。`WM_CLOSE` で `ShowWindow(SW_HIDE)`）。キーの入力はゲームのウィンドウのものだけを使う（今の `WM_ACTIVATE` の処理のままでよい）。
4. **全画面の間に画面の左上に出す警告**（デバッグをしない Release のメンバーにも見えるように）：オンラインのときだけ、DESYNC が出たら `DESYNC 2P FRAME 1234`、VS の版が違う人がいれば `VS VERSION DIFFERS` を赤など目立つ色で出す。
   - 作り：`NetSession` に英字だけの短い文字列を返す関数（例：`GetCornerWarningText()`）を足し、`Hud` が全画面のビューポートに戻したあと（`DrawScene` の後など）に `Number2DDrawText` で描く。全画面かどうかは `Game` に public の設定関数（例：`SetFullscreen(bool)`）を足して `main.cpp` から渡す。使える文字は `GlyphCalcLetterRow`（Q・Z・`:` はない）。大きさ・位置・色は `hud.cpp` の「調整用の値」に置く。
5. **ドキュメント**：`docs/dev/01_DX11基盤の構成.md`（全画面の作り）・`docs/team/04_コマンド一覧.md`（F11・Alt+Enter・`--windowed`）・`docs/dev/04_画面確認の方法.md`（画面確認は `--windowed` で起動する）を直す。`--debugwindow` も `04_コマンド一覧.md` に書く。

**確かめ方**：計算は増えないのでテストは足さない（`run-tests.ps1` が今のまま通ること）。Debug / Release で警告 0。見た目が変わるので画面を撮る：起動して全画面 → F11・Alt+Enter でウィンドウと行き来（ウィンドウは元の 1280×720 に戻る）・1人・LOCAL VS の4分割・オンライン（2つ起動。片方 `--windowed`）・デバッグ用のウィンドウ（Debug で出る・Release は `--debugwindow` のときだけ・日本語が読める・閉じてもゲームは続く・クリックしてもパッドで遊べるか）・左上の警告（DESYNC は確かめにくいので、一時的に条件を真にして見た目だけ見てよい）・Esc の終了確認・Alt+Tab で別のアプリへ行って戻る・カーソルが消えること。

### 段階3：Codex に渡すプロンプト

```text
Proto1 の No.57 全画面を、ブランチ feature/fullscreen で確かめてください（段階3 テスト）。
AGENTS.md の「作業を始める前に必ず読むもの」を読み、git status を先に確認してください。
段階2は Codex が実装し、Debug のビルドだけ確認済みです。AGENTS.md の当面の分担に従い、段階3も Codex が担当します。
docs/handoff.md のプランとコードを照らし合わせ、Debug / Release 警告 0・run-tests.ps1（Debug・Release）、
プランの「確かめ方」のとおりに画面を撮って確かめ、問題があれば直してください。
新しい計算ファイル・テストは増えていないので、わざと計算を壊す確認は不要です。
終わったら tasks.md を ✅ にし、submit-pr.ps1 で PR を出し、Notion を「完了」と PR のリンクにしてください。
テスト・Debug / Release 警告0が通っていれば、AGENTS.md の手順でマージし、handoff.md を書き直してください。
No.57 のあとは優先度：低の未完了タスクだけなので、次に何をするかは管理者に聞いてください。
```

### そのほか

1. No.57 のあと：残りの未完了は優先度：低（No.19・21・23・24・27・28）と、No.24・27 待ちの No.40（ハサミ。最後）だけ。次に何をするかを管理者に聞く。
2. 管理者が全員に同じ Release の exe を配り、Tailscale 4人で ping・自動 delay・待ち・進行を以前（delay 7、ping 33・66・33ms）と比べる（No.52）。No.51 の DESYNC 再発も確認する。
3. メンバーが No.53 のコントローラーを実機で確かめる。
