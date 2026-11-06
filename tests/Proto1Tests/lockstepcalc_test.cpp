// ロックステップの計算だけを確かめる（ソケット・Windows・DirectXは不要）。
#include <gtest/gtest.h>
#include "net/lockstepcalc.h"

namespace Proto1Tests
{
	// ホストの遅延を共有し、先頭は無入力。全員が届くまで同じフレームで待つ。
	TEST(LockstepCalcTest, Delay_WaitsForAllPlayers)
	{
		for (int delay : { LOCKSTEP_DEFAULT_DELAY, 6 }) {
			LockstepCalc calc;
			EXPECT_TRUE(calc.Initialize(0x0B, delay)); // 0・1・3が人、2はダミー
			EXPECT_EQ(delay, calc.GetDelay());
			const auto input = ActionCalcPlayerInput({ 1.0f, 0.0f }, {}, 1u << ACTION_ARM_LEFT);
			EXPECT_TRUE(calc.StoreLocalInput(0, input));
			LOCKSTEP_FRAME frame;
			EXPECT_TRUE(calc.GetFrame(0, frame));
			EXPECT_EQ(0, static_cast<int>(frame.Inputs[0].Buttons));
			for (int i = 0; i < delay; ++i) EXPECT_TRUE(calc.Advance());
			EXPECT_EQ(delay, calc.GetNextFrame());
			EXPECT_FALSE(calc.IsReady());
			EXPECT_FALSE(calc.Advance());
			EXPECT_TRUE(calc.StoreInput(delay, 3, {}));
			EXPECT_FALSE(calc.IsReady());
			EXPECT_TRUE(calc.StoreInput(delay, 1, {}));
			EXPECT_TRUE(calc.GetFrame(delay, frame));
			EXPECT_EQ(32767, static_cast<int>(frame.Inputs[0].MoveX));
			EXPECT_EQ(1 << ACTION_ARM_LEFT, static_cast<int>(frame.Inputs[0].Buttons));
			EXPECT_TRUE(calc.Advance());
		}
	}

	// 紛失した分は再送し、順番が逆でも穴が埋まるまでACKを進めない。
	TEST(LockstepCalcTest, Resend_SelectsOldestEightAndAcknowledgesContiguousFrames)
	{
		LockstepCalc sender, receiver;
		EXPECT_TRUE(sender.Initialize(3, 0));
		EXPECT_TRUE(receiver.Initialize(3, 0));
		for (int frame = 0; frame < 12; ++frame) {
			EXPECT_TRUE(sender.StoreInput(frame, 0, {}));
			EXPECT_TRUE(sender.StoreInput(frame, 1, {}));
		}
		const auto first = sender.SelectUnacknowledged(-1, 3);
		EXPECT_EQ(8, first.Count);
		EXPECT_EQ(0, first.Frames[0].Number);
		EXPECT_EQ(7, first.Frames[7].Number);
		for (int frame = 7; frame >= 1; --frame) {
			for (int slot = 0; slot < 2; ++slot) EXPECT_TRUE(receiver.StoreInput(frame, slot, {}));
		}
		EXPECT_EQ(-1, receiver.GetReceivedThrough(3));
		EXPECT_TRUE(receiver.StoreInput(0, 0, {}));
		EXPECT_EQ(7, receiver.GetReceivedThrough(1));
		EXPECT_EQ(-1, receiver.GetReceivedThrough(3));
		EXPECT_TRUE(receiver.StoreInput(0, 1, {}));
		EXPECT_TRUE(receiver.StoreInput(0, 1, {})); // 重複パケット
		EXPECT_EQ(7, receiver.GetReceivedThrough(3));
		const auto next = sender.SelectUnacknowledged(receiver.GetReceivedThrough(3), 3);
		EXPECT_EQ(4, next.Count);
		EXPECT_EQ(8, next.Frames[0].Number);
		EXPECT_EQ(11, next.Frames[3].Number);
		const auto client = sender.SelectUnacknowledged(7, 2);
		EXPECT_EQ(2, static_cast<int>(client.Frames[0].Mask));
	}

	// 4人分を符号・装備・ACK・版・ビルド込みで往復でき、通信の配置は構造体の余白によらない。
	TEST(LockstepCalcTest, Packet_RoundTripPreservesAllFieldsAndRejectsBrokenData)
	{
		LockstepCalc sender;
		EXPECT_TRUE(sender.Initialize(15, 0));
		for (int frame = 0; frame < 8; ++frame) {
			for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
				auto input = ActionCalcPlayerInput({ -0.6f, 0.8f }, { -0.02f, 0.04f }, 1u << ACTION_JUMP);
				// 3段目の腕（25 まで）の番号も送れる。
				if (slot != 0) input.Equip = { slot % 2, (frame * 3 + slot) % ACTION_EQUIP_NUMBER_COUNT };
				EXPECT_TRUE(sender.StoreInput(frame, slot, input));
			}
		}
		auto packet = sender.SelectUnacknowledged(-1, 15);
		packet.Delay = 7;
		packet.Acknowledged = 42;
		std::vector<uint8_t> bytes;
		EXPECT_TRUE(LockstepCalc::EncodePacket(packet, bytes));
		EXPECT_EQ(431, static_cast<int>(bytes.size()));
		EXPECT_EQ(0x50, static_cast<int>(bytes[0])); // P1N1
		EXPECT_EQ(0x31, static_cast<int>(bytes[1]));
		EXPECT_EQ(static_cast<int>(LOCKSTEP_VERSION), static_cast<int>(bytes[4])); // 版はリトルエンディアン
		EXPECT_EQ(0, static_cast<int>(bytes[5]));
		LOCKSTEP_PACKET decoded;
		EXPECT_TRUE(LockstepCalc::DecodePacket(bytes.data(), bytes.size(), decoded));
		EXPECT_EQ(42, decoded.Acknowledged);
		EXPECT_EQ(7, decoded.Delay);
		EXPECT_EQ(8, decoded.Count);
		EXPECT_EQ(static_cast<int>(LOCKSTEP_CURRENT_BUILD), static_cast<int>(decoded.Build));
		for (int frame = 0; frame < 8; ++frame) {
			EXPECT_EQ(frame, decoded.Frames[frame].Number);
			EXPECT_EQ(15, static_cast<int>(decoded.Frames[frame].Mask));
			for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
				const auto& expected = packet.Frames[frame].Inputs[slot];
				const auto& actual = decoded.Frames[frame].Inputs[slot];
				EXPECT_EQ(static_cast<int>(expected.MoveX), static_cast<int>(actual.MoveX));
				EXPECT_EQ(static_cast<int>(expected.MoveY), static_cast<int>(actual.MoveY));
				EXPECT_EQ(static_cast<int>(expected.LookX), static_cast<int>(actual.LookX));
				EXPECT_EQ(static_cast<int>(expected.LookY), static_cast<int>(actual.LookY));
				EXPECT_EQ(static_cast<int>(expected.Buttons), static_cast<int>(actual.Buttons));
				EXPECT_EQ(expected.Equip.Hand, actual.Equip.Hand);
				EXPECT_EQ(expected.Equip.Number, actual.Equip.Number);
			}
		}
		EXPECT_FALSE(LockstepCalc::DecodePacket(bytes.data(), bytes.size() - 1, decoded));
		bytes[4] = static_cast<uint8_t>(LOCKSTEP_VERSION + 1); // 違う版のデータをゲームの入力へ渡さない
		EXPECT_FALSE(LockstepCalc::DecodePacket(bytes.data(), bytes.size(), decoded));
		EXPECT_EQ(42, decoded.Acknowledged); // 失敗時は出力を変えない
		packet.Count = 0;
		packet.Acknowledged = -1;
		EXPECT_TRUE(LockstepCalc::EncodePacket(packet, bytes));
		EXPECT_TRUE(LockstepCalc::DecodePacket(bytes.data(), bytes.size(), decoded));
		EXPECT_EQ(0, decoded.Count);
		EXPECT_EQ(-1, decoded.Acknowledged);
	}

	// 実行しただけでは消さず、ACK後に解放する。保管数より長い試合も続けられる。
	TEST(LockstepCalcTest, History_KeepsUnacknowledgedInputsAcrossExecution)
	{
		LockstepCalc calc;
		EXPECT_TRUE(calc.Initialize(1, 0));
		LOCKSTEP_FRAME stored;
		for (int frame = 0; frame < LOCKSTEP_HISTORY + 10; ++frame) {
			EXPECT_TRUE(calc.StoreLocalInput(0, {}));
			calc.DiscardAcknowledged(frame); // 実行前の分は消えない
			EXPECT_TRUE(calc.IsReady());
			EXPECT_TRUE(calc.Advance());
			EXPECT_TRUE(calc.GetFrame(frame, stored));
			EXPECT_EQ(frame, calc.GetReceivedThrough(1));
			calc.DiscardAcknowledged(frame);
			EXPECT_FALSE(calc.GetFrame(frame, stored));
		}
	}

	// 位置の各軸・%・KOを1項目ずつ変えると、どのスロットでもチェックサムに反映される。
	TEST(LockstepCalcTest, Checksum_UsesEveryFighterStateField)
	{
		LOCKSTEP_STATE states[FIGHTER_COUNT] = {};
		const uint32_t original = LockstepCalc::CalculateChecksum(states);
		EXPECT_EQ(original, LockstepCalc::CalculateChecksum(states));
		for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
			float* fields[] = { &states[slot].Position.X, &states[slot].Position.Y, &states[slot].Position.Z, &states[slot].Percent };
			for (float* field : fields) {
				*field = 1.0f;
				EXPECT_NE(original, LockstepCalc::CalculateChecksum(states));
				*field = 0.0f;
			}
			states[slot].Score = 1;
			EXPECT_NE(original, LockstepCalc::CalculateChecksum(states));
			states[slot].Score = 0;
		}
	}
}
