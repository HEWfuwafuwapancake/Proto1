#include "game/menuse.h"
#include "audio/se.h"

namespace
{
	// 立っているビットの数
	int CountBits(unsigned bits)
	{
		int count = 0;
		for (; bits != 0; bits &= bits - 1) ++count;
		return count;
	}
}

void MenuSe::Update(const MENU_SE_STATE& before, const MENU_SE_STATE& after, bool back, bool decide, bool tuningReset)
{
	if (before.Scene != after.Scene) {
		// キャラ選択を出たら（マップ選択へ進むときを除く）、マップ選択から先へ進む・戻るときも、案内の声を止める。
		if ((before.Scene == SCENE_TITLE && after.Scene != SCENE_MAP_SELECT) || before.Scene == SCENE_MAP_SELECT) m_Guide.Stop();
		// 試合が時間切れで結果へ移るときなど、ボタンを押していない切り替えでは鳴らさない。
		// 前の画面へ戻ったら、戻るボタンでも「戻る」の項目の決定でも戻るの音。
		// 試合中のメニューを開く・閉じるのは決定と同じ音（OPTIONS / R は決定のボタンではないので、画面で見る）。
		if (back || (decide && SceneCalc::IsBackTransition(before.Scene, after.Scene))) SePlay(SE_MENU_BACK);
		else if (decide || before.Scene == SCENE_PAUSE || after.Scene == SCENE_PAUSE) SePlay(SE_MENU_DECIDE);
		// キャラ選択の画面に入ったら、メカガールの「キャラクターを選んでね」。
		if (after.Scene == SCENE_TITLE && before.Scene != SCENE_MAP_SELECT) m_Guide.Play(SE_CHARACTER_SELECT);
		return;
	}
	if (after.Scene == SCENE_MATCH) return;
	// 「標準に戻す」：変わった設定があれば変更の音、もう全部はじめの値なら戻るの音（どちらも1回だけ）。
	if (tuningReset) {
		SePlay(before.Values != after.Values ? SE_MENU_CURSOR : SE_MENU_BACK);
		return;
	}
	// キャラ選択：準備OK・参加は決定、取り消し・参加をやめるのは戻る。
	const int readyGain = CountBits(after.Ready & ~before.Ready);
	if (readyGain > 0) {
		bool mekaGirl = false;
		for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
			if ((after.Ready & ~before.Ready & (1u << slot)) && after.Characters[slot] == CHARACTER_MEKAGIRL) mekaGirl = true;
		}
		SePlay(SE_MENU_DECIDE);
		if (mekaGirl) m_Guide.Play(SE_MEKAGIRL_DECIDE); // 「キャラクターを選んでね」が残っていれば止めて鳴らす
		return;
	}
	if ((before.Ready & ~after.Ready) != 0 || (before.Joined & ~after.Joined) != 0) {
		m_Guide.Stop();
		SePlay(SE_MENU_BACK);
		return;
	}
	if ((after.Joined & ~before.Joined) != 0) {
		SePlay(SE_MENU_DECIDE);
		return;
	}
	// サウンドの画面で音量を変えたら、そのジャンルの音で大きさを聞かせる（声は案内の枠で鳴らし、続けて変えても重ねない）。
	if (after.Scene == SCENE_SOUND && before.Values != after.Values && before.Cursor == after.Cursor) {
		if (after.Cursor == SOUND_ITEM_VOICE) m_Guide.Play(SE_MEKAGIRL_DECIDE);
		else SePlay(after.Cursor == SOUND_ITEM_BATTLE ? SE_HAMMER_TAP : SE_MENU_CURSOR);
		return;
	}
	// カーソル・選んでいる値が動いた。
	if (before.Cursor != after.Cursor || before.Map != after.Map || before.Rule != after.Rule || before.Focus != after.Focus
		|| before.Values != after.Values || before.Characters != after.Characters || before.CpuLevels != after.CpuLevels) {
		SePlay(SE_MENU_CURSOR);
	}
}
