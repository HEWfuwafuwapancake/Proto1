#include "net/lockstepcalc.h"
#include <cstring>
#include <limits>

// 調整用の値（通信の配置はEncodePacket・DecodePacketで同じ順にする）
static constexpr uint32_t PACKET_MAGIC = 0x314E3150; // P1N1
static constexpr uint8_t PLAYER_MASK = (1u << FIGHTER_COUNT) - 1u;
static constexpr uint16_t BUTTON_MASK = (1u << ACTION_COUNT) - 1u;
static constexpr size_t PACKET_HEADER_BYTES = 15;
static constexpr size_t PLAYER_INPUT_BYTES = 12;
static constexpr size_t FRAME_NUMBER_BYTES = 4;
static constexpr uint32_t CHECKSUM_SEED = 2166136261u;
static constexpr uint32_t CHECKSUM_PRIME = 16777619u;

static bool ValidMask(uint8_t mask)
{
	return mask != 0 && (mask & ~PLAYER_MASK) == 0;
}

static bool ValidInput(const PLAYER_INPUT& input)
{
	const bool noEquip = input.Equip.Hand == -1 && input.Equip.Number == -1;
	const bool equip = input.Equip.Hand >= 0 && input.Equip.Hand < 2 && input.Equip.Number >= 0 && input.Equip.Number < 6;
	return (input.Buttons & ~BUTTON_MASK) == 0 && (noEquip || equip)
		&& input.MoveX != INT16_MIN && input.MoveY != INT16_MIN && input.LookX != INT16_MIN && input.LookY != INT16_MIN;
}

static bool SameInput(const PLAYER_INPUT& a, const PLAYER_INPUT& b)
{
	return a.MoveX == b.MoveX && a.MoveY == b.MoveY && a.LookX == b.LookX && a.LookY == b.LookY
		&& a.Buttons == b.Buttons && a.Equip.Hand == b.Equip.Hand && a.Equip.Number == b.Equip.Number;
}

bool LockstepCalc::Initialize(uint8_t playerMask, int delay)
{
	if (!ValidMask(playerMask) || delay < 0 || delay > LOCKSTEP_MAX_DELAY) return false;
	m_PlayerMask = playerMask;
	m_Delay = delay;
	m_NextFrame = 0;
	m_Frames.clear();
	for (auto& received : m_Received) received = -1;
	for (int frame = 0; frame < delay; ++frame) {
		for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
			if ((playerMask & (1u << slot)) != 0) StoreInput(frame, slot, {});
		}
	}
	return true;
}

bool LockstepCalc::StoreInput(int32_t frame, int slot, const PLAYER_INPUT& input)
{
	if (slot < 0 || slot >= FIGHTER_COUNT || (m_PlayerMask & (1u << slot)) == 0 || !ValidInput(input)) return false;
	if (frame < 0 || frame == INT32_MAX) return false;
	const uint8_t bit = static_cast<uint8_t>(1u << slot);
	auto found = m_Frames.find(frame);
	if (found != m_Frames.end() && (found->second.Mask & bit) != 0) {
		return SameInput(found->second.Inputs[slot], input);
	}
	if (frame < m_NextFrame || static_cast<int64_t>(frame) - m_NextFrame >= LOCKSTEP_HISTORY) return false;
	if (found == m_Frames.end() && m_Frames.size() >= LOCKSTEP_HISTORY) return false;
	LOCKSTEP_FRAME& stored = m_Frames[frame];
	stored.Number = frame;
	stored.Mask |= bit;
	stored.Inputs[slot] = input;
	// 順不同で届いても、抜けが埋まるまでは受領済みの番号を進めない。
	while (true) {
		const auto next = m_Frames.find(m_Received[slot] + 1);
		if (next == m_Frames.end() || (next->second.Mask & bit) == 0) break;
		++m_Received[slot];
	}
	return true;
}

bool LockstepCalc::StoreLocalInput(int slot, const PLAYER_INPUT& input)
{
	if (m_NextFrame > INT32_MAX - m_Delay - 1) return false;
	return StoreInput(m_NextFrame + m_Delay, slot, input);
}

bool LockstepCalc::IsReady() const
{
	const auto found = m_Frames.find(m_NextFrame);
	return m_PlayerMask != 0 && found != m_Frames.end() && (found->second.Mask & m_PlayerMask) == m_PlayerMask;
}

bool LockstepCalc::GetFrame(int32_t frame, LOCKSTEP_FRAME& result) const
{
	const auto found = m_Frames.find(frame);
	if (found == m_Frames.end()) return false;
	result = found->second;
	return true;
}

bool LockstepCalc::Advance()
{
	if (!IsReady()) return false;
	++m_NextFrame;
	return true;
}

int32_t LockstepCalc::GetReceivedThrough(uint8_t mask) const
{
	if (!ValidMask(mask) || (mask & m_PlayerMask) != mask) return -1;
	int32_t received = INT32_MAX;
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		if ((mask & (1u << slot)) != 0 && m_Received[slot] < received) received = m_Received[slot];
	}
	return received;
}

LOCKSTEP_PACKET LockstepCalc::SelectUnacknowledged(int32_t acknowledged, uint8_t mask) const
{
	LOCKSTEP_PACKET packet;
	packet.Delay = m_Delay;
	packet.Mask = mask;
	if (acknowledged < -1 || !ValidMask(mask) || (mask & m_PlayerMask) != mask) return packet;
	for (auto it = m_Frames.upper_bound(acknowledged); it != m_Frames.end() && packet.Count < LOCKSTEP_PACKET_FRAMES; ++it) {
		if ((it->second.Mask & mask) != mask) continue;
		LOCKSTEP_FRAME& frame = packet.Frames[packet.Count++];
		frame.Number = it->first;
		frame.Mask = mask;
		for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
			if ((mask & (1u << slot)) != 0) frame.Inputs[slot] = it->second.Inputs[slot];
		}
	}
	return packet;
}

void LockstepCalc::DiscardAcknowledged(int32_t acknowledged)
{
	for (auto it = m_Frames.begin(); it != m_Frames.end() && it->first <= acknowledged && it->first < m_NextFrame;) {
		it = m_Frames.erase(it);
	}
}

static bool ValidPacket(const LOCKSTEP_PACKET& packet)
{
	if (packet.Version != LOCKSTEP_VERSION || packet.Build > LOCKSTEP_BUILD_RELEASE || !ValidMask(packet.Mask)
		|| packet.Delay < 0 || packet.Delay > LOCKSTEP_MAX_DELAY || packet.Acknowledged < -1
		|| packet.Acknowledged == INT32_MAX || packet.Count < 0 || packet.Count > LOCKSTEP_PACKET_FRAMES) return false;
	int32_t previous = -1;
	for (int i = 0; i < packet.Count; ++i) {
		const LOCKSTEP_FRAME& frame = packet.Frames[i];
		if (frame.Number <= previous || frame.Number == INT32_MAX || frame.Mask != packet.Mask) return false;
		previous = frame.Number;
		for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
			if ((packet.Mask & (1u << slot)) != 0 && !ValidInput(frame.Inputs[slot])) return false;
		}
	}
	return true;
}

static void Write(std::vector<uint8_t>& bytes, uint32_t value, int count)
{
	for (int i = 0; i < count; ++i) bytes.push_back(static_cast<uint8_t>(value >> (i * 8)));
}

static uint32_t Read(const uint8_t*& bytes, int count)
{
	uint32_t value = 0;
	for (int i = 0; i < count; ++i) value |= static_cast<uint32_t>(*bytes++) << (i * 8);
	return value;
}

// 符号なしから符号つきへの範囲外キャストを避け、通信の2の補数を明示的に戻す。
static int16_t ReadAxis(const uint8_t*& bytes)
{
	const int value = static_cast<int>(Read(bytes, 2));
	return static_cast<int16_t>(value <= INT16_MAX ? value : value - 65536);
}

bool LockstepCalc::EncodePacket(const LOCKSTEP_PACKET& packet, std::vector<uint8_t>& bytes)
{
	if (!ValidPacket(packet)) return false;
	bytes.clear();
	Write(bytes, PACKET_MAGIC, 4);
	Write(bytes, packet.Version, 2);
	Write(bytes, packet.Build, 1);
	Write(bytes, packet.Mask, 1);
	Write(bytes, static_cast<uint32_t>(packet.Delay), 2);
	Write(bytes, static_cast<uint32_t>(packet.Acknowledged), 4);
	Write(bytes, static_cast<uint32_t>(packet.Count), 1);
	for (int i = 0; i < packet.Count; ++i) {
		const LOCKSTEP_FRAME& frame = packet.Frames[i];
		Write(bytes, static_cast<uint32_t>(frame.Number), 4);
		for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
			if ((packet.Mask & (1u << slot)) == 0) continue;
			const PLAYER_INPUT& input = frame.Inputs[slot];
			Write(bytes, static_cast<uint16_t>(input.MoveX), 2);
			Write(bytes, static_cast<uint16_t>(input.MoveY), 2);
			Write(bytes, static_cast<uint16_t>(input.LookX), 2);
			Write(bytes, static_cast<uint16_t>(input.LookY), 2);
			Write(bytes, input.Buttons, 2);
			Write(bytes, static_cast<uint8_t>(input.Equip.Hand), 1);
			Write(bytes, static_cast<uint8_t>(input.Equip.Number), 1);
		}
	}
	return true;
}

bool LockstepCalc::DecodePacket(const uint8_t* bytes, size_t size, LOCKSTEP_PACKET& result)
{
	if (bytes == nullptr || size < PACKET_HEADER_BYTES) return false;
	if (Read(bytes, 4) != PACKET_MAGIC) return false;
	LOCKSTEP_PACKET packet;
	packet.Version = static_cast<uint16_t>(Read(bytes, 2));
	packet.Build = static_cast<LOCKSTEP_BUILD>(Read(bytes, 1));
	packet.Mask = static_cast<uint8_t>(Read(bytes, 1));
	packet.Delay = static_cast<int>(Read(bytes, 2));
	const uint32_t ack = Read(bytes, 4);
	if (ack > INT32_MAX && ack != UINT32_MAX) return false;
	packet.Acknowledged = ack == UINT32_MAX ? -1 : static_cast<int32_t>(ack);
	packet.Count = static_cast<int>(Read(bytes, 1));
	if (packet.Count > LOCKSTEP_PACKET_FRAMES || !ValidMask(packet.Mask)) return false;
	size_t players = 0;
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) if ((packet.Mask & (1u << slot)) != 0) ++players;
	const size_t expected = PACKET_HEADER_BYTES + packet.Count * (FRAME_NUMBER_BYTES + players * PLAYER_INPUT_BYTES);
	if (size != expected) return false;
	for (int i = 0; i < packet.Count; ++i) {
		LOCKSTEP_FRAME& frame = packet.Frames[i];
		const uint32_t number = Read(bytes, 4);
		if (number >= INT32_MAX) return false;
		frame.Number = static_cast<int32_t>(number);
		frame.Mask = packet.Mask;
		for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
			if ((packet.Mask & (1u << slot)) == 0) continue;
			PLAYER_INPUT& input = frame.Inputs[slot];
			input.MoveX = ReadAxis(bytes);
			input.MoveY = ReadAxis(bytes);
			input.LookX = ReadAxis(bytes);
			input.LookY = ReadAxis(bytes);
			input.Buttons = static_cast<uint16_t>(Read(bytes, 2));
			const int hand = static_cast<int>(Read(bytes, 1));
			const int equip = static_cast<int>(Read(bytes, 1));
			input.Equip = { hand == 255 ? -1 : hand, equip == 255 ? -1 : equip };
		}
	}
	if (!ValidPacket(packet)) return false;
	result = packet;
	return true;
}

static void HashWord(uint32_t& hash, uint32_t word)
{
	for (int i = 0; i < 4; ++i) {
		hash ^= static_cast<uint8_t>(word >> (i * 8));
		hash *= CHECKSUM_PRIME;
	}
}

static uint32_t FloatBits(float value)
{
	static_assert(sizeof(float) == sizeof(uint32_t) && std::numeric_limits<float>::is_iec559, "IEEE754 float required");
	uint32_t bits = 0;
	// +0 と -0 は同じ位置として扱う。
	if (value != 0.0f) std::memcpy(&bits, &value, sizeof(bits));
	return bits;
}

uint32_t LockstepCalc::CalculateChecksum(const LOCKSTEP_STATE (&states)[FIGHTER_COUNT])
{
	uint32_t hash = CHECKSUM_SEED;
	for (const LOCKSTEP_STATE& state : states) {
		HashWord(hash, FloatBits(state.Position.X));
		HashWord(hash, FloatBits(state.Position.Y));
		HashWord(hash, FloatBits(state.Position.Z));
		HashWord(hash, FloatBits(state.Percent));
		HashWord(hash, static_cast<uint32_t>(state.Score));
	}
	return hash;
}
