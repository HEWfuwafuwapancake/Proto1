#pragma once

#include "net/netsocket.h"
#include "net/sessioncalc.h"
#include <chrono>

/// @brief Gameが所有するUDP接続・機器選択・入力採取（計算はSessionCalcに任せる）
class NetSession
{
public:
	/// @brief 起動設定で接続を準備する（ローカルは何もしない。失敗時false）
	bool Initialize(const NET_OPTIONS& options);
	/// @brief UDP接続を終了する
	void Finalize();
	/// @brief 受信・ロビーの機器選択・未採取の入力の固定・再送を行う（待たない）
	void Update();
	/// @brief 全員の入力が届いたら1フレームを取り出す
	bool TakeFrame(LOCKSTEP_FRAME& frame) { return m_Calc.TakeFrame(frame); }
	/// @brief 実行直後の全4体の状態を渡し、60フレームごとにずれを見張る
	void RecordChecksum(int32_t frame, const LOCKSTEP_STATE (&states)[FIGHTER_COUNT]);
	/// @brief 接続を保ってロビーへ戻る
	void ReturnToLobby() { m_Calc.ReturnToLobby(); }
	/// @brief オンラインで起動したか取得する
	bool IsOnline() const { return m_Options.Mode != NET_MODE_LOCAL; }
	/// @brief ロックステップ中か取得する
	bool IsRunning() const { return m_Calc.IsRunning(); }
	/// @brief 自分のスロットを取得する（未接続は-1）
	int GetLocalSlot() const { return m_Calc.GetLocalSlot(); }
	/// @brief 自分が選んだ機器を取得する（未選択は-1）
	int GetLocalDevice() const { return m_Device; }
	/// @brief 接続済みスロットのビットを取得する
	uint8_t GetMask() const { return m_Calc.GetMask(); }
	/// @brief 機器選択済みスロットのビットを取得する
	uint8_t GetReadyMask() const { return m_Calc.GetReadyMask(); }
	/// @brief タイトルバーに表示する接続状態を取得する
	std::string GetStatusText() const;
	/// @brief 起動失敗の説明を取得する
	const std::string& GetError() const { return m_Error; }

private:
	void Receive();
	void Send(const NET_ADDRESS& address, const SESSION_MESSAGE& message);
	void ReportDesync();
	static const char* FailureText(SESSION_FAILURE failure);
	NET_OPTIONS m_Options;
	NetSocket m_Socket;
	SessionCalc m_Calc;
	NET_ADDRESS m_Peers[FIGHTER_COUNT];
	int m_Device = -1;
	std::string m_Addresses;
	std::string m_Error;
	std::chrono::steady_clock::time_point m_Started;
	int32_t m_ReportedDesync = -1;
};
