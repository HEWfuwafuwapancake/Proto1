#pragma once

#include "net/lockstepcalc.h"
#include <deque>
#include <string>

// 調整用の値（自動の入力の遅れ）
/// @brief 入力の遅れを ping から自動で決めることを表す値（起動の引数に --delay がないとき）
static constexpr int LOCKSTEP_AUTO_DELAY = -1;
/// @brief 自動の入力の遅れで、ping の分に足す余裕（フレーム。通信のゆれの分）
static constexpr int AUTO_DELAY_MARGIN = 2;
/// @brief 自動の入力の遅れの下限（フレーム）
static constexpr int AUTO_DELAY_MIN = 3;
/// @brief 自動の入力の遅れの上限（フレーム）
static constexpr int AUTO_DELAY_MAX = 20;
/// @brief 自動の入力の遅れに使う ping の期間（ミリ秒。この間の AUTO_DELAY_PING_PERCENTILE の値を使う）
static constexpr uint64_t AUTO_DELAY_PING_WINDOW = 2000;
/// @brief 自動の入力の遅れに使う ping の位置（%。小さい順に並べたこの位置の値。たまに遅れた分は使わない）
static constexpr int AUTO_DELAY_PING_PERCENTILE = 90;

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
	int Delay = LOCKSTEP_AUTO_DELAY; ///< ホストが配る入力遅延（LOCKSTEP_AUTO_DELAY なら試合の開始時に ping から決める）
};

/// @brief 起動引数を読み取る（不明な引数・重複・値の不足はfalse。失敗時は出力を変えない）
bool SessionCalcParseOptions(const std::string& commandLine, NET_OPTIONS& result);

/// @brief いちばん遅い相手との往復時間（ミリ秒）から入力の遅れ（フレーム）を決める（未計測の負の値は下限）
int SessionCalcAutoDelay(int pingMilliseconds);

/// @brief 4つの区切りで IPv4 を編集・検査する（DirectX・Windows 不使用）
class SessionAddressCalc
{
public:
	/// @brief 自分の IPv4 の前3区切りを入れ、最後の空欄を選ぶ（取得不可なら全欄を空にする）
	void Initialize(const std::string& localAddress);
	/// @brief 選択中の区切りへ数字を1つ足す（3桁入力済みなら次の区切りへ進む）
	void AppendDigit(int digit);
	/// @brief 選択中の文字を1つ消す（空欄なら前の区切りへ。全欄が空なら false）
	bool Erase();
	/// @brief 選ぶ区切りを左右へ動かす（負：左、正：右）
	void Move(int direction);
	/// @brief 選んだ値を 0〜255 の中で増減する（負：減らす、正：増やす）
	void Adjust(int direction);
	/// @brief 全区切りが入力済みで範囲内か取得する
	bool IsValid() const;
	/// @brief IPv4 の文字列を取得する（先頭の0は取り除き、未入力は空欄のまま）
	std::string GetText() const;
	/// @brief 選んでいる区切りの番号を取得する
	int GetCursor() const { return m_Cursor; }
	/// @brief 指定した区切りの文字を取得する（番号は 0〜3）
	const std::string& GetPart(int index) const { return m_Parts[index]; }

private:
	std::string m_Parts[4];
	int m_Cursor = 0;
};

/// @brief セッション用メッセージの種類（入力パケットとは別の識別子を使う）
enum SESSION_MESSAGE_TYPE : uint8_t
{
	SESSION_JOIN, ///< スロットの要求
	SESSION_READY, ///< 自分の機器を選択済み
	SESSION_STATE, ///< ホストの顔ぶれ・開始状態
	SESSION_INPUTS, ///< セッションと試合番号を付けた入力
	SESSION_REJECT ///< 版・ビルドの不一致で参加を拒否
};

/// @brief 接続を続けられない理由
enum SESSION_FAILURE
{
	SESSION_FAILURE_NONE, ///< 接続を継続
	SESSION_FAILURE_VERSION, ///< 通信の版が異なる
	SESSION_FAILURE_BUILD, ///< DebugとReleaseが異なる
	SESSION_FAILURE_TIMEOUT ///< ホストから3秒受信していない
};

/// @brief 接続と入力を運ぶ共通の封筒（構造体そのものは送らない）
struct SESSION_MESSAGE
{
	SESSION_MESSAGE_TYPE Type = SESSION_JOIN; ///< メッセージの種類
	uint16_t Version = LOCKSTEP_VERSION; ///< 接続時に比較する版
	LOCKSTEP_BUILD Build = LOCKSTEP_CURRENT_BUILD; ///< 接続時に比較するビルド
	uint32_t Session = 0; ///< ホストの起動ごとに変わる番号
	uint32_t Nonce = 0; ///< 送信元、またはホストからの宛先の起動番号
	uint32_t Round = 0; ///< TITLEから開始するたびに増える試合番号（RETRYは同じ）
	int Slot = 0; ///< 参加側のスロット（1〜3）
	uint8_t Mask = 0; ///< 接続済みのスロット
	uint8_t Ready = 0; ///< 機器選択済みのスロット
	int Delay = LOCKSTEP_DEFAULT_DELAY; ///< ホストが決めた遅延
	bool Running = false; ///< true：ロックステップ中、false：ロビー
	uint32_t Stamp = 0; ///< 送信元の単調時計のミリ秒（下位32ビット）
	uint32_t Echo = 0; ///< 相手から最後に受け取ったStamp（往復時間用）
	int32_t CheckFrame = -1; ///< チェックサムの実行済みフレーム（なしは-1）
	uint32_t Checksum = 0; ///< CheckFrameの全4体のチェックサム
	int32_t CheckAck = -1; ///< 相手のチェックサムを比較できた最後のフレーム
	LOCKSTEP_PACKET Packet; ///< SESSION_INPUTSだけで使う入力と方向別ACK
};

/// @brief ロビー・入力の中継・方向別ACKを計算する（ソケット・Windows・DirectX不要）
class SessionCalc
{
public:
	/// @brief 起動設定と0以外の起動番号で初期化する
	void Initialize(const NET_OPTIONS& options, uint32_t nonce);
	/// @brief ホストが参加を受け付け、スロットを返す（満員・開始後の新規参加は-1）
	int Join(uint32_t nonce, uint16_t version = LOCKSTEP_VERSION, LOCKSTEP_BUILD build = LOCKSTEP_CURRENT_BUILD);
	/// @brief 同じ版・ビルドか調べ、違うときの理由を返す
	static SESSION_FAILURE CheckCompatibility(const SESSION_MESSAGE& message);
	/// @brief 単調時計のミリ秒を設定する（受信の直前に呼ぶ。Windowsの時計は呼び出し元で読む）
	void SetTime(uint64_t milliseconds) { m_Now = milliseconds; }
	/// @brief 今の時刻から3秒受信のない相手を切断する（受信処理のあとに呼ぶ）
	void CheckTimeouts();
	/// @brief 実行直後の全4体の状態を記録する（60フレームごとだけ計算・比較）
	void RecordChecksum(int32_t frame, const LOCKSTEP_STATE (&states)[FIGHTER_COUNT]);
	/// @brief 接続終了の理由を取得する（参加側だけ）
	SESSION_FAILURE GetFailure() const { return m_Failure; }
	/// @brief タイムアウトしたスロットのビットを取得する（試合の顔ぶれは保つ）
	uint8_t GetDisconnectedMask() const { return m_Disconnected; }
	/// @brief 相手との往復時間をミリ秒で取得する（まだ計測できなければ-1）
	int GetPing(int peer) const { return m_Ping[peer]; }
	/// @brief 入力を待ってゲームの計算を止めた更新回数を取得する
	uint64_t GetWaitFrames() const { return m_WaitFrames; }
	/// @brief 最初にずれを見つけたフレームを取得する（ずれなしは-1）
	int32_t GetDesyncFrame() const { return m_DesyncFrame; }
	/// @brief 最初にずれを見つけた相手のスロットを取得する
	int GetDesyncPeer() const { return m_DesyncPeer; }
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
	/// @brief ホストが入力の遅れを ping から自動で決めるか取得する（参加側は常にfalse）
	bool IsAutoDelay() const { return m_Host && m_AutoDelay; }
	/// @brief 自動の入力の遅れに使う往復時間をミリ秒で取得する（人ごとの直近2秒の90%の値の、いちばん遅い人の値。未計測は-1）
	int GetAutoDelayPing() const;
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
	void ReceiveMonitor(int peer, const SESSION_MESSAGE& message);
	void CompareChecksums();
	bool IsActivePeer(int peer) const;
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
	uint64_t m_Now = 0;
	uint64_t m_LastReceived[FIGHTER_COUNT] = {};
	uint32_t m_Echo[FIGHTER_COUNT] = {};
	uint32_t m_LastEcho[FIGHTER_COUNT] = {}; // 前に往復時間を測った Echo（同じ Echo の再送は測らない）。
	int m_Ping[FIGHTER_COUNT] = { -1, -1, -1, -1 };
	std::deque<std::pair<uint64_t, int>> m_PingHistory[FIGHTER_COUNT]; // 受け取った時刻と往復時間。
	bool m_AutoDelay = false;
	uint8_t m_Disconnected = 0;
	SESSION_FAILURE m_Failure = SESSION_FAILURE_NONE;
	uint64_t m_WaitFrames = 0;
	int32_t m_DesyncFrame = -1;
	int m_DesyncPeer = -1;
	std::map<int32_t, uint32_t> m_Checksums;
	std::map<int32_t, uint32_t> m_RemoteChecksums[FIGHTER_COUNT];
	int32_t m_CheckAcknowledged[FIGHTER_COUNT] = { -1, -1, -1, -1 };
	int32_t m_CheckedThrough[FIGHTER_COUNT] = { -1, -1, -1, -1 };
	LockstepCalc m_Local; // 自分が採取した入力。参加側ではホストの返送を待つ。
	LockstepCalc m_Shared; // 全員で実行する入力。参加側ではホストからの分だけ。
};
