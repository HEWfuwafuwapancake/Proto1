#include "game/netsession.h"
#include "input/action.h"
#include <random>
#include <sstream>

// 調整用の値（全PCで同じポート。参加側自身は空きポートを使う）
static constexpr uint16_t SESSION_PORT = 27026;
static constexpr int MAX_RECEIVE_PER_UPDATE = 64;
static constexpr size_t RECEIVE_BYTES = 1024;

bool NetSession::Initialize(const NET_OPTIONS& options)
{
	m_Options = options;
	m_Device = -1;
	m_Error.clear();
	if (!IsOnline()) return true;
	if (!m_Socket.Initialize(options.Mode == NET_MODE_HOST ? SESSION_PORT : 0)) {
		m_Error = "UDPを開けませんでした。Winsock: " + std::to_string(m_Socket.GetError());
		return false;
	}
	if (options.Mode == NET_MODE_JOIN && !NetSocket::ParseAddress(options.Host, SESSION_PORT, m_Peers[0])) {
		m_Error = "参加先のIPv4アドレスを確認してください。";
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
			if (peer >= 0) continue; // 開始前の再要求には、下の毎フレームの状態通知で応える。
			const int slot = m_Calc.Join(message.Nonce);
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
	Receive();
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
	if (!IsOnline()) return {};
	int count = 0, ready = 0;
	for (int slot = 0; slot < FIGHTER_COUNT; ++slot) {
		if (m_Calc.GetMask() & (1u << slot)) ++count;
		if (m_Calc.GetReadyMask() & (1u << slot)) ++ready;
	}
	std::ostringstream text;
	text << (m_Calc.IsHost() ? "HOST " : "JOIN ") << m_Addresses;
	if (m_Calc.GetLocalSlot() < 0) text << " 接続待ち";
	else text << " 自分 " << m_Calc.GetLocalSlot() + 1 << "P  " << count << "人（機器選択 " << ready << "人）";
	text << " delay " << m_Calc.GetDelay() << (m_Calc.IsRunning() ? " 対戦中" : " 待機中");
	return text.str();
}
