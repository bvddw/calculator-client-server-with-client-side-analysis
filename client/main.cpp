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
        }
        else {
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


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int main(int argc, char **argv)
{
    string input1, input2;
    cout << "Enter the first row of numbers or strings separated by space: ";
    getline(cin, input1);
    cout << "Enter the operation (add or mult): ";
    getline(cin, input2);

    if (input2 != "add" && input2 != "mult") {
        cout << "operation error";
        return 1;
    }

    vector<string> substrings = splitBySpace(input1);

    bool flag = true;
    for (const string& substr : substrings) {
        if (!isNumber(substr)) {
            flag = false;
        }
    }

    if (!flag && input2 == "mult") {
        cout << "Error. Mult can operate only with numbers.";
        return 1;
    }

    if (input2 == "add" && !flag) {
        input1 += '1'; // concatenation
    } else if (input2 == "add") {
        input1 += '2'; // addition
    } else {
        input1 += '3'; // multiplication
    }

    const char* stringToServer = input1.c_str();

    WSADATA wsaData;
    auto ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = nullptr,
            *ptr,
            hints{};
    const int bufferSize = 100;

    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;

    // Validate the parameters
    if (argc != 2) {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != nullptr ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                               ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %d\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
    iResult = send( ConnectSocket, stringToServer, (int)strlen(stringToServer), 0 );
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %d\n", iResult);

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection
    do {
        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if ( iResult > 0 ) {
            recvbuf[iResult] = '\0';
            cout << "The message, returned from server: " << recvbuf << endl;
            printf("Bytes received: %d\n", iResult);
        }
        else if ( iResult == 0 )
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while( iResult > 0 );

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}