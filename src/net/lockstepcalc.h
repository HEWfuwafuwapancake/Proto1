#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>
#include "input/actioncalc.h"
#include "physics/versuscalc.h"

// 調整用の値（通信の版を変えるときは送受信側をそろえる）
/// @brief 通信の版の番号
static constexpr uint16_t LOCKSTEP_VERSION = 1;
/// @brief 指定がないときの入力の遅れ（フレーム）
static constexpr int LOCKSTEP_DEFAULT_DELAY = 3;
/// @brief 設定できる入力の遅れの上限（フレーム）
static constexpr int LOCKSTEP_MAX_DELAY = 120;
/// @brief 保管できるフレーム数（消すのは実行済みかつ全送信先が受領した分だけ）
static constexpr int LOCKSTEP_HISTORY = 256;
/// @brief 1パケットにまとめる入力の最大フレーム数
static constexpr int LOCKSTEP_PACKET_FRAMES = 8;
/// @brief チェックサムを比較する間隔（フレーム。比較の呼び出しは段階C）
static constexpr int LOCKSTEP_CHECKSUM_INTERVAL = 60;

/// @brief ビルドの種類（数値は通信に使う）
enum LOCKSTEP_BUILD : uint8_t
{
	LOCKSTEP_BUILD_DEBUG = 0, ///< Debug
	LOCKSTEP_BUILD_RELEASE = 1 ///< Release
};
#ifdef _DEBUG
/// @brief この実行ファイルのビルドの種類
static constexpr LOCKSTEP_BUILD LOCKSTEP_CURRENT_BUILD = LOCKSTEP_BUILD_DEBUG;
#else
/// @brief この実行ファイルのビルドの種類
static constexpr LOCKSTEP_BUILD LOCKSTEP_CURRENT_BUILD = LOCKSTEP_BUILD_RELEASE;
#endif

/// @brief 同じフレームの各スロットの入力
struct LOCKSTEP_FRAME
{
	int32_t Number = 0; ///< 試合開始を0とするフレーム番号
	uint8_t Mask = 0; ///< 入力が存在するスロットのビット
	PLAYER_INPUT Inputs[FIGHTER_COUNT]; ///< スロット番号順の入力（Maskにない分は無入力）
};

/// @brief 未受領の入力と、相手から受領した連続フレーム番号をまとめたパケット
struct LOCKSTEP_PACKET
{
	uint16_t Version = LOCKSTEP_VERSION; ///< 通信の版
	LOCKSTEP_BUILD Build = LOCKSTEP_CURRENT_BUILD; ///< 送信側のビルド
	uint8_t Mask = 0; ///< 各フレームに含むスロット（参加側は自分、ホストは人全員）
	int Delay = LOCKSTEP_DEFAULT_DELAY; ///< ホストが決める入力の遅れ
	int32_t Acknowledged = -1; ///< 相手から抜けなく受領した最後のフレーム（まだなしは-1）
	int Count = 0; ///< Framesの有効数（0〜8。0なら受領確認だけ）
	LOCKSTEP_FRAME Frames[LOCKSTEP_PACKET_FRAMES]; ///< フレーム番号の昇順の入力
};

/// @brief ずれの見張りに使う1体分の状態（ダミーも含めてスロット順で渡す）
struct LOCKSTEP_STATE
{
	VEC3 Position = {}; ///< 足元の位置
	float Percent = 0.0f; ///< ダメージ%
	int32_t Score = 0; ///< KO数
};

/// @brief 入力の保管・遅延・再送・パケット変換を行う計算（Windows・DirectX・gameに依存しない）
class LockstepCalc
{
public:
	/// @brief 人のスロットと入力の遅れを設定し、先頭の遅延分を全員無入力にする（不正な設定はfalse）
	bool Initialize(uint8_t playerMask, int delay = LOCKSTEP_DEFAULT_DELAY);
	/// @brief 次に実行するフレームを取得する
	int32_t GetNextFrame() const { return m_NextFrame; }
	/// @brief 設定した入力の遅れを取得する
	int GetDelay() const { return m_Delay; }
	/// @brief 指定フレーム・スロットの入力を保管する（同一入力の再送は成功。矛盾・範囲外・満杯はfalse）
	bool StoreInput(int32_t frame, int slot, const PLAYER_INPUT& input);
	/// @brief 次の実行フレーム＋遅延へ自分の入力を保管する（待機中は同じ入力を再送する）
	bool StoreLocalInput(int slot, const PLAYER_INPUT& input);
	/// @brief 次のフレームに必要な全員の入力がそろったか取得する
	bool IsReady() const;
	/// @brief 保管しているフレームを取得する（なければfalse）
	bool GetFrame(int32_t frame, LOCKSTEP_FRAME& result) const;
	/// @brief 全員分がそろったときだけ次のフレームへ進む（進めたらtrue。入力は再送用に残す）
	bool Advance();
	/// @brief 指定スロット全員から、0から抜けなく受領した最後のフレームを取得する（未受領は-1）
	int32_t GetReceivedThrough(uint8_t mask) const;
	/// @brief 相手が未受領の入力を古い順に最大8フレーム選ぶ（要求スロットがそろった分だけ）
	/// @param acknowledged 相手が受領した連続フレーム番号。作るパケットのAcknowledgedは呼び出し側で設定する
	LOCKSTEP_PACKET SelectUnacknowledged(int32_t acknowledged, uint8_t mask) const;
	/// @brief 全送信先の受領確認の最小値を渡して、実行済みの保管分だけを消す
	void DiscardAcknowledged(int32_t acknowledged);
	/// @brief パケットをパディングに依存しないリトルエンディアンに変換する（不正な内容ならfalse）
	static bool EncodePacket(const LOCKSTEP_PACKET& packet, std::vector<uint8_t>& bytes);
	/// @brief 長さ・版・値を確かめてパケットを読む（失敗時はresultを変えない。ビルドの一致は接続側で確認する）
	static bool DecodePacket(const uint8_t* bytes, size_t size, LOCKSTEP_PACKET& result);
	/// @brief 全4体の位置・%・KO数から同じ並びでチェックサムを作る（構造体の余白は使わない）
	static uint32_t CalculateChecksum(const LOCKSTEP_STATE (&states)[FIGHTER_COUNT]);

private:
	uint8_t m_PlayerMask = 0;
	int m_Delay = LOCKSTEP_DEFAULT_DELAY;
	int32_t m_NextFrame = 0;
	int32_t m_Received[FIGHTER_COUNT] = { -1, -1, -1, -1 };
	std::map<int32_t, LOCKSTEP_FRAME> m_Frames;
};
