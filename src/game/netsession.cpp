#include "game/netsession.h"
#include "input/action.h"
#include <random>
#include <sstream>

#ifdef _DEBUG
#include "core/debug_ostream.h"
#endif

// 調整用の値（全PCで同じポート。参加側自身は空きポートを使う）
static constexpr uint16_t SESSION_PORT = 27026;
static constexpr int MAX_RECEIVE_PER_UPDATE = 64;
static constexpr size_t RECEIVE_BYTES = 1024;

bool NetSession::Initialize(const NET_OPTIONS& options)
{
	Finalize();
	m_Options = options;
	m_Device = -1;
	m_Error.clear();
	m_Started = std::chrono::steady_clock::now();
	m_ReportedDesync = -1;
	if (!IsOnline()) return true;
	if (!m_Socket.Initialize(options.Mode == NET_MODE_HOST ? SESSION_PORT : 0)) {
		m_Error = "UDPを開けませんでした。Winsock: " + std::to_string(m_Socket.GetError());
		Finalize();
		return false;
	}
	if (options.Mode == NET_MODE_JOIN && !NetSocket::ParseAddress(options.Host, SESSION_PORT, m_Peers[0])) {
		m_Error = "参加先のIPv4アドレスを確認してください。";
		Finalize();
		return false;
	}
	std::random_device random;
	m_Calc.Initialize(options, random());
	m_Addresses = options.Mode == NET_MODE_HOST ? NetSocket::GetLocalAddresses() : options.Host;
	return true;
}

void NetSession::Finalize()
{
	m_Socket.Finalize();
	m_Options = {};
	m_Calc = SessionCalc{};
	m_Device = -1;
	for (auto& peer : m_Peers) peer = {};
	m_Addresses.clear();
	m_ReportedDesync = -1;
}

void NetSession::Receive()
{
	for (int count = 0; count < MAX_RECEIVE_PER_UPDATE; ++count) {
		uint8_t bytes[RECEIVE_BYTES];
		NET_ADDRESS from;
		const int size = m_Socket.Receive(from, bytes, sizeof(bytes));
		if (size == 0) break;
		if (size < 0) continue;
		SESSION_MESSAGE message;
		if (!SessionCalc::Decode(bytes, static_cast<size_t>(size), message)) continue;
		if (!m_Calc.IsHost()) {
			if (NetSocket::SameAddress(from, m_Peers[0])) m_Calc.Receive(0, message);
			continue;
		}
		int peer = -1;
		for (int slot = 1; slot < FIGHTER_COUNT; ++slot) {
			if (m_Peers[slot].Port != 0 && NetSocket::SameAddress(from, m_Peers[slot])) peer = slot;
		}
		if (message.Type == SESSION_JOIN) {
			const SESSION_FAILURE failure = SessionCalc::CheckCompatibility(message);
			if (failure != SESSION_FAILURE_NONE) {
				auto rejection = m_Calc.MakeControl();
				rejection.Type = SESSION_REJECT;
				rejection.Nonce = message.Nonce;
				Send(from, rejection);
				m_Error = FailureText(failure);
				continue;
			}
			if (peer >= 0) {
				m_Calc.Receive(peer, message);
				continue;
			}
			const int slot = m_Calc.Join(message.Nonce, message.Version, message.Build);
			if (slot > 0 && m_Peers[slot].Port == 0) m_Peers[slot] = from;
		}
		else if (peer > 0) m_Calc.Receive(peer, message);
	}
}

void NetSession::Send(const NET_ADDRESS& address, const SESSION_MESSAGE& message)
{
	std::vector<uint8_t> bytes;
	if (SessionCalc::Encode(message, bytes)) m_Socket.Send(address, bytes.data(), bytes.size());
}

void NetSession::Update()
{
	if (!IsOnline()) return;
	m_Calc.SetTime(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - m_Started).count()));
	Receive();
	m_Calc.CheckTimeouts();
	ReportDesync();
	if (m_Calc.GetFailure() != SESSION_FAILURE_NONE) {
		m_Error = FailureText(m_Calc.GetFailure());
		Finalize();
		return;
	}
	if (m_Calc.IsHost()) {
		for (int slot = 1; slot < FIGHTER_COUNT; ++slot) {
			if ((m_Calc.GetMask() & (1u << slot)) == 0 || (m_Calc.GetDisconnectedMask() & (1u << slot))) m_Peers[slot] = {};
		}
	}
	if (!m_Calc.IsRunning()) {
		// 最初に決定を押した機器に固定。開始はホストの同じ機器からの再決定だけ。
		for (int device = 0; device < ACTION_DEVICE_COUNT; ++device) {
			if ((m_Device < 0 || m_Device == device) && ActionIsTrigger(device, ACTION_DECIDE)) {
				m_Device = device;
				m_Calc.Decide();
				break;
			}
		}
	}
	if (m_Calc.NeedsInput()) {
		PLAYER_INPUT input = ActionGetPlayerInput(m_Device);
		// パッドで遊ぶ場合もキーボードの装備選択を自分へ配る。F1〜F4には左右されない。
		input.Equip = ActionGetEquipKey(ActionGetEquipPlayer());
		m_Calc.CaptureInput(input);
	}
	if (m_Calc.IsHost()) {
		for (int slot = 1; slot < FIGHTER_COUNT; ++slot) {
			if (m_Peers[slot].Port == 0) continue;
			Send(m_Peers[slot], m_Calc.MakeControl(slot));
			if (m_Calc.HasRound()) Send(m_Peers[slot], m_Calc.MakeInputs(slot));
		}
	}
	else {
		Send(m_Peers[0], m_Calc.MakeControl());
		if (m_Calc.HasRound()) Send(m_Peers[0], m_Calc.MakeInputs());
	}
}

std::string NetSession::GetStatusText() const
{
	if (!IsOnline()) return m_Error.empty() ? std::string{} : "OFFLINE " + m_Error;
	int count = 0, ready = 0;
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		if (m_Calc.GetMask() & (1u << slot)) ++count;
		if (m_Calc.GetReadyMask() & (1u << slot)) ++ready;
	}
	std::ostringstream text;
	text << (m_Calc.IsHost() ? "HOST " : "JOIN ") << m_Addresses;
	if (m_Calc.GetLocalSlot() < 0) text << " 接続待ち";
	else text << " 自分 " << m_Calc.GetLocalSlot() + 1 << "P  " << count << "人（機器選択 " << ready << "人）";
	text << " delay " << m_Calc.GetDelay() << (m_Calc.IsAutoDelay() ? "（自動）" : "") << (m_Calc.IsRunning() ? " 対戦中" : " 待機中");
	for (int peer = 0; peer < FIGHTER_COUNT; ++peer) {
		if (m_Peers[peer].Port == 0) continue;
		text << " ping " << peer + 1 << "P:";
		if (m_Calc.GetPing(peer) < 0) text << "--";
		else text << m_Calc.GetPing(peer);
		text << "ms";
	}
	text << " 待ち " << m_Calc.GetWaitFrames() << "f";
	for (int peer = 1; peer < FIGHTER_COUNT; ++peer) {
		if (m_Calc.GetDisconnectedMask() & (1u << peer)) text << " 切断(無入力):" << peer + 1 << "P";
	}
	if (m_Calc.GetDesyncFrame() >= 0) text << " DESYNC " << m_Calc.GetDesyncPeer() + 1 << "P frame " << m_Calc.GetDesyncFrame();
	if (!m_Error.empty()) text << " " << m_Error;
	return text.str();
}

const char* NetSession::FailureText(SESSION_FAILURE failure)
{
	switch (failure) {
	case SESSION_FAILURE_VERSION: return "参加拒否：通信の版が違います。同じ版を使ってください。";
	case SESSION_FAILURE_BUILD: return "参加拒否：Debug / Release が違います。同じビルドを使ってください。";
	case SESSION_FAILURE_TIMEOUT: return "ホストから3秒届かなかったため、モード選択へ戻りました。";
	default: return "";
	}
}

void NetSession::RecordChecksum(int32_t frame, const LOCKSTEP_STATE (&states)[FIGHTER_COUNT])
{
	m_Calc.RecordChecksum(frame, states);
	ReportDesync();
}

void NetSession::ReportDesync()
{
	const int32_t frame = m_Calc.GetDesyncFrame();
	if (frame < 0) { m_ReportedDesync = -1; return; }
	if (frame == m_ReportedDesync) return;
	m_ReportedDesync = frame;
#ifdef _DEBUG
	hal::dout << "DESYNC: " << m_Calc.GetDesyncPeer() + 1 << "P frame " << frame << std::endl;
#endif
}
