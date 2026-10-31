#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

/// @brief IPv4の送受信先（Addressはネットワーク順、Portはホスト順）
struct NET_ADDRESS
{
	uint32_t Address = 0; ///< IPv4アドレス
	uint16_t Port = 0; ///< UDPポート番号
};

/// @brief Windows標準Winsockの止まらないUDP（game・DirectXには依存しない）
class NetSocket
{
public:
	/// @brief 未接続のソケットを作る
	NetSocket() = default;
	/// @brief 閉じ忘れたソケットを終了する
	~NetSocket();
	/// @brief ソケットの二重解放を避けるためコピーを禁止する
	NetSocket(const NetSocket&) = delete;
	/// @brief ソケットの二重解放を避けるためコピー代入を禁止する
	NetSocket& operator=(const NetSocket&) = delete;
	/// @brief UDPを開く（portが0なら空きポート。成功時true）
	bool Initialize(uint16_t port);
	/// @brief ソケットを閉じ、Winsockを終了する（繰り返してもよい）
	void Finalize();
	/// @brief 指定先へ送る（送信できたらtrue。待たない）
	bool Send(const NET_ADDRESS& address, const uint8_t* bytes, size_t size);
	/// @brief 1パケットを受け取る（正数：長さ、0：未着、-1：破棄またはエラー。待たない）
	int Receive(NET_ADDRESS& address, uint8_t* bytes, size_t capacity);
	/// @brief IPv4の文字列を送信先に変換する（失敗時false）
	static bool ParseAddress(const std::string& ip, uint16_t port, NET_ADDRESS& result);
	/// @brief 同じIPとポートか取得する
	static bool SameAddress(const NET_ADDRESS& a, const NET_ADDRESS& b);
	/// @brief このPCのIPv4一覧を取得する（接続前にも呼べる。区切りは " / "）
	static std::string GetLocalAddresses();
	/// @brief 最後のWinsockエラー番号を取得する
	int GetError() const { return m_Error; }

private:
	uintptr_t m_Socket = ~uintptr_t(0);
	bool m_Started = false;
	int m_Error = 0;
};
