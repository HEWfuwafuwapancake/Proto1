# 引き継ぎメモ（今の状態・次にやること）

作業を始めるときに読みます。ここには**今の状態と次にやることだけ**を書きます。
タスクの一覧は [tasks.md](tasks.md)（Notion の「[Proto1（腕ダフルアームズ）](https://app.notion.com/p/e5e9f1983bbe4758b06b4adba135a491)」と同じ）、決まりは [decisions.md](decisions.md)。

- 最終更新：2026-11-26（Claude。Shota の PC、`Proto1`。人が読む文書を読みやすく整理した）

## 1. 今の状態

- No.206（横の場外の壁・DEAD LINE の帯）まで develop に入っている。対戦マップは6つ（郵便局・市場・ネオンビル群・コロシアム・廃駅・浮島）。
- 文書の整理（`feature/docs-readability`）：README を足し、`tasks.md` は終わっていないタスクだけ、`decisions.md` はテーマごとに、`dev/01` は手引きと細かい記録（`dev/05_仕組みの詳細.md`）に分けた。終わったタスクと古い決まりは `archive/` へ移した。
- 実装は済んでいて、管理者の試遊・試聴を待っているものが下に残っている。終わったタスクの細かい記録は [archive/handoff_2026-11.md](archive/handoff_2026-11.md)。

## 2. 次にやること

### 管理者の試遊・試聴待ち（新しい順）

| No | 確かめること | 調整する値 |
|---|---|---|
| 206 横の場外の壁 | 100% の相手に端で長押しハンマーを当てて場外になるか。「端ぴったり」の段で、立っているだけで場外にならないか。帯の見え方（分割画面・ネオン・廃駅・市場はまだ撮っていない）。試し打ちは F5（相手の %）・F6（相手を呼ぶ）・F7（CPU を止める） | `tuningcalc.cpp` の `RING_OUT_NORMAL`・`RING_OUT_RATES`、帯の濃さは `ringoutwall.cpp` の `BAND_ALPHA` |
| 204・205 追加マップ3面 | 広さ・見た目・霧。廃駅の倒壊ビルを渡る・下をくぐる、コロシアムの外周から落ちる。ジャンプ台で跳ぶところはまだ撮っていない。CPU は柱・ジャンプ台・倒壊ビルを考えずに動く | 霧は `stage.cpp` の `FOG_DISTANCE`・`FOG_AMOUNT`、ジャンプ台は `mapgimmick.cpp` の `PAD_RISE_HEIGHT`、柱は `pillarcalc.cpp` の `DURABILITY` と `mapgimmick.cpp` の `PILLAR_DAMAGE`・`PILLAR_POWER` |
| 203 キーボードの操作 | J / K の腕（単押し・長押し）を遊んで確かめる。試合中の操作一覧は、明るい地面の上で読みにくい。パッド版の一覧はまだ撮っていない | `hud.cpp` の `CONTROL_GUIDE_TEXT`・`CONTROL_GUIDE_COLOR`・`CONTROL_GUIDE_BACKGROUND` |
| 200・201 音量と保存 | ジャンルごとの音量を耳で確かめる。保存はオプションの「保存する」でだけ（作業フォルダの `settings.ini`） | ジャンルの分け方は `se.cpp` の `GetGenre` |
| 195 効果音 | 戻る音のタイミング、オンラインの準備OKの取り消し（2つ起動）、戦闘の音の距離と左右、腕8種の音 | `sespatialcalc.cpp` の `NEAR_DISTANCE`・`FAR_VOLUME`・`THIRD_PARTY_VOLUME`・`PAN_MAX`、`se.cpp` の `SE_TABLE` |
| 193・194 地形とカメラ | 坂の隙間・下から見たちらつき、地形を背にしたカメラ。高い壁を背にしてハードロックすると、自分が画面から外れることがある | `camera.cpp` の `CLIMB_SMOOTH_TIME`・`NEAR_CHECK_RADIUS` |
| 189〜191 対戦マップ２面 | 広さ・高さ・色、触手のＧ、ダッシュ板、カプセルの散らばり、オンラインでホストがマップを選べるか、仕分け台のすり抜け。刀のワープ先・触手の箱の当たりは浮島の形のまま、CPU は地形の壁とＧを考えない | `mapgimmick.cpp` の `BOOST_SPEED`・`BOOST_UP_SPEED` |
| 187・188 パッドのメニュー操作 | 左スティックはカーソル、十字キーは中身。パッドの実機でまだ確かめていない | — |
| 186・180 モーニングスター | 後ろ・横にも当たるか、耐久の減り方 | `arm_flail.cpp` の `SPIN_DRAIN_COST` |
| 184・185 メニューとポーズ | 色・文字・背景のぼけ、ポーズからロビー・タイトルへ戻れるか | `codex_menu.cpp`、ぼけは `backdrop.cpp` の `DRAW_LEVEL` |
| 182 ミサイル腕 | 避けやすさ、場外になる %、耐久の持ち ×0.5 で2本ずつ撃つか | `arm_missile.cpp` の `SHOT_SPEED`・`SHOT_PARAMS`・`FINGER_POWER`・`ARM_POWER` |
| 179 視点感度 | オプションの行の見た目と効き方 | `action.cpp` の `LOOK_SENSITIVITY_SCALES` |
| 178 槍 | 踏み込み・穂先の強さ、CPU の間合い | `arm_spear.cpp` の `LUNGE_SPEED`・`THRUST`、`cpu.cpp` の `ARM_STYLES` |
| 176・177 CPU と吹っ飛び | CPU がガタガタしないか、動けない長さ、吹っ飛び中のダッシュ | `HITSTUN_PER_PERCENT`、CPU の強さは `cpucalc.cpp` の `LEVEL_PARAMS` |
| 53 ほか | パッド全般・別 PC 間のオンラインを実機で（管理者・メンバー） | — |

効果音の素材の担当と決めること：腕の破壊音、長押しの攻撃ボイス・撃墜ボイス・共通の被弾音、レーザーの着弾音、モーニングスターの重い衝撃。

### 管理者が判断すること

- No.206 の赤の RING OUT!（場外になった所を復活まで見せる）を残すか。「エフェクトは作らない」（decisions.md）に当たるかもしれないが、依頼者の判断で足した。
- No.206 のコロシアムの帯の見せ方（壁が客席より外にあり、帯が客席に隠れてほとんど見えない）。
- 爪腕（No.183）を作るか（仕様は会話で決まっている）。
- CPU をオンラインの空き枠にも出すか（全員の PC で同じ入力を作れるかの確認と、通信の版の更新が要る）。
- GAME の試合のあとに DEBUG へ移ったとき、持っていた腕を残すか（今は残す）。
- 腕ごとの耐久の減り方・カプセルの数と出る間隔を、遊んで調整する（各腕のファイルと `capsulecalc.cpp`）。
- しびれ以外の状態異常と、Q / E の段の切り替えを残すか。
- 使わなくなった OBJ（磁石・グラップリング・盾の板・ハサミの残り）を消すか。
- 【当面】の AI の分担（decisions.md の「2. AI の使い方」。Claude の担当も Codex が行う）が今も効いているか。最近は Claude が実装している。
