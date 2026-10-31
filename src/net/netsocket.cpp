#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include "net/netsocket.h"
#include <climits>

#pragma comment(lib, "ws2_32.lib")

NetSocket::~NetSocket()
{
	Finalize();
}

bool NetSocket::Initialize(uint16_t port)
{
	Finalize();
	m_Error = 0;
	WSADATA data{};
	m_Error = WSAStartup(MAKEWORD(2, 2), &data);
	if (m_Error != 0) return false;
	m_Started = true;
	m_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_Socket == INVALID_SOCKET) { m_Error = WSAGetLastError(); Finalize(); return false; }
	// 同じPCの2つ目のホストが同じポートを共有しないようにする。
	const BOOL exclusive = TRUE;
	u_long nonblocking = 1;
	sockaddr_in local{};
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	local.sin_port = htons(port);
	if (setsockopt(m_Socket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&exclusive), sizeof(exclusive)) != 0
		|| bind(m_Socket, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) != 0
		|| ioctlsocket(m_Socket, FIONBIO, &nonblocking) != 0) {
		m_Error = WSAGetLastError();
		Finalize();
		return false;
	}
	return true;
}

void NetSocket::Finalize()
{
	if (m_Socket != INVALID_SOCKET) closesocket(m_Socket);
	m_Socket = INVALID_SOCKET;
	if (m_Started) WSACleanup();
	m_Started = false;
}

bool NetSocket::Send(const NET_ADDRESS& address, const uint8_t* bytes, size_t size)
{
	if (m_Socket == INVALID_SOCKET || bytes == nullptr || size > INT_MAX) return false;
	sockaddr_in to{};
	to.sin_family = AF_INET;
	to.sin_addr.s_addr = address.Address;
	to.sin_port = htons(address.Port);
	const int sent = sendto(m_Socket, reinterpret_cast<const char*>(bytes), static_cast<int>(size), 0,
		reinterpret_cast<const sockaddr*>(&to), sizeof(to));
	if (sent == SOCKET_ERROR) m_Error = WSAGetLastError();
	return sent == static_cast<int>(size);
}

int NetSocket::Receive(NET_ADDRESS& address, uint8_t* bytes, size_t capacity)
{
	if (m_Socket == INVALID_SOCKET || bytes == nullptr || capacity > INT_MAX) return -1;
	sockaddr_in from{};
	int length = sizeof(from);
	const int received = recvfrom(m_Socket, reinterpret_cast<char*>(bytes), static_cast<int>(capacity), 0,
		reinterpret_cast<sockaddr*>(&from), &length);
	if (received == SOCKET_ERROR) {
		m_Error = WSAGetLastError();
		return m_Error == WSAEWOULDBLOCK ? 0 : -1;
	}
	address = { from.sin_addr.s_addr, ntohs(from.sin_port) };
	return received == 0 ? -1 : received;
}

bool NetSocket::ParseAddress(const std::string& ip, uint16_t port, NET_ADDRESS& result)
{
	in_addr address{};
	if (inet_pton(AF_INET, ip.c_str(), &address) != 1) return false;
	result = { address.s_addr, port };
	return true;
}

bool NetSocket::SameAddress(const NET_ADDRESS& a, const NET_ADDRESS& b)
{
	return a.Address == b.Address && a.Port == b.Port;
}

std::string NetSocket::GetLocalAddresses()
{
	WSADATA data{};
	if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return "127.0.0.1";
	char name[256]{};
	if (gethostname(name, sizeof(name)) != 0) { WSACleanup(); return "127.0.0.1"; }
	addrinfo hints{};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	addrinfo* addresses = nullptr;
	if (getaddrinfo(name, nullptr, &hints, &addresses) != 0) { WSACleanup(); return "127.0.0.1"; }
	std::string text;
	for (auto* address = addresses; address != nullptr; address = address->ai_next) {
		char ip[INET_ADDRSTRLEN]{};
		const auto* ipv4 = reinterpret_cast<const sockaddr_in*>(address->ai_addr);
		if (inet_ntop(AF_INET, &ipv4->sin_addr, ip, sizeof(ip)) == nullptr) continue;
		if (!text.empty()) text += " / ";
		text += ip;
	}
	freeaddrinfo(addresses);
	WSACleanup();
	return text.empty() ? "127.0.0.1" : text;
}
