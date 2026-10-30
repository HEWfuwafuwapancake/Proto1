#include "CppUnitTest.h"
#include "net/sessioncalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	// 実際に送るバイト列を通し、ソケットなしでホストと参加者をつなぐ。
	static void Deliver(SessionCalc& receiver, int peer, const SESSION_MESSAGE& message)
	{
		std::vector<uint8_t> bytes;
		Assert::IsTrue(SessionCalc::Encode(message, bytes));
		SESSION_MESSAGE decoded;
		Assert::IsTrue(SessionCalc::Decode(bytes.data(), bytes.size(), decoded));
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
				Assert::AreEqual(slot, Nodes[0].Join(100 + slot));
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

	TEST_CLASS(SessionCalcTest)
	{
	public:
		TEST_METHOD(Address_KeyboardEditsLanPrefixAndRejectsIncompleteOrOutOfRange)
		{
			SessionAddressCalc address;
			address.Initialize("192.168.1.10");
			Assert::AreEqual(std::string("192.168.1."), address.GetText());
			Assert::IsFalse(address.IsValid());
			address.AppendDigit(2);
			address.AppendDigit(5);
			address.AppendDigit(6);
			Assert::IsFalse(address.IsValid());
			Assert::IsTrue(address.Erase());
			address.AppendDigit(4);
			Assert::IsTrue(address.IsValid());
			Assert::AreEqual(std::string("192.168.1.254"), address.GetText());
			NET_OPTIONS options;
			Assert::IsTrue(SessionCalcParseOptions("--join " + address.GetText(), options));
			while (address.Erase()) {} // 空になった次の Backspace は戻る。
			Assert::AreEqual(std::string("..."), address.GetText());
			for (char c : std::string("127.0.0.1")) {
				if (c == '.') address.Move(1);
				else address.AppendDigit(c - '0');
			}
			Assert::AreEqual(std::string("127.0.0.1"), address.GetText());
			Assert::IsTrue(address.IsValid());
		}

		TEST_METHOD(Address_DpadMovesAndChangesSelectedPart)
		{
			SessionAddressCalc address;
			address.Initialize("192.168.1.10");
			address.Adjust(1);
			address.Adjust(1);
			Assert::AreEqual(std::string("192.168.1.1"), address.GetText());
			address.Move(-1);
			Assert::AreEqual(2, address.GetCursor());
			address.Adjust(1);
			Assert::AreEqual(std::string("192.168.2.1"), address.GetText());
			address.Adjust(-1);
			Assert::AreEqual(std::string("192.168.1.1"), address.GetText());
			Assert::IsTrue(address.IsValid());
		}

		TEST_METHOD(Options_LocalHostJoinAndDelay)
		{
			NET_OPTIONS options;
			Assert::IsTrue(SessionCalcParseOptions("", options));
			Assert::AreEqual(static_cast<int>(NET_MODE_LOCAL), static_cast<int>(options.Mode));
			Assert::IsTrue(SessionCalcParseOptions("--host", options));
			Assert::AreEqual(3, options.Delay);
			Assert::IsTrue(SessionCalcParseOptions("--delay 6 --host", options));
			Assert::AreEqual(6, options.Delay);
			Assert::IsTrue(SessionCalcParseOptions("--join 100.64.1.2 --delay 7", options));
			Assert::AreEqual(std::string("100.64.1.2"), options.Host);
			Assert::AreEqual(static_cast<int>(NET_MODE_JOIN), static_cast<int>(options.Mode));
			for (const char* invalid : { "--host --join 127.0.0.1", "--join", "--delay 3", "--host --delay 121",
				"--host --delay 3bad", "--join 1.2.3.999", "--unknown" }) {
				Assert::IsFalse(SessionCalcParseOptions(invalid, options));
			}
			Assert::AreEqual(7, options.Delay); // 失敗時に有効な設定を壊さない。
		}

		TEST_METHOD(Lobby_WaitsForDevicesAssignsFourSlotsAndLocksRoster)
		{
			NET_OPTIONS options;
			options.Mode = NET_MODE_HOST;
			options.Delay = 6;
			SessionCalc host, client;
			host.Initialize(options, 100);
			options.Mode = NET_MODE_JOIN;
			client.Initialize(options, 101);
			Assert::AreEqual(1, host.Join(101));
			Assert::AreEqual(1, host.Join(101)); // 再要求は同じ枠。
			host.Decide();
			host.Decide();
			Assert::IsFalse(host.IsRunning());
			Deliver(client, 0, host.MakeControl(1));
			Assert::AreEqual(1, client.GetLocalSlot());
			Assert::AreEqual(6, client.GetDelay());
			client.Decide();
			client.Decide();
			Assert::IsFalse(client.IsRunning()); // 参加側は開始できない。
			Deliver(host, 1, client.MakeControl());
			host.Decide();
			Assert::IsTrue(host.IsRunning());
			Deliver(client, 0, host.MakeControl(1));
			Assert::IsTrue(client.IsRunning());
			Assert::AreEqual(-1, host.Join(102));
			host.ReturnToLobby();
			Assert::AreEqual(-1, host.Join(102));
			Assert::AreEqual(3, static_cast<int>(host.GetReadyMask()));
			SessionNetwork four(4, 3);
			Assert::AreEqual(15, static_cast<int>(four.Nodes[0].GetMask()));
			Assert::AreEqual(-1, four.Nodes[0].Join(999));
			VersusCalc participants;
			participants.SetSlotDevice(0, 4);
			for (int slot = 1; slot < FIGHTER_COUNT; ++slot) participants.SetSlotDevice(slot, VERSUS_DEVICE_REMOTE);
			Assert::AreEqual(4, participants.GetHumanCount());
			Assert::AreEqual(VERSUS_DEVICE_REMOTE, participants.GetDevice(3));
		}

		TEST_METHOD(Messages_RoundTripAndSeparateControlFromInput)
		{
			SessionNetwork network(2, 6);
			network.Capture();
			for (auto message : { network.Nodes[0].MakeControl(1), network.Nodes[1].MakeInputs() }) {
				std::vector<uint8_t> bytes;
				Assert::IsTrue(SessionCalc::Encode(message, bytes));
				SESSION_MESSAGE decoded;
				Assert::IsTrue(SessionCalc::Decode(bytes.data(), bytes.size(), decoded));
				Assert::AreEqual(message.Nonce, decoded.Nonce);
				Assert::AreEqual(message.Session, decoded.Session);
				Assert::AreEqual(message.Round, decoded.Round);
				Assert::AreEqual(message.Slot, decoded.Slot);
				Assert::AreEqual(6, decoded.Delay);
				Assert::AreEqual(static_cast<int>(message.Type), static_cast<int>(decoded.Type));
				Assert::AreEqual(message.Running, decoded.Running);
				LOCKSTEP_PACKET wrong;
				Assert::IsFalse(LockstepCalc::DecodePacket(bytes.data(), bytes.size(), wrong));
				Assert::IsFalse(SessionCalc::Decode(bytes.data(), bytes.size() - 1, decoded));
				bytes.push_back(0);
				Assert::IsFalse(SessionCalc::Decode(bytes.data(), bytes.size(), decoded));
			}
		}

		TEST_METHOD(Inputs_FixedWhileWaitingAndRequireHostEcho)
		{
			SessionNetwork network(2, 0);
			network.Capture();
			auto& host = network.Nodes[0];
			auto& client = network.Nodes[1];
			LOCKSTEP_FRAME frame;
			Assert::IsFalse(client.TakeFrame(frame));
			Assert::IsFalse(client.NeedsInput());
			client.CaptureInput({}); // 待機中の採り直しでは入力を変えない。
			const auto original = client.MakeInputs();
			Assert::AreEqual(1, original.Packet.Count);
			Assert::AreEqual(static_cast<int>(MakeInput(1, 0).MoveX), static_cast<int>(original.Packet.Frames[0].Inputs[1].MoveX));
			auto wrongSlot = original;
			wrongSlot.Packet.Mask = 1;
			wrongSlot.Packet.Frames[0].Mask = 1;
			Deliver(host, 1, wrongSlot);
			Assert::IsFalse(host.TakeFrame(frame));
			Deliver(host, 1, original);
			Assert::IsTrue(host.TakeFrame(frame));
			Assert::IsFalse(client.TakeFrame(frame)); // 自分で保管していても返送前には進まない。
			Deliver(client, 0, host.MakeInputs(1));
			Assert::IsTrue(client.TakeFrame(frame));
			Assert::AreEqual(0, frame.Number);
			Assert::AreEqual(static_cast<int>(MakeInput(1, 0).MoveX), static_cast<int>(frame.Inputs[1].MoveX));
		}

		TEST_METHOD(FourPlayers_ResendLossAndKeepFramesUntilEveryPeerAcknowledges)
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
							Assert::AreEqual(static_cast<int>(expected.MoveX), static_cast<int>(frame.Inputs[slot].MoveX));
							Assert::AreEqual(static_cast<int>(expected.Buttons), static_cast<int>(frame.Inputs[slot].Buttons));
							Assert::AreEqual(expected.Equip.Number, frame.Inputs[slot].Equip.Number);
						}
					}
				}
				for (auto& node : network.Nodes) Assert::IsTrue(node.GetNextFrame() > LOCKSTEP_HISTORY * 2);
			}
			// 2PからだけACKが届いても、3Pへ再送する同じ履歴を消さない。
			SessionNetwork network(3, 0);
			network.Capture();
			network.Exchange();
			LOCKSTEP_FRAME frame;
			for (int slot = 0; slot < 3; ++slot) Assert::IsTrue(network.Nodes[slot].TakeFrame(frame));
			Deliver(network.Nodes[0], 1, network.Nodes[1].MakeInputs());
			const auto retained = network.Nodes[0].MakeInputs(2);
			Assert::AreEqual(1, retained.Packet.Count);
			Assert::AreEqual(0, retained.Packet.Frames[0].Number);
		}

		TEST_METHOD(Title_RetransmitsLastFrameAndRejectsOldRoundAfterRestart)
		{
			SessionNetwork network(2, 0);
			auto& host = network.Nodes[0];
			auto& client = network.Nodes[1];
			network.Capture();
			Deliver(host, 1, client.MakeInputs());
			LOCKSTEP_FRAME frame;
			Assert::IsTrue(host.TakeFrame(frame));
			const auto oldStart = host.MakeControl(1);
			host.ReturnToLobby();
			host.Decide();
			Assert::IsFalse(host.IsRunning()); // 最終入力が未受領なので次の試合へ進めない。
			const auto finalInput = host.MakeInputs(1);
			Deliver(client, 0, host.MakeControl(1)); // 先にロビー通知が来ても実行を止めない。
			Assert::IsTrue(client.IsRunning());
			Deliver(client, 0, finalInput);
			Assert::IsTrue(client.TakeFrame(frame));
			client.ReturnToLobby();
			Deliver(client, 0, oldStart);
			Assert::IsFalse(client.IsRunning());
			Deliver(host, 1, client.MakeInputs());
			host.Decide();
			Assert::IsTrue(host.IsRunning());
			Deliver(client, 0, host.MakeControl(1));
			Assert::IsTrue(client.IsRunning());
			Assert::AreEqual(0, client.GetNextFrame());
			Deliver(client, 0, finalInput); // 前の試合の入力は新しい0フレームを埋めない。
			client.CaptureInput({});
			Assert::IsFalse(client.TakeFrame(frame));
		}

		TEST_METHOD(Checksum_WaitsForSameFrameResendsAndDetectsEachStateField)
		{
			// 位置・%・KOをそれぞれ変え、ホスト側と参加側の両方でずれを検出する。
			for (int changed = 0; changed < 3; ++changed) {
				SessionNetwork network(2, 0);
				auto& host = network.Nodes[0];
				auto& client = network.Nodes[1];
				LOCKSTEP_STATE states[FIGHTER_COUNT] = {};
				host.RecordChecksum(58, states);
				Assert::AreEqual(-1, host.MakeControl(1).CheckFrame);
				host.RecordChecksum(59, states);
				const auto first = host.MakeControl(1);
				Assert::AreEqual(59, first.CheckFrame);
				// 次のチェック時刻まで最初のパケットが失われても、古い分を送り直す。
				host.RecordChecksum(119, states);
				Assert::AreEqual(59, host.MakeControl(1).CheckFrame);
				Deliver(client, 0, first); // 自分がまだそのフレームへ到達していない。
				Assert::AreEqual(-1, client.GetDesyncFrame());
				client.RecordChecksum(59, states);
				Deliver(host, 1, client.MakeControl());
				Assert::AreEqual(-1, host.GetDesyncFrame());
				Assert::AreEqual(119, host.MakeControl(1).CheckFrame);
				if (changed == 0) states[3].Position.Z = 2.0f; // ダミーも比較する。
				if (changed == 1) states[2].Percent = 12.0f;
				if (changed == 2) states[0].Score = 1;
				client.RecordChecksum(119, states);
				Deliver(client, 0, host.MakeControl(1));
				Deliver(host, 1, client.MakeControl());
				Assert::AreEqual(119, client.GetDesyncFrame());
				Assert::AreEqual(119, host.GetDesyncFrame());
				Assert::AreEqual(1, host.GetDesyncPeer());
			}
		}

		TEST_METHOD(Disconnect_HostFillsMissingInputAndKeepsSurvivorInSync)
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
			Assert::AreEqual(0, static_cast<int>(host.GetDisconnectedMask()));
			host.SetTime(3000);
			host.CheckTimeouts();
			Assert::AreEqual(2, static_cast<int>(host.GetDisconnectedMask()));
			Assert::AreEqual(7, static_cast<int>(host.GetMask())); // 人の枠はダミーへ変えない。
			LOCKSTEP_FRAME a, b;
			Assert::IsTrue(host.TakeFrame(a));
			Assert::AreEqual(static_cast<int>(MakeInput(1, 0).MoveX), static_cast<int>(a.Inputs[1].MoveX)); // 配布済みは保持。
			Assert::IsTrue(survivor.TakeFrame(b));
			for (int frame = 1; frame < LOCKSTEP_HISTORY * 2; ++frame) {
				host.CaptureInput(MakeInput(0, frame));
				survivor.CaptureInput(MakeInput(2, frame));
				Deliver(host, 2, survivor.MakeInputs());
				Deliver(host, 1, late); // タイムアウトした人から遅れて届いても復帰させない。
				Assert::IsTrue(host.TakeFrame(a));
				Deliver(survivor, 0, host.MakeInputs(2));
				Assert::IsTrue(survivor.TakeFrame(b));
				Assert::AreEqual(frame, b.Number);
				Assert::AreEqual(0, static_cast<int>(b.Inputs[1].MoveX));
				Assert::AreEqual(0, static_cast<int>(b.Inputs[1].Buttons));
				Assert::AreEqual(-1, b.Inputs[1].Equip.Number);
				Assert::AreEqual(static_cast<int>(a.Inputs[2].MoveX), static_cast<int>(b.Inputs[2].MoveX));
			}
		}

		TEST_METHOD(Disconnect_ClientUsesHeartbeatAndStopsAfterThreeSeconds)
		{
			SessionNetwork network(2, 3);
			auto& client = network.Nodes[1];
			client.SetTime(2000);
			Deliver(client, 0, network.Nodes[0].MakeControl(1)); // 対戦中のSTATEも生存確認。
			client.SetTime(4999);
			client.CheckTimeouts();
			Assert::IsTrue(client.IsRunning());
			client.SetTime(5000);
			client.CheckTimeouts();
			Assert::IsFalse(client.IsRunning());
			Assert::AreEqual(static_cast<int>(SESSION_FAILURE_TIMEOUT), static_cast<int>(client.GetFailure()));
		}

		TEST_METHOD(Disconnect_ClientWaitsForHostStartedLater)
		{
			NET_OPTIONS options;
			options.Mode = NET_MODE_JOIN;
			SessionCalc client;
			client.Initialize(options, 200);
			client.SetTime(10000); // ホストがまだ起動していない。
			client.CheckTimeouts();
			Assert::AreEqual(static_cast<int>(SESSION_FAILURE_NONE), static_cast<int>(client.GetFailure()));
		}

		TEST_METHOD(Compatibility_RefusesVersionOrBuildBeforeAssigningSlot)
		{
			for (bool changeVersion : { true, false }) {
				NET_OPTIONS options;
				options.Mode = NET_MODE_HOST;
				SessionCalc host, client;
				host.Initialize(options, 100);
				options.Mode = NET_MODE_JOIN;
				client.Initialize(options, 101);
				auto request = client.MakeControl();
				if (changeVersion) ++request.Version;
				else request.Build = LOCKSTEP_CURRENT_BUILD == LOCKSTEP_BUILD_DEBUG ? LOCKSTEP_BUILD_RELEASE : LOCKSTEP_BUILD_DEBUG;
				Assert::AreEqual(-1, host.Join(request.Nonce, request.Version, request.Build));
				Assert::AreEqual(1, static_cast<int>(host.GetMask()));
				// 版が違う制御通知も読めるので、タイムアウトを待たず理由を返せる。
				std::vector<uint8_t> bytes;
				Assert::IsTrue(SessionCalc::Encode(request, bytes));
				SESSION_MESSAGE decoded;
				Assert::IsTrue(SessionCalc::Decode(bytes.data(), bytes.size(), decoded));
				const auto expected = changeVersion ? SESSION_FAILURE_VERSION : SESSION_FAILURE_BUILD;
				Assert::AreEqual(static_cast<int>(expected), static_cast<int>(SessionCalc::CheckCompatibility(decoded)));
				auto rejection = host.MakeControl();
				rejection.Type = SESSION_REJECT;
				rejection.Nonce = 101;
				rejection.Version = request.Version;
				rejection.Build = request.Build;
				Deliver(client, 0, rejection);
				Assert::AreEqual(static_cast<int>(expected), static_cast<int>(client.GetFailure()));
				Assert::AreEqual(-1, client.GetLocalSlot());
			}
		}

		TEST_METHOD(Monitor_RoundTripPingAndCountsOnlyRunningInputWaits)
		{
			SessionNetwork network(2, 0);
			auto& host = network.Nodes[0];
			auto& client = network.Nodes[1];
			host.SetTime(100);
			client.SetTime(1000); // PCどうしの時計の原点は違ってよい。
			Deliver(client, 0, host.MakeControl(1));
			host.SetTime(140);
			Deliver(host, 1, client.MakeControl());
			Assert::AreEqual(40, host.GetPing(1));
			client.SetTime(1040);
			Deliver(client, 0, host.MakeControl(1));
			Assert::AreEqual(40, client.GetPing(0));
			network.Capture();
			LOCKSTEP_FRAME frame;
			Assert::IsFalse(host.TakeFrame(frame));
			Assert::IsFalse(host.TakeFrame(frame));
			Assert::AreEqual(uint64_t(2), host.GetWaitFrames());
			network.Exchange();
			Assert::IsTrue(host.TakeFrame(frame));
			Assert::AreEqual(uint64_t(2), host.GetWaitFrames());
			host.ReturnToLobby();
			Assert::IsFalse(host.TakeFrame(frame));
			Assert::AreEqual(uint64_t(2), host.GetWaitFrames());
		}

		TEST_METHOD(Disconnect_LobbyReleasesSlotBeforeRosterIsLocked)
		{
			NET_OPTIONS options;
			options.Mode = NET_MODE_HOST;
			SessionCalc host;
			host.Initialize(options, 100);
			Assert::AreEqual(1, host.Join(101));
			host.SetTime(3000);
			host.CheckTimeouts();
			Assert::AreEqual(1, static_cast<int>(host.GetMask()));
			Assert::AreEqual(1, host.Join(102));
		}
	};
}
