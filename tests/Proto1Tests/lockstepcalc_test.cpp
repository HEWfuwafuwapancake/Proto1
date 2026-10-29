// ロックステップの計算だけを確かめる（ソケット・Windows・DirectXは不要）。
#include "CppUnitTest.h"
#include "net/lockstepcalc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Proto1Tests
{
	TEST_CLASS(LockstepCalcTest)
	{
	public:
		// ホストの遅延を共有し、先頭は無入力。全員が届くまで同じフレームで待つ。
		TEST_METHOD(Delay_WaitsForAllPlayers)
		{
			for (int delay : { LOCKSTEP_DEFAULT_DELAY, 6 }) {
				LockstepCalc calc;
				Assert::IsTrue(calc.Initialize(0x0B, delay)); // 0・1・3が人、2はダミー
				Assert::AreEqual(delay, calc.GetDelay());
				const auto input = ActionCalcPlayerInput({ 1.0f, 0.0f }, {}, 1u << ACTION_ARM_LEFT);
				Assert::IsTrue(calc.StoreLocalInput(0, input));
				LOCKSTEP_FRAME frame;
				Assert::IsTrue(calc.GetFrame(0, frame));
				Assert::AreEqual(0, static_cast<int>(frame.Inputs[0].Buttons));
				for (int i = 0; i < delay; ++i) Assert::IsTrue(calc.Advance());
				Assert::AreEqual(delay, calc.GetNextFrame());
				Assert::IsFalse(calc.IsReady());
				Assert::IsFalse(calc.Advance());
				Assert::IsTrue(calc.StoreInput(delay, 3, {}));
				Assert::IsFalse(calc.IsReady());
				Assert::IsTrue(calc.StoreInput(delay, 1, {}));
				Assert::IsTrue(calc.GetFrame(delay, frame));
				Assert::AreEqual(32767, static_cast<int>(frame.Inputs[0].MoveX));
				Assert::AreEqual(1 << ACTION_ARM_LEFT, static_cast<int>(frame.Inputs[0].Buttons));
				Assert::IsTrue(calc.Advance());
			}
		}

		// 紛失した分は再送し、順番が逆でも穴が埋まるまでACKを進めない。
		TEST_METHOD(Resend_SelectsOldestEightAndAcknowledgesContiguousFrames)
		{
			LockstepCalc sender, receiver;
			Assert::IsTrue(sender.Initialize(3, 0));
			Assert::IsTrue(receiver.Initialize(3, 0));
			for (int frame = 0; frame < 12; ++frame) {
				Assert::IsTrue(sender.StoreInput(frame, 0, {}));
				Assert::IsTrue(sender.StoreInput(frame, 1, {}));
			}
			const auto first = sender.SelectUnacknowledged(-1, 3);
			Assert::AreEqual(8, first.Count);
			Assert::AreEqual(0, first.Frames[0].Number);
			Assert::AreEqual(7, first.Frames[7].Number);
			for (int frame = 7; frame >= 1; --frame) {
				for (int slot = 0; slot < 2; ++slot) Assert::IsTrue(receiver.StoreInput(frame, slot, {}));
			}
			Assert::AreEqual(-1, receiver.GetReceivedThrough(3));
			Assert::IsTrue(receiver.StoreInput(0, 0, {}));
			Assert::AreEqual(7, receiver.GetReceivedThrough(1));
			Assert::AreEqual(-1, receiver.GetReceivedThrough(3));
			Assert::IsTrue(receiver.StoreInput(0, 1, {}));
			Assert::IsTrue(receiver.StoreInput(0, 1, {})); // 重複パケット
			Assert::AreEqual(7, receiver.GetReceivedThrough(3));
			const auto next = sender.SelectUnacknowledged(receiver.GetReceivedThrough(3), 3);
			Assert::AreEqual(4, next.Count);
			Assert::AreEqual(8, next.Frames[0].Number);
			Assert::AreEqual(11, next.Frames[3].Number);
			const auto client = sender.SelectUnacknowledged(7, 2);
			Assert::AreEqual(2, static_cast<int>(client.Frames[0].Mask));
		}

		// 4人分を符号・装備・ACK・版・ビルド込みで往復でき、通信の配置は構造体の余白によらない。
		TEST_METHOD(Packet_RoundTripPreservesAllFieldsAndRejectsBrokenData)
		{
			LockstepCalc sender;
			Assert::IsTrue(sender.Initialize(15, 0));
			for (int frame = 0; frame < 8; ++frame) {
				for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
					auto input = ActionCalcPlayerInput({ -0.6f, 0.8f }, { -0.02f, 0.04f }, 1u << ACTION_JUMP);
					if (slot != 0) input.Equip = { slot % 2, frame % 6 };
					Assert::IsTrue(sender.StoreInput(frame, slot, input));
				}
			}
			auto packet = sender.SelectUnacknowledged(-1, 15);
			packet.Delay = 7;
			packet.Acknowledged = 42;
			std::vector<uint8_t> bytes;
			Assert::IsTrue(LockstepCalc::EncodePacket(packet, bytes));
			Assert::AreEqual(431, static_cast<int>(bytes.size()));
			Assert::AreEqual(0x50, static_cast<int>(bytes[0])); // P1N1
			Assert::AreEqual(0x31, static_cast<int>(bytes[1]));
			Assert::AreEqual(1, static_cast<int>(bytes[4])); // 版はリトルエンディアン
			Assert::AreEqual(0, static_cast<int>(bytes[5]));
			LOCKSTEP_PACKET decoded;
			Assert::IsTrue(LockstepCalc::DecodePacket(bytes.data(), bytes.size(), decoded));
			Assert::AreEqual(42, decoded.Acknowledged);
			Assert::AreEqual(7, decoded.Delay);
			Assert::AreEqual(8, decoded.Count);
			Assert::AreEqual(static_cast<int>(LOCKSTEP_CURRENT_BUILD), static_cast<int>(decoded.Build));
			for (int frame = 0; frame < 8; ++frame) {
				Assert::AreEqual(frame, decoded.Frames[frame].Number);
				Assert::AreEqual(15, static_cast<int>(decoded.Frames[frame].Mask));
				for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
					const auto& expected = packet.Frames[frame].Inputs[slot];
					const auto& actual = decoded.Frames[frame].Inputs[slot];
					Assert::AreEqual(static_cast<int>(expected.MoveX), static_cast<int>(actual.MoveX));
					Assert::AreEqual(static_cast<int>(expected.MoveY), static_cast<int>(actual.MoveY));
					Assert::AreEqual(static_cast<int>(expected.LookX), static_cast<int>(actual.LookX));
					Assert::AreEqual(static_cast<int>(expected.LookY), static_cast<int>(actual.LookY));
					Assert::AreEqual(static_cast<int>(expected.Buttons), static_cast<int>(actual.Buttons));
					Assert::AreEqual(expected.Equip.Hand, actual.Equip.Hand);
					Assert::AreEqual(expected.Equip.Number, actual.Equip.Number);
				}
			}
			Assert::IsFalse(LockstepCalc::DecodePacket(bytes.data(), bytes.size() - 1, decoded));
			bytes[4] = 2; // 違う版のデータをゲームの入力へ渡さない
			Assert::IsFalse(LockstepCalc::DecodePacket(bytes.data(), bytes.size(), decoded));
			Assert::AreEqual(42, decoded.Acknowledged); // 失敗時は出力を変えない
			packet.Count = 0;
			packet.Acknowledged = -1;
			Assert::IsTrue(LockstepCalc::EncodePacket(packet, bytes));
			Assert::IsTrue(LockstepCalc::DecodePacket(bytes.data(), bytes.size(), decoded));
			Assert::AreEqual(0, decoded.Count);
			Assert::AreEqual(-1, decoded.Acknowledged);
		}

		// 実行しただけでは消さず、ACK後に解放する。保管数より長い試合も続けられる。
		TEST_METHOD(History_KeepsUnacknowledgedInputsAcrossExecution)
		{
			LockstepCalc calc;
			Assert::IsTrue(calc.Initialize(1, 0));
			LOCKSTEP_FRAME stored;
			for (int frame = 0; frame < LOCKSTEP_HISTORY + 10; ++frame) {
				Assert::IsTrue(calc.StoreLocalInput(0, {}));
				calc.DiscardAcknowledged(frame); // 実行前の分は消えない
				Assert::IsTrue(calc.IsReady());
				Assert::IsTrue(calc.Advance());
				Assert::IsTrue(calc.GetFrame(frame, stored));
				Assert::AreEqual(frame, calc.GetReceivedThrough(1));
				calc.DiscardAcknowledged(frame);
				Assert::IsFalse(calc.GetFrame(frame, stored));
			}
		}

		// 位置の各軸・%・KOを1項目ずつ変えると、どのスロットでもチェックサムに反映される。
		TEST_METHOD(Checksum_UsesEveryFighterStateField)
		{
			LOCKSTEP_STATE states[FIGHTER_COUNT] = {};
			const uint32_t original = LockstepCalc::CalculateChecksum(states);
			Assert::AreEqual(original, LockstepCalc::CalculateChecksum(states));
			for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
				float* fields[] = { &states[slot].Position.X, &states[slot].Position.Y, &states[slot].Position.Z, &states[slot].Percent };
				for (float* field : fields) {
					*field = 1.0f;
					Assert::AreNotEqual(original, LockstepCalc::CalculateChecksum(states));
					*field = 0.0f;
				}
				states[slot].Score = 1;
				Assert::AreNotEqual(original, LockstepCalc::CalculateChecksum(states));
				states[slot].Score = 0;
			}
		}
	};
}
