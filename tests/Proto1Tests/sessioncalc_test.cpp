#include <gtest/gtest.h>
#include "net/sessioncalc.h"

namespace Proto1Tests
{
	// 実際に送るバイト列を通し、ソケットなしでホストと参加者をつなぐ。
	static void Deliver(SessionCalc& receiver, int peer, const SESSION_MESSAGE& message)
	{
		std::vector<uint8_t> bytes;
		EXPECT_TRUE(SessionCalc::Encode(message, bytes));
		SESSION_MESSAGE decoded;
		EXPECT_TRUE(SessionCalc::Decode(bytes.data(), bytes.size(), decoded));
		receiver.Receive(peer, decoded);
	}

	static PLAYER_INPUT MakeInput(int slot, int frame)
	{
		auto input = ActionCalcPlayerInput({ (slot + 1) * 0.2f, 0.0f }, { 0.0f, 0.01f },
			frame % 2 == 0 ? (1u << ACTION_JUMP) : 0);
		input.Equip = { slot % 2, frame % 5 };
		return input;
	}

	struct SessionNetwork
	{
		SessionCalc Nodes[FIGHTER_COUNT];
		int Count;

		SessionNetwork(int count, int delay) : Count(count)
		{
			NET_OPTIONS options;
			options.Mode = NET_MODE_HOST;
			options.Delay = delay;
			Nodes[0].Initialize(options, 100);
			for (int slot = 1; slot < count; ++slot) {
				options.Mode = NET_MODE_JOIN;
				options.Delay = LOCKSTEP_DEFAULT_DELAY; // ホストの値で上書きされる。
				Nodes[slot].Initialize(options, 100 + slot);
				EXPECT_EQ(slot, Nodes[0].Join(100 + slot));
				Deliver(Nodes[slot], 0, Nodes[0].MakeControl(slot));
				Nodes[slot].Decide();
				Deliver(Nodes[0], slot, Nodes[slot].MakeControl());
			}
			Nodes[0].Decide();
			Nodes[0].Decide();
			for (int slot = 1; slot < count; ++slot) Deliver(Nodes[slot], 0, Nodes[0].MakeControl(slot));
		}

		void Capture()
		{
			for (int slot = 0; slot < Count; ++slot) {
				if (Nodes[slot].NeedsInput()) Nodes[slot].CaptureInput(MakeInput(slot, Nodes[slot].GetNextFrame()));
			}
		}

		void Exchange(int dropUp = -1, int dropDown = -1)
		{
			for (int slot = 1; slot < Count; ++slot) {
				if (slot != dropUp) Deliver(Nodes[0], slot, Nodes[slot].MakeInputs());
			}
			for (int slot = 1; slot < Count; ++slot) {
				if (slot != dropDown) Deliver(Nodes[slot], 0, Nodes[0].MakeInputs(slot));
			}
		}
	};

	TEST(SessionCalcTest, Address_KeyboardEditsLanPrefixAndRejectsIncompleteOrOutOfRange)
	{
		SessionAddressCalc address;
		address.Initialize("192.168.1.10");
		EXPECT_EQ(std::string("192.168.1."), address.GetText());
		EXPECT_FALSE(address.IsValid());
		address.AppendDigit(2);
		address.AppendDigit(5);
		address.AppendDigit(6);
		EXPECT_FALSE(address.IsValid());
		EXPECT_TRUE(address.Erase());
		address.AppendDigit(4);
		EXPECT_TRUE(address.IsValid());
		EXPECT_EQ(std::string("192.168.1.254"), address.GetText());
		NET_OPTIONS options;
		EXPECT_TRUE(SessionCalcParseOptions("--join " + address.GetText(), options));
		while (address.Erase()) {} // 空になった次の Backspace は戻る。
		EXPECT_EQ(std::string("..."), address.GetText());
		for (char c : std::string("127.0.0.1")) {
			if (c == '.') address.Move(1);
			else address.AppendDigit(c - '0');
		}
		EXPECT_EQ(std::string("127.0.0.1"), address.GetText());
		EXPECT_TRUE(address.IsValid());
	}

	TEST(SessionCalcTest, Address_DpadMovesAndChangesSelectedPart)
	{
		SessionAddressCalc address;
		address.Initialize("192.168.1.10");
		address.Adjust(1);
		address.Adjust(1);
		EXPECT_EQ(std::string("192.168.1.1"), address.GetText());
		address.Move(-1);
		EXPECT_EQ(2, address.GetCursor());
		address.Adjust(1);
		EXPECT_EQ(std::string("192.168.2.1"), address.GetText());
		address.Adjust(-1);
		EXPECT_EQ(std::string("192.168.1.1"), address.GetText());
		EXPECT_TRUE(address.IsValid());
	}

	TEST(SessionCalcTest, Options_LocalHostJoinAndDelay)
	{
		NET_OPTIONS options;
		EXPECT_TRUE(SessionCalcParseOptions("", options));
		EXPECT_EQ(static_cast<int>(NET_MODE_LOCAL), static_cast<int>(options.Mode));
		EXPECT_TRUE(SessionCalcParseOptions("--host", options));
		EXPECT_EQ(LOCKSTEP_AUTO_DELAY, options.Delay);
		EXPECT_TRUE(SessionCalcParseOptions("--delay 6 --host", options));
		EXPECT_EQ(6, options.Delay);
		EXPECT_TRUE(SessionCalcParseOptions("--join 100.64.1.2 --delay 7", options));
		EXPECT_EQ(std::string("100.64.1.2"), options.Host);
		EXPECT_EQ(static_cast<int>(NET_MODE_JOIN), static_cast<int>(options.Mode));
		for (const char* invalid : { "--host --join 127.0.0.1", "--join", "--delay 3", "--host --delay 121",
			"--host --delay 3bad", "--join 1.2.3.999", "--unknown" }) {
			EXPECT_FALSE(SessionCalcParseOptions(invalid, options));
		}
		EXPECT_EQ(7, options.Delay); // 失敗時に有効な設定を壊さない。
	}

	TEST(SessionCalcTest, Lobby_WaitsForDevicesAssignsFourSlotsAndLocksRoster)
	{
		NET_OPTIONS options;
		options.Mode = NET_MODE_HOST;
		options.Delay = 6;
		SessionCalc host, client;
		host.Initialize(options, 100);
		options.Mode = NET_MODE_JOIN;
		client.Initialize(options, 101);
		EXPECT_EQ(1, host.Join(101));
		EXPECT_EQ(1, host.Join(101)); // 再要求は同じ枠。
		host.Decide();
		host.Decide();
		EXPECT_FALSE(host.IsRunning());
		Deliver(client, 0, host.MakeControl(1));
		EXPECT_EQ(1, client.GetLocalSlot());
		EXPECT_EQ(6, client.GetDelay());
		client.Decide();
		client.Decide();
		EXPECT_FALSE(client.IsRunning()); // 参加側は開始できない。
		Deliver(host, 1, client.MakeControl());
		host.Decide();
		EXPECT_TRUE(host.IsRunning());
		Deliver(client, 0, host.MakeControl(1));
		EXPECT_TRUE(client.IsRunning());
		EXPECT_EQ(-1, host.Join(102));
		host.ReturnToLobby();
		EXPECT_EQ(-1, host.Join(102));
		EXPECT_EQ(3, static_cast<int>(host.GetReadyMask()));
		SessionNetwork four(4, 3);
		EXPECT_EQ(15, static_cast<int>(four.Nodes[0].GetMask()));
		EXPECT_EQ(-1, four.Nodes[0].Join(999));
		VersusCalc participants;
		participants.SetSlotDevice(0, 4);
		for (int slot = 1; slot < FIGHTER_COUNT; ++slot) participants.SetSlotDevice(slot, VERSUS_DEVICE_REMOTE);
		EXPECT_EQ(4, participants.GetHumanCount());
		EXPECT_EQ(VERSUS_DEVICE_REMOTE, participants.GetDevice(3));
	}

	TEST(SessionCalcTest, Messages_RoundTripAndSeparateControlFromInput)
	{
		SessionNetwork network(2, 6);
		network.Capture();
		for (auto message : { network.Nodes[0].MakeControl(1), network.Nodes[1].MakeInputs() }) {
			message.Hold = 1234;
			std::vector<uint8_t> bytes;
			EXPECT_TRUE(SessionCalc::Encode(message, bytes));
			SESSION_MESSAGE decoded;
			EXPECT_TRUE(SessionCalc::Decode(bytes.data(), bytes.size(), decoded));
			EXPECT_EQ(message.Nonce, decoded.Nonce);
			EXPECT_EQ(message.Hold, decoded.Hold);
			EXPECT_EQ(message.Session, decoded.Session);
			EXPECT_EQ(message.Round, decoded.Round);
			EXPECT_EQ(message.Slot, decoded.Slot);
			EXPECT_EQ(6, decoded.Delay);
			EXPECT_EQ(static_cast<int>(message.Type), static_cast<int>(decoded.Type));
			EXPECT_EQ(message.Running, decoded.Running);
			LOCKSTEP_PACKET wrong;
			EXPECT_FALSE(LockstepCalc::DecodePacket(bytes.data(), bytes.size(), wrong));
			EXPECT_FALSE(SessionCalc::Decode(bytes.data(), bytes.size() - 1, decoded));
			bytes.push_back(0);
			EXPECT_FALSE(SessionCalc::Decode(bytes.data(), bytes.size(), decoded));
		}
	}

	// JOIN と機器選択で自分のキャラを送り、開始通知で全員のキャラとホストのルール・種を受け取る。
	TEST(SessionCalcTest, State_SharesLobbyCharactersAndHostRuleAndSeed)
	{
		SessionCalc host, guest;
		constexpr uint32_t hostNonce = 0x12345678;
		host.Initialize({ NET_MODE_HOST }, hostNonce, CHARACTER_MEKA, GAME_RULE_GAME);
		guest.Initialize({ NET_MODE_JOIN }, 101, CHARACTER_MEKAGIRL, GAME_RULE_DEBUG);

		const auto join = guest.MakeControl();
		std::vector<uint8_t> bytes;
		ASSERT_TRUE(SessionCalc::Encode(join, bytes));
		SESSION_MESSAGE decoded;
		ASSERT_TRUE(SessionCalc::Decode(bytes.data(), bytes.size(), decoded));
		const int slot = host.Join(decoded.Nonce, decoded.Version, decoded.Build, decoded.Program,
			static_cast<CHARACTER_TYPE>(decoded.Characters[decoded.Slot]));
		ASSERT_EQ(1, slot);
		EXPECT_EQ(CHARACTER_MEKAGIRL, host.GetCharacter(slot));

		Deliver(guest, 0, host.MakeControl(slot));
		EXPECT_EQ(CHARACTER_MEKAGIRL, guest.GetCharacter(slot));
		guest.Decide();
		const auto ready = guest.MakeControl();
		EXPECT_EQ(SESSION_READY, ready.Type);
		EXPECT_EQ(CHARACTER_MEKAGIRL, ready.Characters[slot]);
		Deliver(host, slot, ready);
		host.Decide();
		host.Decide();
		Deliver(guest, 0, host.MakeControl(slot));
		EXPECT_TRUE(guest.IsRunning());
		EXPECT_EQ(CHARACTER_MEKA, guest.GetCharacter(0));
		EXPECT_EQ(CHARACTER_MEKAGIRL, guest.GetCharacter(slot));
		EXPECT_EQ(GAME_RULE_GAME, guest.GetRule());
		EXPECT_EQ(hostNonce, guest.GetSeed());
	}

	// ロビーのホストが選んだ7項目はバイト列を通して届き、参加側は上書きできない。
	TEST(SessionCalcTest, State_SharesHostTuningAndLobbyChanges)
	{
		SessionCalc host, guest;
		host.Initialize({ NET_MODE_HOST }, 100);
		guest.Initialize({ NET_MODE_JOIN }, 101);
		ASSERT_EQ(1, host.Join(101));
		MATCH_TUNING tuning = TuningCalcDefault();
		for (int i = 0; i < TUNING_ITEM_COUNT; ++i) {
			tuning.Levels[i] = static_cast<uint8_t>((tuning.Levels[i] + 1) % TuningCalcLevelCount(static_cast<TUNING_ITEM>(i)));
		}
		host.SetTuning(tuning);
		Deliver(guest, 0, host.MakeControl(1));
		EXPECT_EQ(tuning.Levels, guest.GetTuning().Levels);
		guest.SetTuning(TuningCalcDefault());
		EXPECT_EQ(tuning.Levels, guest.GetTuning().Levels);

		tuning.Levels[TUNING_ARM_USES] = 4; // ロビー中にもう一度変更し、無制限の段も配る。
		host.SetTuning(tuning);
		Deliver(guest, 0, host.MakeControl(1));
		EXPECT_EQ(tuning.Levels, guest.GetTuning().Levels);
		EXPECT_TRUE(TuningCalcIsUnlimitedUses(guest.GetTuning()));

		guest.Decide();
		Deliver(host, 1, guest.MakeControl());
		host.Decide();
		host.Decide();
		Deliver(guest, 0, host.MakeControl(1));
		ASSERT_TRUE(host.IsRunning());
		ASSERT_TRUE(guest.IsRunning());
		EXPECT_EQ(tuning.Levels, guest.GetTuning().Levels);
	}

	// 開始後のホストは変更できず、ロビーへ戻ると次の試合の設定を変更できる。
	TEST(SessionCalcTest, TuningStaysFixedUntilReturningToLobby)
	{
		SessionNetwork network(2, 0);
		auto& host = network.Nodes[0];
		auto& guest = network.Nodes[1];
		ASSERT_TRUE(host.IsRunning());
		ASSERT_TRUE(guest.IsRunning());
		const MATCH_TUNING original = host.GetTuning();
		MATCH_TUNING changed = original;
		changed.Levels[TUNING_DAMAGE] = 5;
		host.SetTuning(changed);
		EXPECT_EQ(original.Levels, host.GetTuning().Levels);

		host.ReturnToLobby();
		guest.ReturnToLobby();
		host.SetTuning(changed);
		Deliver(guest, 0, host.MakeControl(1));
		EXPECT_EQ(changed.Levels, host.GetTuning().Levels);
		EXPECT_EQ(changed.Levels, guest.GetTuning().Levels);
	}

	// 受信したSTATEの段が範囲外なら、その通知を受け付けない。
	TEST(SessionCalcTest, State_RejectsInvalidTuningLevel)
	{
		SessionCalc host;
		host.Initialize({ NET_MODE_HOST }, 100);
		ASSERT_EQ(1, host.Join(101));
		std::vector<uint8_t> bytes;
		ASSERT_TRUE(SessionCalc::Encode(host.MakeControl(1), bytes));
		bytes.back() = static_cast<uint8_t>(TuningCalcLevelCount(TUNING_ARM_USES));
		SESSION_MESSAGE decoded;
		EXPECT_FALSE(SessionCalc::Decode(bytes.data(), bytes.size(), decoded));
	}

	// STATE のルールが範囲外なら受け付けず、ルール欄のない旧版には版の不一致を返せる。
	TEST(SessionCalcTest, State_RejectsInvalidRuleAndRecognizesPreviousVersion)
	{
		SessionCalc host;
		host.Initialize({ NET_MODE_HOST }, 100, CHARACTER_DEFAULT, GAME_RULE_GAME);
		ASSERT_EQ(1, host.Join(101));
		std::vector<uint8_t> bytes;
		ASSERT_TRUE(SessionCalc::Encode(host.MakeControl(1), bytes));
		const size_t ruleOffset = bytes.size() - TUNING_ITEM_COUNT - sizeof(uint32_t) - 1;
		bytes[ruleOffset] = GAME_RULE_COUNT;
		SESSION_MESSAGE decoded;
		EXPECT_FALSE(SessionCalc::Decode(bytes.data(), bytes.size(), decoded));

		bytes.resize(ruleOffset);
		EXPECT_FALSE(SessionCalc::Decode(bytes.data(), bytes.size(), decoded));
		bytes[4] = 7; // 版7の STATE はキャラ番号まで。版は先頭の識別子4バイトの直後。
		ASSERT_TRUE(SessionCalc::Decode(bytes.data(), bytes.size(), decoded));
		EXPECT_EQ(SESSION_FAILURE_VERSION, SessionCalc::CheckCompatibility(decoded));
	}

	TEST(SessionCalcTest, Inputs_CompleteThroughWaitsForEveryPlayer)
	{
		SessionNetwork network(3, 0);
		auto& host = network.Nodes[0];
		EXPECT_EQ(-1, host.GetCompleteThrough());
		network.Capture();
		EXPECT_EQ(-1, host.GetCompleteThrough());
		Deliver(host, 1, network.Nodes[1].MakeInputs());
		EXPECT_EQ(-1, host.GetCompleteThrough());
		Deliver(host, 2, network.Nodes[2].MakeInputs());
		EXPECT_EQ(0, host.GetCompleteThrough());
	}

	TEST(SessionCalcTest, Inputs_FixedWhileWaitingAndRequireHostEcho)
	{
		SessionNetwork network(2, 0);
		network.Capture();
		auto& host = network.Nodes[0];
		auto& client = network.Nodes[1];
		LOCKSTEP_FRAME frame;
		EXPECT_FALSE(client.TakeFrame(frame));
		EXPECT_FALSE(client.NeedsInput());
		client.CaptureInput({}); // 待機中の採り直しでは入力を変えない。
		const auto original = client.MakeInputs();
		EXPECT_EQ(1, original.Packet.Count);
		EXPECT_EQ(static_cast<int>(MakeInput(1, 0).MoveX), static_cast<int>(original.Packet.Frames[0].Inputs[1].MoveX));
		auto wrongSlot = original;
		wrongSlot.Packet.Mask = 1;
		wrongSlot.Packet.Frames[0].Mask = 1;
		Deliver(host, 1, wrongSlot);
		EXPECT_FALSE(host.TakeFrame(frame));
		Deliver(host, 1, original);
		EXPECT_TRUE(host.TakeFrame(frame));
		EXPECT_FALSE(client.TakeFrame(frame)); // 自分で保管していても返送前には進まない。
		Deliver(client, 0, host.MakeInputs(1));
		EXPECT_TRUE(client.TakeFrame(frame));
		EXPECT_EQ(0, frame.Number);
		EXPECT_EQ(static_cast<int>(MakeInput(1, 0).MoveX), static_cast<int>(frame.Inputs[1].MoveX));
	}

	TEST(SessionCalcTest, FourPlayers_ResendLossAndKeepFramesUntilEveryPeerAcknowledges)
	{
		for (int delay : { 0, LOCKSTEP_DEFAULT_DELAY, 6 }) {
			SessionNetwork network(4, delay);
			for (int tick = 0; tick < LOCKSTEP_HISTORY * 3; ++tick) {
				network.Capture();
				network.Exchange(tick % 7 == 0 ? 2 : -1, tick % 11 == 0 ? 1 : -1);
				for (auto& node : network.Nodes) {
					LOCKSTEP_FRAME frame;
					if (!node.TakeFrame(frame)) continue;
					for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
						const PLAYER_INPUT expected = frame.Number < delay ? PLAYER_INPUT{} : MakeInput(slot, frame.Number - delay);
						EXPECT_EQ(static_cast<int>(expected.MoveX), static_cast<int>(frame.Inputs[slot].MoveX));
						EXPECT_EQ(static_cast<int>(expected.Buttons), static_cast<int>(frame.Inputs[slot].Buttons));
						EXPECT_EQ(expected.Equip.Number, frame.Inputs[slot].Equip.Number);
					}
				}
			}
			for (auto& node : network.Nodes) EXPECT_TRUE(node.GetNextFrame() > LOCKSTEP_HISTORY * 2);
		}
		// 2PからだけACKが届いても、3Pへ再送する同じ履歴を消さない。
		SessionNetwork network(3, 0);
		network.Capture();
		network.Exchange();
		LOCKSTEP_FRAME frame;
		for (int slot = 0; slot < 3; ++slot) EXPECT_TRUE(network.Nodes[slot].TakeFrame(frame));
		Deliver(network.Nodes[0], 1, network.Nodes[1].MakeInputs());
		const auto retained = network.Nodes[0].MakeInputs(2);
		EXPECT_EQ(1, retained.Packet.Count);
		EXPECT_EQ(0, retained.Packet.Frames[0].Number);
	}

	TEST(SessionCalcTest, Title_RetransmitsLastFrameAndRejectsOldRoundAfterRestart)
	{
		SessionNetwork network(2, 0);
		auto& host = network.Nodes[0];
		auto& client = network.Nodes[1];
		network.Capture();
		Deliver(host, 1, client.MakeInputs());
		LOCKSTEP_FRAME frame;
		EXPECT_TRUE(host.TakeFrame(frame));
		const auto oldStart = host.MakeControl(1);
		host.ReturnToLobby();
		host.Decide();
		EXPECT_FALSE(host.IsRunning()); // 最終入力が未受領なので次の試合へ進めない。
		const auto finalInput = host.MakeInputs(1);
		Deliver(client, 0, host.MakeControl(1)); // 先にロビー通知が来ても実行を止めない。
		EXPECT_TRUE(client.IsRunning());
		Deliver(client, 0, finalInput);
		EXPECT_TRUE(client.TakeFrame(frame));
		client.ReturnToLobby();
		Deliver(client, 0, oldStart);
		EXPECT_FALSE(client.IsRunning());
		Deliver(host, 1, client.MakeInputs());
		host.Decide();
		EXPECT_TRUE(host.IsRunning());
		Deliver(client, 0, host.MakeControl(1));
		EXPECT_TRUE(client.IsRunning());
		EXPECT_EQ(0, client.GetNextFrame());
		Deliver(client, 0, finalInput); // 前の試合の入力は新しい0フレームを埋めない。
		client.CaptureInput({});
		EXPECT_FALSE(client.TakeFrame(frame));
	}

	TEST(SessionCalcTest, Checksum_WaitsForSameFrameResendsAndDetectsEachStateField)
	{
		// 位置・%・KOをそれぞれ変え、ホスト側と参加側の両方でずれを検出する。
		for (int changed = 0; changed < 3; ++changed) {
			SessionNetwork network(2, 0);
			auto& host = network.Nodes[0];
			auto& client = network.Nodes[1];
			LOCKSTEP_STATE states[FIGHTER_COUNT] = {};
			host.RecordChecksum(58, states);
			EXPECT_EQ(-1, host.MakeControl(1).CheckFrame);
			host.RecordChecksum(59, states);
			const auto first = host.MakeControl(1);
			EXPECT_EQ(59, first.CheckFrame);
			// 次のチェック時刻まで最初のパケットが失われても、古い分を送り直す。
			host.RecordChecksum(119, states);
			EXPECT_EQ(59, host.MakeControl(1).CheckFrame);
			Deliver(client, 0, first); // 自分がまだそのフレームへ到達していない。
			EXPECT_EQ(-1, client.GetDesyncFrame());
			client.RecordChecksum(59, states);
			Deliver(host, 1, client.MakeControl());
			EXPECT_EQ(-1, host.GetDesyncFrame());
			EXPECT_EQ(119, host.MakeControl(1).CheckFrame);
			if (changed == 0) states[3].Position.Z = 2.0f; // ダミーも比較する。
			if (changed == 1) states[2].Percent = 12.0f;
			if (changed == 2) states[0].Score = 1;
			client.RecordChecksum(119, states);
			Deliver(client, 0, host.MakeControl(1));
			Deliver(host, 1, client.MakeControl());
			EXPECT_EQ(119, client.GetDesyncFrame());
			EXPECT_EQ(119, host.GetDesyncFrame());
			EXPECT_EQ(1, host.GetDesyncPeer());
		}
	}

	TEST(SessionCalcTest, Disconnect_HostFillsMissingInputAndKeepsSurvivorInSync)
	{
		SessionNetwork network(3, 0);
		auto& host = network.Nodes[0];
		auto& survivor = network.Nodes[2];
		network.Capture();
		network.Exchange();
		const auto late = network.Nodes[1].MakeInputs();
		host.SetTime(2999);
		Deliver(host, 2, survivor.MakeControl());
		host.CheckTimeouts();
		EXPECT_EQ(0, static_cast<int>(host.GetDisconnectedMask()));
		host.SetTime(3000);
		host.CheckTimeouts();
		EXPECT_EQ(2, static_cast<int>(host.GetDisconnectedMask()));
		EXPECT_EQ(7, static_cast<int>(host.GetMask())); // 人の枠はダミーへ変えない。
		LOCKSTEP_FRAME a, b;
		EXPECT_TRUE(host.TakeFrame(a));
		EXPECT_EQ(static_cast<int>(MakeInput(1, 0).MoveX), static_cast<int>(a.Inputs[1].MoveX)); // 配布済みは保持。
		EXPECT_TRUE(survivor.TakeFrame(b));
		for (int frame = 1; frame < LOCKSTEP_HISTORY * 2; ++frame) {
			host.CaptureInput(MakeInput(0, frame));
			survivor.CaptureInput(MakeInput(2, frame));
			Deliver(host, 2, survivor.MakeInputs());
			Deliver(host, 1, late); // タイムアウトした人から遅れて届いても復帰させない。
			EXPECT_TRUE(host.TakeFrame(a));
			Deliver(survivor, 0, host.MakeInputs(2));
			EXPECT_TRUE(survivor.TakeFrame(b));
			EXPECT_EQ(frame, b.Number);
			EXPECT_EQ(0, static_cast<int>(b.Inputs[1].MoveX));
			EXPECT_EQ(0, static_cast<int>(b.Inputs[1].Buttons));
			EXPECT_EQ(-1, b.Inputs[1].Equip.Number);
			EXPECT_EQ(static_cast<int>(a.Inputs[2].MoveX), static_cast<int>(b.Inputs[2].MoveX));
		}
	}

	TEST(SessionCalcTest, Disconnect_ClientUsesHeartbeatAndStopsAfterThreeSeconds)
	{
		SessionNetwork network(2, 3);
		auto& client = network.Nodes[1];
		client.SetTime(2000);
		Deliver(client, 0, network.Nodes[0].MakeControl(1)); // 対戦中のSTATEも生存確認。
		client.SetTime(4999);
		client.CheckTimeouts();
		EXPECT_TRUE(client.IsRunning());
		client.SetTime(5000);
		client.CheckTimeouts();
		EXPECT_FALSE(client.IsRunning());
		EXPECT_EQ(static_cast<int>(SESSION_FAILURE_TIMEOUT), static_cast<int>(client.GetFailure()));
	}

	TEST(SessionCalcTest, Disconnect_ClientWaitsForHostStartedLater)
	{
		NET_OPTIONS options;
		options.Mode = NET_MODE_JOIN;
		SessionCalc client;
		client.Initialize(options, 200);
		client.SetTime(10000); // ホストがまだ起動していない。
		client.CheckTimeouts();
		EXPECT_EQ(static_cast<int>(SESSION_FAILURE_NONE), static_cast<int>(client.GetFailure()));
	}

	TEST(SessionCalcTest, Compatibility_RefusesVersionBuildOrProgramBeforeAssigningSlot)
	{
		for (const SESSION_FAILURE expected : { SESSION_FAILURE_VERSION, SESSION_FAILURE_BUILD, SESSION_FAILURE_PROGRAM }) {
			NET_OPTIONS options;
			options.Mode = NET_MODE_HOST;
			SessionCalc host, client;
			host.Initialize(options, 100);
			options.Mode = NET_MODE_JOIN;
			client.Initialize(options, 101);
			auto request = client.MakeControl();
			if (expected == SESSION_FAILURE_VERSION) ++request.Version;
			else if (expected == SESSION_FAILURE_BUILD) request.Build = LOCKSTEP_CURRENT_BUILD == LOCKSTEP_BUILD_DEBUG ? LOCKSTEP_BUILD_RELEASE : LOCKSTEP_BUILD_DEBUG;
			else request.Program = SessionCalcGetProgram() + 1; // ソースが違う exe
			EXPECT_EQ(-1, host.Join(request.Nonce, request.Version, request.Build, request.Program));
			EXPECT_EQ(1, static_cast<int>(host.GetMask()));
			// 版が違う制御通知も読めるので、タイムアウトを待たず理由を返せる。
			std::vector<uint8_t> bytes;
			EXPECT_TRUE(SessionCalc::Encode(request, bytes));
			SESSION_MESSAGE decoded;
			EXPECT_TRUE(SessionCalc::Decode(bytes.data(), bytes.size(), decoded));
			EXPECT_EQ(static_cast<int>(expected), static_cast<int>(SessionCalc::CheckCompatibility(decoded)));
			auto rejection = host.MakeControl();
			rejection.Type = SESSION_REJECT;
			rejection.Nonce = 101;
			rejection.Version = request.Version;
			rejection.Build = request.Build;
			rejection.Program = request.Program;
			Deliver(client, 0, rejection);
			EXPECT_EQ(static_cast<int>(expected), static_cast<int>(client.GetFailure()));
			EXPECT_EQ(-1, client.GetLocalSlot());
		}
	}

	TEST(SessionCalcTest, Compatibility_AcceptsDifferentCompilerAndReportsIt)
	{
		// VS の版が違う exe は断らずにつなぎ、両方に知らせる。
		SessionNetwork network(2, 0);
		auto& host = network.Nodes[0];
		auto& client = network.Nodes[1];
		auto request = client.MakeControl();
		request.Compiler = SESSION_COMPILER + 1;
		EXPECT_EQ(static_cast<int>(SESSION_FAILURE_NONE), static_cast<int>(SessionCalc::CheckCompatibility(request)));
		Deliver(host, 1, request);
		EXPECT_TRUE(host.HasCompilerDifference());
		auto state = host.MakeControl(1);
		state.Compiler = SESSION_COMPILER + 1;
		Deliver(client, 0, state);
		EXPECT_TRUE(client.HasCompilerDifference());
		EXPECT_EQ(1, client.GetLocalSlot());
	}

	TEST(SessionCalcTest, Monitor_SubtractsReplyHoldFromPing)
	{
		SessionNetwork network(2, 0);
		auto& host = network.Nodes[0];
		auto& client = network.Nodes[1];
		host.SetTime(100);
		client.SetTime(1000);
		Deliver(client, 0, host.MakeControl(1));
		client.SetTime(1015);
		const auto reply = client.MakeControl();
		EXPECT_EQ(uint32_t(15), reply.Hold);
		host.SetTime(140);
		Deliver(host, 1, reply);
		EXPECT_EQ(25, host.GetPing(1));
		EXPECT_EQ(25, host.GetAutoDelayPing());
		auto invalid = reply;
		invalid.Echo = 130;
		invalid.Hold = 11; // 経った10msより長い待ちは測らない。
		Deliver(host, 1, invalid);
		EXPECT_EQ(25, host.GetPing(1));
	}

	TEST(SessionCalcTest, Monitor_LateOldStampDoesNotReplaceEcho)
	{
		SessionNetwork network(2, 0);
		auto& host = network.Nodes[0];
		auto& client = network.Nodes[1];
		host.SetTime(100);
		const auto older = host.MakeControl(1);
		host.SetTime(110);
		const auto newer = host.MakeControl(1);
		client.SetTime(1000);
		Deliver(client, 0, newer);
		client.SetTime(1005);
		Deliver(client, 0, older); // 順番が入れ替わって遅れて届いた。
		client.SetTime(1015);
		const auto reply = client.MakeControl();
		EXPECT_EQ(uint32_t(110), reply.Echo);
		EXPECT_EQ(uint32_t(15), reply.Hold);
	}

	TEST(SessionCalcTest, Monitor_RoundTripPingAndCountsOnlyRunningInputWaits)
	{
		SessionNetwork network(2, 0);
		auto& host = network.Nodes[0];
		auto& client = network.Nodes[1];
		host.SetTime(100);
		client.SetTime(1000); // PCどうしの時計の原点は違ってよい。
		Deliver(client, 0, host.MakeControl(1));
		host.SetTime(140);
		Deliver(host, 1, client.MakeControl());
		EXPECT_EQ(40, host.GetPing(1));
		client.SetTime(1040);
		Deliver(client, 0, host.MakeControl(1));
		EXPECT_EQ(40, client.GetPing(0));
		network.Capture();
		LOCKSTEP_FRAME frame;
		EXPECT_FALSE(host.TakeFrame(frame));
		EXPECT_FALSE(host.TakeFrame(frame));
		EXPECT_EQ(uint64_t(2), host.GetWaitFrames());
		network.Exchange();
		EXPECT_TRUE(host.TakeFrame(frame));
		EXPECT_EQ(uint64_t(2), host.GetWaitFrames());
		host.ReturnToLobby();
		EXPECT_FALSE(host.TakeFrame(frame));
		EXPECT_EQ(uint64_t(2), host.GetWaitFrames());
	}

	TEST(SessionCalcTest, AutoDelay_HostUsesPingOfLastTwoSecondsAtRoundStart)
	{
		EXPECT_EQ(4, SessionCalcAutoDelay(-1));
		EXPECT_EQ(4, SessionCalcAutoDelay(5));
		EXPECT_EQ(6, SessionCalcAutoDelay(60));
		EXPECT_EQ(8, SessionCalcAutoDelay(100));
		EXPECT_EQ(AUTO_DELAY_MAX, SessionCalcAutoDelay(1000));
		for (const int fixedDelay : { LOCKSTEP_AUTO_DELAY, 4 }) {
			NET_OPTIONS options;
			options.Mode = NET_MODE_HOST;
			options.Delay = fixedDelay;
			SessionCalc host, client;
			host.Initialize(options, 100);
			options.Mode = NET_MODE_JOIN;
			client.Initialize(options, 101);
			EXPECT_EQ(1, host.Join(101));
			// ホストの送った時刻が、ping ミリ秒後に参加側から返ってくる。
			const auto measure = [&](uint32_t sent, int ping) {
				host.SetTime(sent);
				Deliver(client, 0, host.MakeControl(1));
				host.SetTime(sent + ping);
				Deliver(host, 1, client.MakeControl());
			};
			measure(100, 200);
			EXPECT_EQ(200, host.GetAutoDelayPing());
			measure(3000, 60); // 2秒より前の 200ms は使わない。
			EXPECT_EQ(60, host.GetAutoDelayPing());
			client.Decide();
			Deliver(host, 1, client.MakeControl());
			host.Decide();
			host.Decide();
			EXPECT_TRUE(host.IsRunning());
			const int expected = fixedDelay == LOCKSTEP_AUTO_DELAY ? 6 : 4;
			EXPECT_EQ(fixedDelay == LOCKSTEP_AUTO_DELAY, host.IsAutoDelay());
			EXPECT_EQ(expected, host.GetDelay());
			Deliver(client, 0, host.MakeControl(1));
			EXPECT_TRUE(client.IsRunning());
			EXPECT_EQ(expected, client.GetDelay());
		}
	}

	TEST(SessionCalcTest, AutoDelay_IgnoresRareSpikeAndRepeatedEcho)
	{
		NET_OPTIONS options;
		options.Mode = NET_MODE_HOST;
		SessionCalc host, client;
		host.Initialize(options, 100);
		options.Mode = NET_MODE_JOIN;
		client.Initialize(options, 101);
		EXPECT_EQ(1, host.Join(101));
		// 同じ Echo がもう一度届いても、2回目（80ms 後）は測らない。
		host.SetTime(100);
		Deliver(client, 0, host.MakeControl(1));
		host.SetTime(130);
		Deliver(host, 1, client.MakeControl());
		host.SetTime(180);
		Deliver(host, 1, client.MakeControl());
		EXPECT_EQ(30, host.GetAutoDelayPing());
		// 30ms が19回と 100ms が1回なら、一瞬の 100ms は使わない。
		for (int i = 1; i < 20; ++i) {
			const uint32_t sent = 200 + i * 50;
			const int ping = i == 10 ? 100 : 30;
			host.SetTime(sent);
			Deliver(client, 0, host.MakeControl(1));
			host.SetTime(sent + ping);
			Deliver(host, 1, client.MakeControl());
		}
		EXPECT_EQ(30, host.GetAutoDelayPing());
		client.Decide();
		Deliver(host, 1, client.MakeControl());
		host.Decide();
		host.Decide();
		EXPECT_TRUE(host.IsRunning());
		EXPECT_EQ(4, host.GetDelay());
	}

	TEST(SessionCalcTest, Disconnect_LobbyReleasesSlotBeforeRosterIsLocked)
	{
		NET_OPTIONS options;
		options.Mode = NET_MODE_HOST;
		SessionCalc host;
		host.Initialize(options, 100);
		EXPECT_EQ(1, host.Join(101));
		host.SetTime(3000);
		host.CheckTimeouts();
		EXPECT_EQ(1, static_cast<int>(host.GetMask()));
		EXPECT_EQ(1, host.Join(102));
	}
}
