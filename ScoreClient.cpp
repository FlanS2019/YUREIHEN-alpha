#pragma execution_character_set("utf-8")

// winsock2.h は windows.h より必ず先にインクルードする
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#include "ScoreClient.h"
#include "debug_ostream.h"

// ★ サーバーのIPアドレスとポートをここで設定
static const char* SERVER_IP   = "192.168.10.19";
static const int   SERVER_PORT = 5000;

bool Score_SendToServer(int score)
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		hal::dout << "[ScoreClient] WSAStartup failed" << std::endl;
		return false;
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		hal::dout << "[ScoreClient] socket failed" << std::endl;
		WSACleanup();
		return false;
	}

	sockaddr_in serverAddr{};
	serverAddr.sin_family      = AF_INET;
	serverAddr.sin_port        = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

	if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		hal::dout << "[ScoreClient] connect failed. Error: " << WSAGetLastError() << std::endl;
		closesocket(sock);
		WSACleanup();
		return false;
	}

	// int をそのまま送信（サーバーと同じ形式）
	int totalSent = 0;
	while (totalSent < (int)sizeof(int))
	{
		int sent = send(sock,
			(char*)&score + totalSent,
			sizeof(int) - totalSent,
			0);
		if (sent == SOCKET_ERROR)
		{
			hal::dout << "[ScoreClient] send failed" << std::endl;
			closesocket(sock);
			WSACleanup();
			return false;
		}
		totalSent += sent;
	}

	hal::dout << "[ScoreClient] Score sent: " << score << std::endl;

	closesocket(sock);
	WSACleanup();
	return true;
}