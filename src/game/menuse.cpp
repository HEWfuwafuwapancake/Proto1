#include "game/menuse.h"
#include "audio/se.h"

void MenuSe::Update(const MENU_SE_STATE& before, const MENU_SE_STATE& after, bool back, bool decide, bool tuningReset, bool optionsSaved)
{
	if (before.Scene != after.Scene) {
		// 試合が時間切れで結果へ移るときなど、ボタンを押していない切り替えでは鳴らさない。
		// 前の画面へ戻ったら、戻るボタンでも「戻る」の項目の決定でも戻るの音。
		// 試合中のメニューを開く・閉じるのは決定と同じ音（OPTIONS / R は決定のボタンではないので、画面で見る）。
		if (back || (decide && SceneCalc::IsBackTransition(before.Scene, after.Scene))) SePlay(SE_MENU_BACK);
		else if (decide || before.Scene == SCENE_PAUSE || after.Scene == SCENE_PAUSE) SePlay(SE_MENU_DECIDE);
		return;
	}
	if (after.Scene == SCENE_MATCH) return;
	// 「保存」：ファイルに書けたら決定の音。
	if (optionsSaved) {
		SePlay(SE_MENU_DECIDE);
		return;
	}
	// 「標準に戻す」：変わった設定があれば変更の音、もう全部はじめの値なら戻るの音（どちらも1回だけ）。
	if (tuningReset) {
		SePlay(before.Values != after.Values ? SE_MENU_CURSOR : SE_MENU_BACK);
		return;
	}
	// キャラ選択：準備OK・参加は決定、取り消し・参加をやめるのは戻る。
	if ((after.Ready & ~before.Ready) != 0) {
		SePlay(SE_MENU_DECIDE);
		return;
	}
	if ((before.Ready & ~after.Ready) != 0 || (before.Joined & ~after.Joined) != 0) {
		SePlay(SE_MENU_BACK);
		return;
	}
	if ((after.Joined & ~before.Joined) != 0) {
		SePlay(SE_MENU_DECIDE);
		return;
	}
	// サウンドの画面で音量を変えたら、そのジャンルの音で大きさを聞かせる。
	if (after.Scene == SCENE_SOUND && before.Values != after.Values && before.Cursor == after.Cursor) {
		SePlay(after.Cursor == SOUND_ITEM_BATTLE ? SE_HAMMER_TAP : SE_MENU_CURSOR);
		return;
	}
	// カーソル・選んでいる値が動いた。
	if (before.Cursor != after.Cursor || before.Map != after.Map || before.Rule != after.Rule || before.Focus != after.Focus
		|| before.Values != after.Values || before.Characters != after.Characters || before.CpuLevels != after.CpuLevels) {
		SePlay(SE_MENU_CURSOR);
	}
}
