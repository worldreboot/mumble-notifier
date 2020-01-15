#define WIN32_LEAN_AND_MEAN

#include "wintoastlib.h"

#include <windows.h>
#include <shellapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <windows.ui.notifications.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib,"shlwapi")
#pragma comment(lib,"user32")

#define DEFAULT_BUFLEN 128
#define DEFAULT_IP "INSERT MUMBLE SERVER IP HERE"
#define DEFAULT_PORT "INSERT MUMBLE SERVER PORT HERE"

using namespace std;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Foundation;
using namespace WinToastLib;

class CustomHandler : public IWinToastHandler {
public:
	void toastActivated() const {
	}

	void toastActivated(int actionIndex) const {
	}

	void toastDismissed(WinToastDismissalReason state) const {
		switch (state) {
		case UserCanceled:
			break;
		case TimedOut:
			break;
		case ApplicationHidden:
			break;
		default:
			break;
		}
	}

	void toastFailed() const {
		std::wcout << L"Error showing current toast" << std::endl;
	}
};

int wmain()
{
	if (!WinToast::isCompatible()) {
		wcerr << L"System is not compatible with Toast notifications\n";
		return 1;
	}

	WinToast::instance()->setAppName(L"Mumble Notifier");
	WinToast::instance()->setAppUserModelId(L"Mumble-Notifier-1");

	if (!WinToast::instance()->initialize()) {
		std::wcerr << L"Error, your system in not compatible!" << std::endl;
		return 1;
	}

	WinToastTemplate::AudioOption audioOption = WinToastTemplate::AudioOption::Default;

	WinToastTemplate templ(WinToastTemplate::Text01);
	templ.setAudioOption(audioOption);
	templ.setAttributionText(L"");

    WSADATA wsaData;
    
    int iresult;
    iresult = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (iresult != 0) {
        cout << "WSAStartup failed: " << iresult << '\n';
        return 1;
    }

    struct addrinfo *result = NULL, *ptr = NULL, hints;

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    iresult = getaddrinfo( DEFAULT_IP, DEFAULT_PORT, &hints, &result );
    if (iresult != 0) {
        cout << "getaddrinfo failed: " << iresult << '\n';
        WSACleanup();
        return 1;
    }

    SOCKET ConnectSocket = INVALID_SOCKET;

    ptr = result;

    ConnectSocket = socket( ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol );

    if (ConnectSocket == INVALID_SOCKET) {
        cout << "Error at socket(): " << WSAGetLastError() << '\n';
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iresult = connect( ConnectSocket, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen) );
    if (iresult == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        cout << "Unable to connect to server!\n";
        WSACleanup();
        return 1;
    }

    unsigned long mode = 1;
    iresult = ioctlsocket(ConnectSocket, FIONBIO, &mode);
    if (iresult != NO_ERROR) {
        printf("ioctlsocket failed with error: %d\n", iresult);
    }

    int recvbuflen = DEFAULT_BUFLEN;
    char recvbuf[DEFAULT_BUFLEN];

    const char sendbuf[12] = "\x00\x00\x00\x00\x33\x95\x00\x00\x8d\x6a\x00";

	wstring message = L"";
    int currentNumUsers = 0, previousNumUsers = 0;
    int WSALastError = 0;
    do {
		WSALastError = 0;
        send(ConnectSocket, sendbuf, sizeof(sendbuf), 0);
        iresult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (iresult > 0) {
            currentNumUsers = (recvbuf[12] << 24) | (recvbuf[13] << 16) | (recvbuf[14] << 8) | recvbuf[15];
        }
        else if (iresult == 0) {
            cout << "Connection closed\n";
        }
        else {
            WSALastError = WSAGetLastError();
        }
		if (currentNumUsers != previousNumUsers) {
			message = to_wstring(currentNumUsers) + L"\\10";
			templ.setTextField(message, WinToastTemplate::FirstLine);
			if (WinToast::instance()->showToast(templ, new CustomHandler()) < 0) {
				std::wcerr << L"Could not launch your toast notification!";
				return 1;
			}
		}
		previousNumUsers = currentNumUsers;
        Sleep(5000);
    } while (iresult > 0 || WSALastError == WSAEWOULDBLOCK);

    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}
