#include "net/sessioncalc.h"
#include <algorithm>
#include <sstream>

// 調整用の値（入力のP1N1と区別するP1S1）
static constexpr uint32_t SESSION_MAGIC = 0x31533150;
static constexpr size_t SESSION_HEADER_BYTES = 26;
static constexpr uint8_t ALL_PLAYERS = (1u << FIGHTER_COUNT) - 1u;

bool SessionCalcParseOptions(const std::string& commandLine, NET_OPTIONS& result)
{
	NET_OPTIONS options;
	std::istringstream stream(commandLine);
	std::string word;
	bool mode = false, delay = false;
	while (stream >> word) {
		if (word == "--host" || word == "--join") {
			if (mode) return false;
			mode = true;
			options.Mode = word == "--host" ? NET_MODE_HOST : NET_MODE_JOIN;
			if (options.Mode == NET_MODE_JOIN) {
				if (!(stream >> options.Host)) return false;
				std::istringstream address(options.Host);
				for (int part = 0; part < 4; ++part) {
					std::string octet;
					if (!std::getline(address, octet, '.') || octet.empty() || octet.size() > 3) return false;
					int value = 0;
					for (char c : octet) { if (c < '0' || c > '9') return false; value = value * 10 + c - '0'; }
					if (value > 255) return false;
				}
				if (options.Host.back() == '.' || address.peek() != EOF) return false;
			}
		}
		else if (word == "--delay") {
			if (delay || !(stream >> word) || word.empty()) return false;
			delay = true;
			int value = 0;
			for (char c : word) {
				if (c < '0' || c > '9' || value > LOCKSTEP_MAX_DELAY) return false;
				value = value * 10 + c - '0';
			}
			if (value > LOCKSTEP_MAX_DELAY) return false;
			options.Delay = value;
		}
		else return false;
	}
	if (delay && !mode) return false;
	result = options;
	return true;
}

void SessionCalc::Initialize(const NET_OPTIONS& options, uint32_t nonce)
{
	*this = SessionCalc{};
	m_Host = options.Mode == NET_MODE_HOST;
	m_Nonce = nonce == 0 ? 1 : nonce;
	m_Delay = options.Delay;
	if (m_Host) {
		m_Session = m_Nonce;
		m_Slot = 0;
		m_Mask = 1;
	}
}

int SessionCalc::Join(uint32_t nonce)
{
	if (!m_Host || nonce == 0) return -1;
	for (int slot = 1; slot < FIGHTER_COUNT; ++slot) if (m_PeerNonces[slot] == nonce) return slot;
	if (m_Locked) return -1;
	for (int slot = 1; slot < FIGHTER_COUNT; ++slot) {
		if (m_PeerNonces[slot] != 0) continue;
		m_PeerNonces[slot] = nonce;
		m_Mask |= static_cast<uint8_t>(1u << slot);
		return slot;
	}
	return -1;
}

bool SessionCalc::CanStart() const
{
	if (m_Mask == 1 || m_Ready != m_Mask || m_Round == UINT32_MAX) return false;
	// 最後のTITLE入力も全員に届くまで前の履歴を捨てない。
	for (int slot = 1; slot < FIGHTER_COUNT; ++slot) {
		if ((m_Mask & (1u << slot)) && m_HasRound && m_Acknowledged[slot] < m_Shared.GetNextFrame() - 1) return false;
	}
	return true;
}

void SessionCalc::Decide()
{
	if (m_Running) return;
	const bool alreadyReady = m_LocalReady;
	m_LocalReady = true;
	if (m_Host) {
		m_Ready |= 1;
		if (alreadyReady && CanStart()) BeginRound(m_Round + 1);
	}
}

void SessionCalc::BeginRound(uint32_t round)
{
	m_Round = round;
	m_Running = m_HasRound = m_Locked = true;
	m_Local.Initialize(static_cast<uint8_t>(1u << m_Slot), m_Delay);
	m_Shared.Initialize(m_Mask, m_Delay);
	for (auto& ack : m_Acknowledged) ack = -1;
	m_CapturedFrame = -1;
	m_CapturedStored = false;
}

void SessionCalc::Receive(int peer, const SESSION_MESSAGE& message)
{
	if (m_Host) {
		if (peer <= 0 || peer >= FIGHTER_COUNT || message.Session != m_Session
			|| message.Nonce != m_PeerNonces[peer] || message.Round != m_Round) return;
		if (message.Type == SESSION_READY && !m_Running) m_Ready |= static_cast<uint8_t>(1u << peer);
		if (message.Type != SESSION_INPUTS || !m_HasRound || message.Packet.Mask != (1u << peer)
			|| message.Packet.Delay != m_Delay) return;
		for (int i = 0; i < message.Packet.Count; ++i) {
			const auto& frame = message.Packet.Frames[i];
			m_Shared.StoreInput(frame.Number, peer, frame.Inputs[peer]);
		}
		const int32_t ack = message.Packet.Acknowledged;
		if (ack <= m_Shared.GetReceivedThrough(m_Mask)) m_Acknowledged[peer] = (std::max)(m_Acknowledged[peer], ack);
	}
	else {
		if (peer != 0 || message.Nonce != m_Nonce || message.Session == 0
			|| (m_Session != 0 && m_Session != message.Session)) return;
		if (message.Type == SESSION_STATE) {
			if (m_Running || message.Round < m_Round || message.Slot <= 0 || message.Slot >= FIGHTER_COUNT
				|| (message.Mask & (1u << message.Slot)) == 0 || (message.Mask & 1) == 0) return;
			// 遅れて届く前の開始通知ではTITLEから再開しない。
			if (message.Running && message.Round <= m_Round) return;
			m_Session = message.Session;
			m_Slot = message.Slot;
			m_Mask = message.Mask;
			m_Ready = message.Ready;
			m_Delay = message.Delay;
			if (message.Running && m_Ready == m_Mask) BeginRound(message.Round);
			return;
		}
		if (message.Type != SESSION_INPUTS || !m_HasRound || message.Round != m_Round
			|| message.Packet.Mask != m_Mask || message.Packet.Delay != m_Delay) return;
		for (int i = 0; i < message.Packet.Count; ++i) {
			const auto& frame = message.Packet.Frames[i];
			for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
				if (m_Mask & (1u << slot)) m_Shared.StoreInput(frame.Number, slot, frame.Inputs[slot]);
			}
		}
		const int32_t ack = message.Packet.Acknowledged;
		if (ack <= m_Local.GetReceivedThrough(static_cast<uint8_t>(1u << m_Slot))) {
			m_Acknowledged[0] = (std::max)(m_Acknowledged[0], ack);
		}
	}
	DiscardHistory();
	StoreCaptured();
}

SESSION_MESSAGE SessionCalc::MakeControl(int peer) const
{
	SESSION_MESSAGE message;
	message.Type = m_Host ? SESSION_STATE : (m_Session != 0 && m_LocalReady ? SESSION_READY : SESSION_JOIN);
	message.Session = m_Session;
	message.Nonce = m_Host && peer > 0 && peer < FIGHTER_COUNT ? m_PeerNonces[peer] : m_Nonce;
	message.Round = m_Round;
	message.Slot = m_Host ? peer : (m_Slot < 0 ? 0 : m_Slot);
	message.Mask = m_Mask;
	message.Ready = m_Ready;
	message.Delay = m_Delay;
	message.Running = m_Running;
	return message;
}

SESSION_MESSAGE SessionCalc::MakeInputs(int peer) const
{
	SESSION_MESSAGE message = MakeControl(peer);
	message.Type = SESSION_INPUTS;
	if (!m_HasRound || peer < 0 || peer >= FIGHTER_COUNT) return message;
	if (m_Host) {
		message.Packet = m_Shared.SelectUnacknowledged(m_Acknowledged[peer], m_Mask);
		message.Packet.Acknowledged = m_Shared.GetReceivedThrough(static_cast<uint8_t>(1u << peer));
	}
	else {
		message.Packet = m_Local.SelectUnacknowledged(m_Acknowledged[0], static_cast<uint8_t>(1u << m_Slot));
		message.Packet.Acknowledged = m_Shared.GetReceivedThrough(m_Mask);
	}
	return message;
}

bool SessionCalc::NeedsInput() const
{
	return m_Running && m_CapturedFrame != m_Shared.GetNextFrame();
}

void SessionCalc::CaptureInput(const PLAYER_INPUT& input)
{
	if (!NeedsInput()) return;
	m_CapturedFrame = m_Shared.GetNextFrame();
	m_Captured = input;
	m_CapturedStored = false;
	StoreCaptured();
}

void SessionCalc::StoreCaptured()
{
	if (!m_Running || m_CapturedStored || m_CapturedFrame != m_Shared.GetNextFrame()) return;
	if (!m_Local.StoreLocalInput(m_Slot, m_Captured)) return;
	if (m_Host && !m_Shared.StoreLocalInput(m_Slot, m_Captured)) return;
	m_CapturedStored = true;
}

bool SessionCalc::TakeFrame(LOCKSTEP_FRAME& frame)
{
	StoreCaptured();
	if (!m_Running || !m_CapturedStored || m_CapturedFrame != m_Shared.GetNextFrame() || !m_Shared.IsReady()) return false;
	m_Shared.GetFrame(m_Shared.GetNextFrame(), frame);
	m_Shared.Advance();
	m_Local.Advance();
	DiscardHistory();
	return true;
}

void SessionCalc::DiscardHistory()
{
	if (!m_HasRound) return;
	int32_t ack = m_Acknowledged[0];
	if (m_Host) {
		ack = INT32_MAX;
		for (int slot = 1; slot < FIGHTER_COUNT; ++slot) {
			if (m_Mask & (1u << slot)) ack = (std::min)(ack, m_Acknowledged[slot]);
		}
	}
	m_Local.DiscardAcknowledged(m_Host ? m_Local.GetNextFrame() - 1 : ack);
	m_Shared.DiscardAcknowledged(m_Host ? ack : m_Shared.GetNextFrame() - 1);
}

void SessionCalc::ReturnToLobby()
{
	m_Running = false;
}

static void WriteWord(std::vector<uint8_t>& bytes, uint32_t word, int count)
{
	for (int i = 0; i < count; ++i) bytes.push_back(static_cast<uint8_t>(word >> (i * 8)));
}

static uint32_t ReadWord(const uint8_t*& bytes, int count)
{
	uint32_t word = 0;
	for (int i = 0; i < count; ++i) word |= static_cast<uint32_t>(*bytes++) << (i * 8);
	return word;
}

static bool ValidMessage(const SESSION_MESSAGE& message)
{
	return message.Type <= SESSION_INPUTS && message.Nonce != 0 && message.Slot >= 0 && message.Slot < FIGHTER_COUNT
		&& (message.Mask & ~ALL_PLAYERS) == 0 && (message.Ready & ~message.Mask) == 0
		&& message.Delay >= 0 && message.Delay <= LOCKSTEP_MAX_DELAY;
}

bool SessionCalc::Encode(const SESSION_MESSAGE& message, std::vector<uint8_t>& bytes)
{
	if (!ValidMessage(message)) return false;
	std::vector<uint8_t> payload;
	if (message.Type == SESSION_INPUTS && !LockstepCalc::EncodePacket(message.Packet, payload)) return false;
	bytes.clear();
	WriteWord(bytes, SESSION_MAGIC, 4);
	WriteWord(bytes, LOCKSTEP_VERSION, 2);
	WriteWord(bytes, message.Type, 1);
	WriteWord(bytes, static_cast<uint32_t>(message.Slot), 1);
	WriteWord(bytes, message.Mask, 1);
	WriteWord(bytes, message.Ready, 1);
	WriteWord(bytes, static_cast<uint32_t>(message.Delay), 2);
	WriteWord(bytes, message.Running ? 1 : 0, 1);
	WriteWord(bytes, 0, 1); // 予約
	WriteWord(bytes, message.Session, 4);
	WriteWord(bytes, message.Nonce, 4);
	WriteWord(bytes, message.Round, 4);
	bytes.insert(bytes.end(), payload.begin(), payload.end());
	return true;
}

bool SessionCalc::Decode(const uint8_t* bytes, size_t size, SESSION_MESSAGE& result)
{
	if (bytes == nullptr || size < SESSION_HEADER_BYTES) return false;
	if (ReadWord(bytes, 4) != SESSION_MAGIC || ReadWord(bytes, 2) != LOCKSTEP_VERSION) return false;
	SESSION_MESSAGE message;
	message.Type = static_cast<SESSION_MESSAGE_TYPE>(ReadWord(bytes, 1));
	message.Slot = static_cast<int>(ReadWord(bytes, 1));
	message.Mask = static_cast<uint8_t>(ReadWord(bytes, 1));
	message.Ready = static_cast<uint8_t>(ReadWord(bytes, 1));
	message.Delay = static_cast<int>(ReadWord(bytes, 2));
	const uint32_t running = ReadWord(bytes, 1);
	if (running > 1 || ReadWord(bytes, 1) != 0) return false;
	message.Running = running != 0;
	message.Session = ReadWord(bytes, 4);
	message.Nonce = ReadWord(bytes, 4);
	message.Round = ReadWord(bytes, 4);
	if (!ValidMessage(message)) return false;
	if (message.Type == SESSION_INPUTS) {
		if (!LockstepCalc::DecodePacket(bytes, size - SESSION_HEADER_BYTES, message.Packet)) return false;
	}
	else if (size != SESSION_HEADER_BYTES) return false;
	result = message;
	return true;
}
