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

void MenuSe::Update(const MENU_SE_STATE& before, const MENU_SE_STATE& after, bool back, bool decide)
{
	if (before.Scene != after.Scene) {
		// 試合が時間切れで結果へ移るときなど、ボタンを押していない切り替えでは鳴らさない。
		// 試合中のメニューを開く・閉じるのは決定と同じ音（OPTIONS / R は決定のボタンではないので、画面で見る）。
		if (back) SePlay(SE_MENU_BACK);
		else if (decide || before.Scene == SCENE_PAUSE || after.Scene == SCENE_PAUSE) SePlay(SE_MENU_DECIDE);
		// キャラ選択の画面に入ったら、メカガールの「キャラクターを選んでね」。
		if (after.Scene == SCENE_TITLE && before.Scene != SCENE_MAP_SELECT) SePlay(SE_CHARACTER_SELECT);
		return;
	}
	if (after.Scene == SCENE_MATCH) return;
	// キャラ選択：準備OK・参加は決定、取り消し・参加をやめるのは戻る。
	const int readyGain = CountBits(after.Ready & ~before.Ready);
	if (readyGain > 0) {
		bool mekaGirl = false;
		for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
			if ((after.Ready & ~before.Ready & (1u << slot)) && after.Characters[slot] == CHARACTER_MEKAGIRL) mekaGirl = true;
		}
		SePlay(SE_MENU_DECIDE);
		if (mekaGirl) SePlay(SE_MEKAGIRL_DECIDE);
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
	// カーソル・選んでいる値が動いた。
	if (before.Cursor != after.Cursor || before.Map != after.Map || before.Rule != after.Rule || before.Focus != after.Focus
		|| before.Values != after.Values || before.Characters != after.Characters || before.CpuLevels != after.CpuLevels) {
		SePlay(SE_MENU_CURSOR);
	}
}
