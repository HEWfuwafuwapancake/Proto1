#pragma once

#include "net/lockstepcalc.h"
#include <string>

/// @brief 起動時の対戦方法
enum NET_MODE
{
	NET_MODE_LOCAL, ///< このPCだけで遊ぶ
	NET_MODE_HOST, ///< 1Pとして参加を待つ
	NET_MODE_JOIN ///< ホストへ参加する
};

/// @brief Windowsに依存しない起動設定
struct NET_OPTIONS
{
	NET_MODE Mode = NET_MODE_LOCAL; ///< 対戦方法
	std::string Host; ///< 参加先のIPv4アドレス
	int Delay = LOCKSTEP_DEFAULT_DELAY; ///< ホストが配る入力遅延
};

/// @brief 起動引数を読み取る（不明な引数・重複・値の不足はfalse。失敗時は出力を変えない）
bool SessionCalcParseOptions(const std::string& commandLine, NET_OPTIONS& result);

/// @brief セッション用メッセージの種類（入力パケットとは別の識別子を使う）
enum SESSION_MESSAGE_TYPE : uint8_t
{
	SESSION_JOIN, ///< スロットの要求
	SESSION_READY, ///< 自分の機器を選択済み
	SESSION_STATE, ///< ホストの顔ぶれ・開始状態
	SESSION_INPUTS ///< セッションと試合番号を付けた入力
};

/// @brief 接続と入力を運ぶ共通の封筒（構造体そのものは送らない）
struct SESSION_MESSAGE
{
	SESSION_MESSAGE_TYPE Type = SESSION_JOIN; ///< メッセージの種類
	uint32_t Session = 0; ///< ホストの起動ごとに変わる番号
	uint32_t Nonce = 0; ///< 送信元、またはホストからの宛先の起動番号
	uint32_t Round = 0; ///< TITLEから開始するたびに増える試合番号（RETRYは同じ）
	int Slot = 0; ///< 参加側のスロット（1〜3）
	uint8_t Mask = 0; ///< 接続済みのスロット
	uint8_t Ready = 0; ///< 機器選択済みのスロット
	int Delay = LOCKSTEP_DEFAULT_DELAY; ///< ホストが決めた遅延
	bool Running = false; ///< true：ロックステップ中、false：ロビー
	LOCKSTEP_PACKET Packet; ///< SESSION_INPUTSだけで使う入力と方向別ACK
};

/// @brief ロビー・入力の中継・方向別ACKを計算する（ソケット・Windows・DirectX不要）
class SessionCalc
{
public:
	/// @brief 起動設定と0以外の起動番号で初期化する
	void Initialize(const NET_OPTIONS& options, uint32_t nonce);
	/// @brief ホストが参加を受け付け、スロットを返す（満員・開始後の新規参加は-1）
	int Join(uint32_t nonce);
	/// @brief 選択済みにする、または全員選択済みならホストの再決定で開始する
	void Decide();
	/// @brief 送信元のスロットとメッセージを受け取る（参加側にとってホストは0）
	void Receive(int peer, const SESSION_MESSAGE& message);
	/// @brief 毎回送り直す接続・ロビー・開始状態を作る（ホストは宛先スロットを渡す）
	SESSION_MESSAGE MakeControl(int peer = 0) const;
	/// @brief 未受領の入力と、この送信先から受領した累積ACKを作る
	SESSION_MESSAGE MakeInputs(int peer = 0) const;
	/// @brief この実行フレームの入力をまだ採取していないか取得する
	bool NeedsInput() const;
	/// @brief 未採取なら一度だけ入力を固定する（保管が満杯でも採取値は保つ）
	void CaptureInput(const PLAYER_INPUT& input);
	/// @brief 全員分と自分の先行入力が保管できたら、1フレーム分を取り出して進める
	bool TakeFrame(LOCKSTEP_FRAME& frame);
	/// @brief TITLEへ戻る（接続・機器・最終入力の再送履歴は保つ）
	void ReturnToLobby();
	/// @brief ホストか取得する
	bool IsHost() const { return m_Host; }
	/// @brief ロックステップ中か取得する
	bool IsRunning() const { return m_Running; }
	/// @brief 再送する試合の履歴を持っているか取得する
	bool HasRound() const { return m_HasRound; }
	/// @brief 自分のスロットを取得する（未接続は-1）
	int GetLocalSlot() const { return m_Slot; }
	/// @brief 接続済みスロットのビットを取得する
	uint8_t GetMask() const { return m_Mask; }
	/// @brief 機器選択済みスロットのビットを取得する
	uint8_t GetReadyMask() const { return m_Ready; }
	/// @brief ホストが配った入力遅延を取得する
	int GetDelay() const { return m_Delay; }
	/// @brief 次に実行するフレームを取得する
	int32_t GetNextFrame() const { return m_Shared.GetNextFrame(); }
	/// @brief メッセージを明示的なリトルエンディアンへ変換する
	static bool Encode(const SESSION_MESSAGE& message, std::vector<uint8_t>& bytes);
	/// @brief 長さ・種類・値を確認して読む（失敗時は出力を変えない）
	static bool Decode(const uint8_t* bytes, size_t size, SESSION_MESSAGE& result);

private:
	void BeginRound(uint32_t round);
	void StoreCaptured();
	void DiscardHistory();
	bool CanStart() const;
	bool m_Host = false;
	bool m_Running = false;
	bool m_HasRound = false;
	bool m_Locked = false;
	bool m_LocalReady = false;
	int m_Slot = -1;
	int m_Delay = LOCKSTEP_DEFAULT_DELAY;
	uint8_t m_Mask = 0;
	uint8_t m_Ready = 0;
	uint32_t m_Nonce = 0;
	uint32_t m_Session = 0;
	uint32_t m_Round = 0;
	uint32_t m_PeerNonces[FIGHTER_COUNT] = {};
	int32_t m_Acknowledged[FIGHTER_COUNT] = { -1, -1, -1, -1 };
	int32_t m_CapturedFrame = -1;
	bool m_CapturedStored = false;
	PLAYER_INPUT m_Captured;
	LockstepCalc m_Local; // 自分が採取した入力。参加側ではホストの返送を待つ。
	LockstepCalc m_Shared; // 全員で実行する入力。参加側ではホストからの分だけ。
};
