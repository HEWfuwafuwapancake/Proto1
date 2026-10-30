// タイトル・試合・結果の遷移とメニューの主な動き（DirectX なし）
#include "CppUnitTest.h"
#include "physics/scenecalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	static void SelectMode(SceneCalc& scene, MODE_ITEM item)
	{
		for (int i = 0; i < item; ++i) {
			scene.Step({ false, false, 0.0f }, false);
			scene.Step({ false, false, -1.0f }, false);
		}
	}

	TEST_CLASS(SceneCalcTest)
	{
	public:
		TEST_METHOD(Title_DecideStartsMatch)
		{
			SceneCalc scene;
			scene.Start(true);
			Assert::AreEqual((int)SCENE_TITLE, (int)scene.GetScene());
			Assert::AreEqual((int)SCENE_EVENT_NONE, (int)scene.Step({ false, false, 0 }, false));
			Assert::AreEqual((int)SCENE_EVENT_START_MATCH, (int)scene.Step({ true, false, 0 }, false));
			Assert::AreEqual((int)SCENE_MATCH, (int)scene.GetScene());
		}

		TEST_METHOD(Match_DecideOnlyShowsResultAfterTimeUp)
		{
			SceneCalc scene;
			scene.Start(true);
			scene.Step({ true, false, 0 }, false);
			Assert::AreEqual((int)SCENE_EVENT_NONE, (int)scene.Step({ true, false, 0 }, false));
			Assert::AreEqual((int)SCENE_MATCH, (int)scene.GetScene());
			Assert::AreEqual((int)SCENE_EVENT_NONE, (int)scene.Step({ false, false, 0 }, true));
			Assert::AreEqual((int)SCENE_EVENT_SHOW_RESULT, (int)scene.Step({ true, false, 0 }, true));
			Assert::AreEqual((int)SCENE_RESULT, (int)scene.GetScene());
			Assert::AreEqual((int)RESULT_ITEM_RETRY, scene.GetCursor());
		}

		TEST_METHOD(Restart_StartsMatchDuringPlayAfterTimeUpAndFromResult)
		{
			SceneCalc scene;
			scene.Start(true);
			scene.Step({ true, false, 0 }, false);
			Assert::AreEqual((int)SCENE_EVENT_START_MATCH, (int)scene.Step({ false, true, 0 }, false));
			Assert::AreEqual((int)SCENE_EVENT_START_MATCH, (int)scene.Step({ true, true, 0 }, true));
			Assert::AreEqual((int)SCENE_MATCH, (int)scene.GetScene());
			scene.Step({ true, false, 0 }, true);
			scene.Step({ false, false, -1 }, true); // TITLE を選んでいてもやり直せる
			Assert::AreEqual((int)SCENE_EVENT_START_MATCH, (int)scene.Step({ false, true, 0 }, true));
			Assert::AreEqual((int)SCENE_MATCH, (int)scene.GetScene());
		}

		TEST_METHOD(Result_DecideRetriesOrReturnsToTitle)
		{
			SceneCalc scene;
			scene.Start(true);
			scene.Step({ true, false, 0 }, false);
			scene.Step({ true, false, 0 }, true);
			Assert::AreEqual((int)SCENE_EVENT_START_MATCH, (int)scene.Step({ true, false, 0 }, true));
			Assert::AreEqual((int)SCENE_MATCH, (int)scene.GetScene());
			scene.Step({ true, false, 0 }, true);
			scene.Step({ false, false, -1 }, true);
			Assert::AreEqual((int)RESULT_ITEM_TITLE, scene.GetCursor());
			Assert::AreEqual((int)SCENE_EVENT_BACK_TITLE, (int)scene.Step({ true, false, 0 }, true));
			Assert::AreEqual((int)SCENE_TITLE, (int)scene.GetScene());
		}

		TEST_METHOD(Menu_HeldStickWaitsForNeutralAndMovesOncePerFlick)
		{
			SceneCalc scene;
			scene.Start(true);
			scene.Step({ true, false, 0 }, false);
			scene.Step({ true, false, -1 }, true); // 入る前から倒している
			scene.Step({ false, false, -1 }, true);
			Assert::AreEqual((int)RESULT_ITEM_RETRY, scene.GetCursor());
			scene.Step({ false, false, 0 }, true);
			scene.Step({ false, false, -0.7f }, true);
			Assert::AreEqual((int)RESULT_ITEM_TITLE, scene.GetCursor());
			for (int i = 0; i < 3; ++i) scene.Step({ false, false, -1 }, true);
			scene.Step({ false, false, 1 }, true); // 戻さず逆に倒しても次の弾きにしない
			Assert::AreEqual((int)RESULT_ITEM_TITLE, scene.GetCursor());
			scene.Step({ false, false, 0.2f }, true);
			scene.Step({ false, false, 0.7f }, true);
			Assert::AreEqual((int)RESULT_ITEM_RETRY, scene.GetCursor());
		}

		TEST_METHOD(GameStart_SoloStartsAndLocalResultReturnsToModes)
		{
			SceneCalc scene;
			scene.Start();
			Assert::AreEqual((int)SCENE_GAME_START, (int)scene.GetScene());
			scene.Step({ true, false, 0 }, false);
			Assert::AreEqual((int)SCENE_MODE_SELECT, (int)scene.GetScene());
			Assert::AreEqual((int)SCENE_EVENT_START_MATCH, (int)scene.Step({ true, false, 0 }, false));
			scene.Step({ true, false, 0 }, true);
			scene.Step({ false, false, -1 }, true);
			Assert::AreEqual((int)SCENE_EVENT_BACK_TITLE, (int)scene.Step({ true, false, 0 }, true));
			Assert::AreEqual((int)SCENE_MODE_SELECT, (int)scene.GetScene());
		}

		TEST_METHOD(Modes_LocalLobbyAndBackToGameStart)
		{
			SceneCalc scene;
			scene.ShowModes();
			SelectMode(scene, MODE_ITEM_LOCAL);
			scene.Step({ true, false, 0 }, false);
			Assert::AreEqual((int)SCENE_TITLE, (int)scene.GetScene());
			Assert::AreEqual((int)SCENE_EVENT_BACK_MENU, (int)scene.Step({ false, false, 0, true }, false));
			Assert::AreEqual((int)SCENE_MODE_SELECT, (int)scene.GetScene());
			SelectMode(scene, MODE_ITEM_BACK);
			scene.Step({ true, false, 0 }, false);
			Assert::AreEqual((int)SCENE_GAME_START, (int)scene.GetScene());
		}

		TEST_METHOD(Modes_HostAndValidatedJoinOpenLobby)
		{
			SceneCalc scene;
			scene.ShowModes();
			SelectMode(scene, MODE_ITEM_HOST);
			Assert::AreEqual((int)SCENE_EVENT_HOST, (int)scene.Step({ true, false, 0 }, false));
			scene.OpenLobby();
			Assert::AreEqual((int)SCENE_TITLE, (int)scene.GetScene());
			scene.Step({ false, false, 0, true }, false);
			SelectMode(scene, MODE_ITEM_JOIN);
			scene.Step({ true, false, 0 }, false);
			Assert::AreEqual((int)SCENE_JOIN_ADDRESS, (int)scene.GetScene());
			Assert::AreEqual((int)SCENE_EVENT_NONE, (int)scene.Step({ true, false, 0, false, false }, false));
			Assert::AreEqual((int)SCENE_EVENT_JOIN, (int)scene.Step({ true, false, 0, false, true }, false));
			scene.Step({ false, false, 0, true }, false);
			Assert::AreEqual((int)SCENE_MODE_SELECT, (int)scene.GetScene());
			scene.OpenLobby(); // 参加成功後も切断時には直接モード選択へ戻せる。
			scene.ShowModes();
			Assert::AreEqual((int)SCENE_MODE_SELECT, (int)scene.GetScene());
		}
	};
}
