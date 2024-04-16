#undef UNICODE

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cctype>

using namespace std;

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

bool isNumber(const string& s) {
    if (s.empty()) {
        return false;
    }
    bool hasDigit = false;
    bool hasDecimal = false;
    bool hasSign = false;
    for (char c : s) {
        if (isdigit(c)) {
            hasDigit = true;
        } else if (c == '.') {
            if (hasDecimal || !hasDigit) {
                return false;
            }
            hasDecimal = true;
        } else if (c == '-' && !hasDigit && !hasSign) {
            hasSign = true;
        } else if (c == '+' && !hasDigit && !hasSign) {
            hasSign = true;
        } else {
            return false;
        }
    }
    return hasDigit;
}

vector<string> splitBySpace(const string& input) {
    vector<string> result;
    stringstream ss(input);
    string token;
    while (ss >> token) {
        result.push_back(token);
    }
    return result;
}

vector<double> convertToNumbers(const vector<string>& substrings) {
    vector<double> numbers;
    for (const string& substr : substrings) {
        if (isNumber(substr)) {
            numbers.push_back(stod(substr));
        }
    }
    return numbers;
}

double add(const vector<double>& nums) {
    double result = 0;
    for (double num : nums) {
        result += num;
    }
    return result;
}

double multiply(const vector<double>& nums) {
    double result = 1;
    for (double num : nums) {
        result *= num;
    }
    return result;
}

string concatenate(const vector<string>& substrings) {
    string result;
    for (const string& substr : substrings) {
        result += substr;
    }
    return result;
}

int main()
{
    WSADATA wsaData;
    int iResult;

    auto ListenSocket = INVALID_SOCKET;
    auto ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = nullptr;
    struct addrinfo hints{};

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(nullptr, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Set up the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, nullptr, nullptr);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    closesocket(ListenSocket);

    struct sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);
    getpeername(ClientSocket, (struct sockaddr*)&clientAddr, &addrLen);

    cout << "client's port: " << ntohs(clientAddr.sin_port) << endl;

    // Receive until the peer shuts down the connection
    do {

        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            printf("Bytes received: %d\n", iResult);
            recvbuf[iResult] = '\0';
            cout << "The message received from the client: " << recvbuf << endl;
            string receivedInfo(recvbuf, iResult - 1);
            char operation = recvbuf[iResult - 1];

            vector<string> substrings = splitBySpace(receivedInfo);
            string infoToSend;
            if (operation == '1') {
                infoToSend = concatenate(substrings);
            } else {
                vector<double> numbers = convertToNumbers(substrings);
                if (operation == '2') {
                    infoToSend = to_string(add(numbers));
                } else if (operation == '3') {
                    infoToSend = to_string(multiply(numbers));
                }
            }

            const char* recvbuf = infoToSend.c_str();
            cout << "The message received from the client and counted by server: " << recvbuf << endl;
            // Echo the buffer back to the sender
            iResult = strlen(recvbuf);
            iSendResult = send( ClientSocket, recvbuf, iResult, 0 );
            if (iSendResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }
            printf("Bytes sent: %d\n", iSendResult);
        }
        else if (iResult == 0)
            printf("Connection closing...\n");
        else  {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }

    } while (iResult > 0);

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}