#include "net/sessioncalc.h"
#include <algorithm>
#include <sstream>

// 調整用の値（入力のP1N1と区別するP1S1）
static constexpr uint32_t SESSION_MAGIC = 0x31533150;
static constexpr size_t SESSION_HEADER_BYTES = 50;

static uint32_t s_Program = 0; // この実行ファイルの識別番号（NetSession が exe の中身から作る）。

void SessionCalcSetProgram(uint32_t program)
{
	s_Program = program;
}

uint32_t SessionCalcGetProgram()
{
	return s_Program;
}
static constexpr uint8_t ALL_PLAYERS = (1u << FIGHTER_COUNT) - 1u;
static constexpr uint64_t DISCONNECT_MILLISECONDS = 3000;

void SessionAddressCalc::Initialize(const std::string& localAddress)
{
	*this = SessionAddressCalc{};
	NET_OPTIONS parsed;
	if (!SessionCalcParseOptions("--join " + localAddress, parsed)) return;
	std::istringstream address(localAddress);
	for (int part = 0; part < 3; ++part) std::getline(address, m_Parts[part], '.');
	m_Cursor = 3;
}

void SessionAddressCalc::AppendDigit(int digit)
{
	if (digit < 0 || digit > 9) return;
	if (m_Parts[m_Cursor].size() == 3 && m_Cursor < 3) ++m_Cursor;
	if (m_Parts[m_Cursor].size() < 3) m_Parts[m_Cursor] += static_cast<char>('0' + digit);
}

bool SessionAddressCalc::Erase()
{
	if (GetText() == "...") return false;
	while (m_Parts[m_Cursor].empty() && m_Cursor > 0) --m_Cursor;
	if (!m_Parts[m_Cursor].empty()) m_Parts[m_Cursor].pop_back();
	return true;
}

void SessionAddressCalc::Move(int direction)
{
	if (direction < 0 && m_Cursor > 0) --m_Cursor;
	if (direction > 0 && m_Cursor < 3) ++m_Cursor;
}

void SessionAddressCalc::Adjust(int direction)
{
	if (direction == 0) return;
	if (m_Parts[m_Cursor].empty()) {
		m_Parts[m_Cursor] = direction > 0 ? "0" : "255";
		return;
	}
	int value = std::stoi(m_Parts[m_Cursor]);
	value = (std::max)(0, (std::min)(255, value));
	value = direction > 0 ? (value + 1) % 256 : (value + 255) % 256;
	m_Parts[m_Cursor] = std::to_string(value);
}

bool SessionAddressCalc::IsValid() const
{
	for (const auto& part : m_Parts) {
		if (part.empty() || std::stoi(part) > 255) return false;
	}
	return true;
}

std::string SessionAddressCalc::GetText() const
{
	std::string text;
	for (int part = 0; part < 4; ++part) {
		if (part > 0) text += '.';
		if (!m_Parts[part].empty()) text += std::to_string(std::stoi(m_Parts[part]));
	}
	return text;
}

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

int SessionCalcAutoDelay(int pingMilliseconds)
{
	if (pingMilliseconds < 0) return AUTO_DELAY_MIN;
	// 往復時間には1フレームごとの送受信の待ちも入っているので、フレーム数へ切り上げて余裕を足すだけでよい。
	const int frames = (pingMilliseconds * 60 + 999) / 1000 + AUTO_DELAY_MARGIN;
	return (std::max)(AUTO_DELAY_MIN, (std::min)(AUTO_DELAY_MAX, frames));
}

void SessionCalc::Initialize(const NET_OPTIONS& options, uint32_t nonce)
{
	*this = SessionCalc{};
	m_Host = options.Mode == NET_MODE_HOST;
	m_Nonce = nonce == 0 ? 1 : nonce;
	m_AutoDelay = options.Delay < 0;
	m_Delay = m_AutoDelay ? LOCKSTEP_DEFAULT_DELAY : options.Delay;
	if (m_Host) {
		m_Session = m_Nonce;
		m_Slot = 0;
		m_Mask = 1;
	}
}

SESSION_FAILURE SessionCalc::CheckCompatibility(const SESSION_MESSAGE& message)
{
	if (message.Version != LOCKSTEP_VERSION) return SESSION_FAILURE_VERSION;
	if (message.Build != LOCKSTEP_CURRENT_BUILD) return SESSION_FAILURE_BUILD;
	if (message.Program != s_Program) return SESSION_FAILURE_PROGRAM;
	return SESSION_FAILURE_NONE;
}

int SessionCalc::Join(uint32_t nonce, uint16_t version, LOCKSTEP_BUILD build, uint32_t program)
{
	if (!m_Host || nonce == 0 || version != LOCKSTEP_VERSION || build != LOCKSTEP_CURRENT_BUILD || program != s_Program) return -1;
	for (int slot = 1; slot < FIGHTER_COUNT; ++slot) {
		if (m_PeerNonces[slot] == nonce) return (m_Disconnected & (1u << slot)) ? -1 : slot;
	}
	if (m_Locked) return -1;
	for (int slot = 1; slot < FIGHTER_COUNT; ++slot) {
		if (m_PeerNonces[slot] != 0) continue;
		m_PeerNonces[slot] = nonce;
		m_LastReceived[slot] = m_Now;
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
		if (IsActivePeer(slot) && m_HasRound && m_Acknowledged[slot] < m_Shared.GetNextFrame() - 1) return false;
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
		if (alreadyReady && CanStart()) {
			// 試合の途中では変えない。参加側へは開始の状態通知で配る。
			if (m_AutoDelay) m_Delay = SessionCalcAutoDelay(GetAutoDelayPing());
			BeginRound(m_Round + 1);
		}
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
	m_WaitFrames = 0;
	m_DesyncFrame = -1;
	m_DesyncPeer = -1;
	m_Checksums.clear();
	for (int peer = 0; peer < FIGHTER_COUNT; ++peer) {
		m_RemoteChecksums[peer].clear();
		m_CheckAcknowledged[peer] = m_CheckedThrough[peer] = -1;
	}
}

void SessionCalc::Receive(int peer, const SESSION_MESSAGE& message)
{
	if (m_Failure != SESSION_FAILURE_NONE) return;
	if (m_Host) {
		if (peer <= 0 || peer >= FIGHTER_COUNT || message.Session != m_Session
			|| !IsActivePeer(peer) || message.Nonce != m_PeerNonces[peer] || message.Round != m_Round
			|| CheckCompatibility(message) != SESSION_FAILURE_NONE
			|| (message.Type != SESSION_JOIN && message.Type != SESSION_READY && message.Type != SESSION_INPUTS)) return;
		ReceiveMonitor(peer, message);
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
		if (message.Type != SESSION_STATE && message.Type != SESSION_INPUTS && message.Type != SESSION_REJECT) return;
		const SESSION_FAILURE compatibility = CheckCompatibility(message);
		if (compatibility != SESSION_FAILURE_NONE) {
			m_Failure = compatibility;
			m_Running = false;
			return;
		}
		if (message.Type == SESSION_REJECT) return;
		// 同じ接続からの状態通知は、ロックステップ中やロビーへ戻った直後も生存確認に使う。
		m_LastReceived[0] = m_Now;
		if (message.Round == m_Round) ReceiveMonitor(0, message);
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
	if (peer >= 0 && peer < FIGHTER_COUNT) {
		message.Stamp = static_cast<uint32_t>(m_Now);
		message.Echo = m_Echo[peer];
		message.CheckAck = m_CheckedThrough[peer];
		const auto check = m_Checksums.upper_bound(m_CheckAcknowledged[peer]);
		if (check != m_Checksums.end()) {
			message.CheckFrame = check->first;
			message.Checksum = check->second;
		}
	}
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
	if (m_Host && m_Running) {
		// すでに全員へ配った入力は変えない。切断した人の未受領分だけをホストが埋める。
		for (int slot = 1; slot < FIGHTER_COUNT; ++slot) {
			if ((m_Disconnected & (1u << slot)) == 0) continue;
			const int64_t end = static_cast<int64_t>(m_Shared.GetNextFrame()) + m_Delay;
			for (int64_t number = m_Shared.GetNextFrame(); number <= end && number < INT32_MAX; ++number) {
				LOCKSTEP_FRAME stored;
				const int32_t frameNumber = static_cast<int32_t>(number);
				if (!m_Shared.GetFrame(frameNumber, stored) || (stored.Mask & (1u << slot)) == 0) m_Shared.StoreInput(frameNumber, slot, {});
			}
		}
	}
	StoreCaptured();
	if (!m_Running) return false;
	if (!m_CapturedStored || m_CapturedFrame != m_Shared.GetNextFrame() || !m_Shared.IsReady()) {
		++m_WaitFrames;
		return false;
	}
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
			if (IsActivePeer(slot)) ack = (std::min)(ack, m_Acknowledged[slot]);
		}
	}
	m_Local.DiscardAcknowledged(m_Host ? m_Local.GetNextFrame() - 1 : ack);
	m_Shared.DiscardAcknowledged(m_Host ? ack : m_Shared.GetNextFrame() - 1);
}

void SessionCalc::ReturnToLobby()
{
	m_Running = false;
}

bool SessionCalc::IsActivePeer(int peer) const
{
	return peer >= 0 && peer < FIGHTER_COUNT && peer != m_Slot
		&& (m_Mask & (1u << peer)) != 0 && (m_Disconnected & (1u << peer)) == 0 && (m_Host || peer == 0);
}

void SessionCalc::CheckTimeouts()
{
	if (!m_Host) {
		// ホストが先に起動していなくても、最初の返事が来るまでは待ち続ける。
		if (m_Session != 0 && m_Failure == SESSION_FAILURE_NONE && m_Now - m_LastReceived[0] >= DISCONNECT_MILLISECONDS) {
			m_Failure = SESSION_FAILURE_TIMEOUT;
			m_Running = false;
		}
		return;
	}
	for (int peer = 1; peer < FIGHTER_COUNT; ++peer) {
		if (!IsActivePeer(peer) || m_Now - m_LastReceived[peer] < DISCONNECT_MILLISECONDS) continue;
		if (m_Locked) m_Disconnected |= static_cast<uint8_t>(1u << peer);
		else {
			m_Mask &= static_cast<uint8_t>(~(1u << peer));
			m_Ready &= m_Mask;
			m_PeerNonces[peer] = 0;
			m_LastEcho[peer] = 0;
		}
		m_RemoteChecksums[peer].clear();
	}
	DiscardHistory(); // 切断した人のACK待ちで保管が満杯になっても再開できる。
	CompareChecksums();
	StoreCaptured();
}

void SessionCalc::ReceiveMonitor(int peer, const SESSION_MESSAGE& message)
{
	m_LastReceived[peer] = m_Now;
	m_Echo[peer] = message.Stamp;
	// 相手が新しい時刻をまだ受け取っていないと同じ Echo が返り、待ったフレームの分だけ長く測れてしまう。
	if (message.Echo != 0 && message.Echo != m_LastEcho[peer]) {
		m_LastEcho[peer] = message.Echo;
		const uint32_t elapsed = static_cast<uint32_t>(m_Now) - message.Echo;
		if (elapsed < DISCONNECT_MILLISECONDS) {
			m_Ping[peer] = static_cast<int>(elapsed);
			m_PingHistory[peer].emplace_back(m_Now, m_Ping[peer]);
		}
	}
	while (!m_PingHistory[peer].empty() && m_Now - m_PingHistory[peer].front().first > AUTO_DELAY_PING_WINDOW) {
		m_PingHistory[peer].pop_front();
	}
	if (!m_HasRound) return;
	// 比較済みのACKだけで再送を終える。入力より先に相手のチェックサムが来ても保管する。
	if (!m_Checksums.empty() && message.CheckAck <= m_Checksums.rbegin()->first) {
		m_CheckAcknowledged[peer] = (std::max)(m_CheckAcknowledged[peer], message.CheckAck);
	}
	if (message.CheckFrame > m_CheckedThrough[peer]) {
		m_RemoteChecksums[peer].emplace(message.CheckFrame, message.Checksum);
	}
	CompareChecksums();
}

int SessionCalc::GetAutoDelayPing() const
{
	int slowest = -1;
	std::vector<int> pings;
	for (int peer = 0; peer < FIGHTER_COUNT; ++peer) {
		if (!IsActivePeer(peer)) continue;
		pings.clear();
		for (const auto& sample : m_PingHistory[peer]) {
			if (m_Now - sample.first <= AUTO_DELAY_PING_WINDOW) pings.push_back(sample.second);
		}
		if (pings.empty()) continue;
		// 小さい順の90%の位置（切り上げ）。遅いほうの1割を使わず、一瞬の遅れで delay を上げない。
		const size_t rank = (pings.size() * AUTO_DELAY_PING_PERCENTILE + 99) / 100;
		const size_t index = rank == 0 ? 0 : rank - 1;
		std::nth_element(pings.begin(), pings.begin() + index, pings.end());
		slowest = (std::max)(slowest, pings[index]);
	}
	return slowest;
}

void SessionCalc::RecordChecksum(int32_t frame, const LOCKSTEP_STATE (&states)[FIGHTER_COUNT])
{
	if (!m_HasRound || frame < 0 || frame == INT32_MAX || (frame + 1) % LOCKSTEP_CHECKSUM_INTERVAL != 0) return;
	m_Checksums.emplace(frame, LockstepCalc::CalculateChecksum(states));
	CompareChecksums();
}

void SessionCalc::CompareChecksums()
{
	int32_t discard = INT32_MAX;
	for (int peer = 0; peer < FIGHTER_COUNT; ++peer) {
		if (!IsActivePeer(peer)) continue;
		auto& remote = m_RemoteChecksums[peer];
		for (auto it = remote.begin(); it != remote.end();) {
			const auto local = m_Checksums.find(it->first);
			if (local == m_Checksums.end()) break;
			if (local->second != it->second && m_DesyncFrame < 0) {
				m_DesyncFrame = it->first;
				m_DesyncPeer = peer;
			}
			m_CheckedThrough[peer] = it->first;
			it = remote.erase(it);
		}
		discard = (std::min)(discard, (std::min)(m_CheckAcknowledged[peer], m_CheckedThrough[peer]));
	}
	for (auto it = m_Checksums.begin(); it != m_Checksums.end() && it->first <= discard;) it = m_Checksums.erase(it);
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
	return message.Type <= SESSION_REJECT && message.Build <= LOCKSTEP_BUILD_RELEASE
		&& message.CheckFrame >= -1 && message.CheckAck >= -1
		&& (message.CheckFrame == -1 || (message.CheckFrame < INT32_MAX && (message.CheckFrame + 1) % LOCKSTEP_CHECKSUM_INTERVAL == 0))
		&& (message.CheckAck == -1 || (message.CheckAck < INT32_MAX && (message.CheckAck + 1) % LOCKSTEP_CHECKSUM_INTERVAL == 0))
		&& message.Nonce != 0 && message.Slot >= 0 && message.Slot < FIGHTER_COUNT
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
	WriteWord(bytes, message.Version, 2);
	WriteWord(bytes, message.Type, 1);
	WriteWord(bytes, static_cast<uint32_t>(message.Slot), 1);
	WriteWord(bytes, message.Mask, 1);
	WriteWord(bytes, message.Ready, 1);
	WriteWord(bytes, static_cast<uint32_t>(message.Delay), 2);
	WriteWord(bytes, message.Running ? 1 : 0, 1);
	WriteWord(bytes, message.Build, 1);
	WriteWord(bytes, message.Session, 4);
	WriteWord(bytes, message.Nonce, 4);
	WriteWord(bytes, message.Round, 4);
	WriteWord(bytes, message.Stamp, 4);
	WriteWord(bytes, message.Echo, 4);
	WriteWord(bytes, static_cast<uint32_t>(message.CheckFrame), 4);
	WriteWord(bytes, message.Checksum, 4);
	WriteWord(bytes, static_cast<uint32_t>(message.CheckAck), 4);
	WriteWord(bytes, message.Program, 4);
	bytes.insert(bytes.end(), payload.begin(), payload.end());
	return true;
}

bool SessionCalc::Decode(const uint8_t* bytes, size_t size, SESSION_MESSAGE& result)
{
	if (bytes == nullptr || size < SESSION_HEADER_BYTES) return false;
	if (ReadWord(bytes, 4) != SESSION_MAGIC) return false;
	SESSION_MESSAGE message;
	message.Version = static_cast<uint16_t>(ReadWord(bytes, 2)); // 不一致でも制御通知は読んで理由を返す。
	message.Type = static_cast<SESSION_MESSAGE_TYPE>(ReadWord(bytes, 1));
	message.Slot = static_cast<int>(ReadWord(bytes, 1));
	message.Mask = static_cast<uint8_t>(ReadWord(bytes, 1));
	message.Ready = static_cast<uint8_t>(ReadWord(bytes, 1));
	message.Delay = static_cast<int>(ReadWord(bytes, 2));
	const uint32_t running = ReadWord(bytes, 1);
	message.Build = static_cast<LOCKSTEP_BUILD>(ReadWord(bytes, 1));
	if (running > 1) return false;
	message.Running = running != 0;
	message.Session = ReadWord(bytes, 4);
	message.Nonce = ReadWord(bytes, 4);
	message.Round = ReadWord(bytes, 4);
	message.Stamp = ReadWord(bytes, 4);
	message.Echo = ReadWord(bytes, 4);
	const uint32_t checkFrame = ReadWord(bytes, 4);
	message.Checksum = ReadWord(bytes, 4);
	const uint32_t checkAck = ReadWord(bytes, 4);
	message.Program = ReadWord(bytes, 4);
	if ((checkFrame > INT32_MAX && checkFrame != UINT32_MAX) || (checkAck > INT32_MAX && checkAck != UINT32_MAX)) return false;
	message.CheckFrame = checkFrame == UINT32_MAX ? -1 : static_cast<int32_t>(checkFrame);
	message.CheckAck = checkAck == UINT32_MAX ? -1 : static_cast<int32_t>(checkAck);
	if (!ValidMessage(message)) return false;
	if (message.Type == SESSION_INPUTS) {
		if (!LockstepCalc::DecodePacket(bytes, size - SESSION_HEADER_BYTES, message.Packet)) return false;
	}
	else if (size != SESSION_HEADER_BYTES) return false;
	result = message;
	return true;
}
