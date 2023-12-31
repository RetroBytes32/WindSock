#define _WIN32_WINNT   0x0601

#include "windsock.h"


std::string IPAddress::str() {
    std::string addressString;
    std::stringstream sStream;
    
    sStream << (unsigned int)addr[0];
    addressString = sStream.str() + ".";
    sStream.str("");
    
    sStream << (unsigned int)addr[1];
    addressString += sStream.str() + ".";
    sStream.str("");
    
    sStream << (unsigned int)addr[2];
    addressString += sStream.str() + ".";
    sStream.str("");
    
    sStream << (unsigned int)addr[3];
    addressString += sStream.str();
    return addressString;
}



WindSock::WindSock(void) :
    
    mLastHost(""),
    mLastPort(0),
    mLastAddress({127, 0, 0, 1}),
    mLastIndex(0),
    
    mIsConnected(false),
    mSocket(0)
{
    
}

WindSock::~WindSock(void) {
    
    if (mSocket != INVALID_SOCKET) 
        closesocket(mSocket);
    
    WSACleanup();
}


unsigned int WindSock::GetLastPort(void) {
    return mLastPort;
}

std::string  WindSock::GetLastHost(void) {
    return mLastHost;
}

IPAddress WindSock::GetLastAddress(void) {
    return mLastAddress;
}

unsigned int WindSock::GetLastIndex(void) {
    return mLastIndex;
}

unsigned int WindSock::GetNumberOfHosts(void) {
    return mHostList.size();
}

std::string WindSock::GetHostIndex(unsigned int index) {
    return mHostList[index];
}

int WindSock::FindHost(std::string name) {
    for (unsigned int i=0; i < GetNumberOfHosts(); i++) 
        if (GetHostIndex(i) == name) return i;
    return -1;
}


unsigned int WindSock::GetNumberOfPorts(void) {
    return mPortList.size();
}

unsigned int WindSock::GetPortIndex(unsigned int index) {
    return mPortList[index];
}

unsigned int WindSock::GetNumberOfSockets(void) {
    return mSocketList.size();
}

SOCKET WindSock::GetSocketIndex(unsigned int index) {
    return mSocketList[index];
}

std::string WindSock::GetBufferString(unsigned int index) {
    return mBufferList[index];
}

void WindSock::ClearBufferString(unsigned int index) {
    mBufferList[index] = "";
}



int WindSock::GetNumberOfTimers(void) {
    return mTimeoutList.size();
}

int WindSock::GetTimerIndex(unsigned int index) {
    return mTimeoutList[index];
}

void WindSock::SetTimerValue(unsigned int index, int value) {
    mTimeoutList[index] = value;
}


int WindSock::InitiateServer(unsigned int port, unsigned int maxConn) {
    
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);
    
    int WSOK = WSAStartup(ver, &wsData);
    if (WSOK != 0) 
        return WSOK;
    
    mSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mSocket == INVALID_SOCKET) 
        return WSACleanup();
    
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    hint.sin_addr.S_un.S_addr = INADDR_ANY;
    
    bind(mSocket, (sockaddr*)&hint, sizeof(hint));
    listen(mSocket, maxConn);
    
    u_long sockMode = 1; // Non blocking listener
    ioctlsocket(mSocket, FIONBIO, &sockMode);
    
    if (mSocket == INVALID_SOCKET) 
        return -1;
    
    time.SetRefreshRate(1);
    
    mIsConnected = true;
    return 1;
}

SOCKET WindSock::CheckIncomingConnections(void) {
    
    sockaddr_in client;
    int clientSz = sizeof(client);
    
    SOCKET clientSocket = accept(mSocket, (sockaddr*)&client, &clientSz);
    
    if ((clientSocket == WSAEWOULDBLOCK) | (clientSocket == INVALID_SOCKET)) 
        return clientSocket;
    
    char newHost[NI_MAXHOST];
    char newPort[NI_MAXSERV];
    
    ZeroMemory(newHost, NI_MAXHOST);
    ZeroMemory(newPort, NI_MAXSERV);
    
    getnameinfo((sockaddr*)&client, clientSz, newHost, NI_MAXHOST, newPort, NI_MAXSERV, 0);
    
    IPAddress address;
    address.addr[0] = client.sin_addr.S_un.S_un_b.s_b1;
    address.addr[1] = client.sin_addr.S_un.S_un_b.s_b2;
    address.addr[2] = client.sin_addr.S_un.S_un_b.s_b3;
    address.addr[3] = client.sin_addr.S_un.S_un_b.s_b4;
    
    // Accept the client into the connection list
    mLastIndex   = GetNumberOfSockets() + 1;
    mLastHost    = newHost;
    mLastPort    = client.sin_port;
    mLastAddress = address;
    
    std::string newBuffer = "";
    
    mSocketList.push_back(clientSocket);
    mHostList.push_back(mLastHost);
    mPortList.push_back(mLastPort);
    mAddressList.push_back(mLastAddress);
    mBufferList.push_back(newBuffer);
    mTimeoutList.push_back(CONNECTION_TIMEOUT);
    
    // Log client has connected
#ifdef LOG_ACTIVITY
    std::cout << "Connected         " << GetLastAddress().str() << std::endl;
#endif
    
    return clientSocket;
}

int WindSock::CheckIncomingMessages(char* buffer, unsigned int bufferSize) {
    int numberOfBytes = SOCKET_ERROR;
    
    for (unsigned int i=0; i < mSocketList.size(); i++) {
        SOCKET socket = mSocketList[i];
        
        numberOfBytes = MessageReceive(socket, buffer, bufferSize);
        
        if (numberOfBytes < 0) 
            continue;
        
        // Remember the last client to access the server
        mLastIndex   = i;
        mLastHost    = mHostList[i];
        mLastPort    = mPortList[i];
        mLastAddress = mAddressList[i];
        
        if (numberOfBytes == 0) {
            
            DisconnectFromClient(i);
            
#ifdef LOG_ACTIVITY
            std::cout << "Disconnected      " << GetLastAddress().str() << std::endl;
#endif
            
            continue;
        }
        
        // Reset connection time out
        mTimeoutList[i] = CONNECTION_TIMEOUT;
        
        // Assemble the message string
        std::string bufferBuf;
        bufferBuf.reserve(numberOfBytes+1);
        bufferBuf.resize(numberOfBytes);
        for (int a=0; a < numberOfBytes; a++) 
            bufferBuf += buffer[a];
        bufferBuf += "[break]";
        
        mBufferList[i] += bufferBuf;
        
        continue;
    }
    
    return numberOfBytes;
}

int WindSock::CheckTimers(void) {
    
    // Increment timers
    if (!time.Update()) 
        return 0;
    
    // Check timers
    for (int i=0; i < GetNumberOfTimers(); i++) {
        
        SetTimerValue(i, GetTimerIndex(i) - 1);
        
        if (GetTimerIndex(i) <= 0) {
            
            DisconnectFromClient(i);
            
#ifdef LOG_ACTIVITY
            std::cout << "Timed-out         " << GetLastAddress().str() << std::endl;
#endif
            
        }
        
    }
    
    return 0;
}

int WindSock::DisconnectFromClient(unsigned int index) {
    
    if (index > mSocketList.size()) 
        return -1;
    
    SOCKET socket = mSocketList[index];
    
    mSocketList.erase(mSocketList.begin() + index);
    mHostList.erase(mHostList.begin() + index);
    mPortList.erase(mPortList.begin() + index);
    mAddressList.erase(mAddressList.begin() + index);
    mBufferList.erase(mBufferList.begin() + index);
    mTimeoutList.erase(mTimeoutList.begin() + index);
    
    closesocket(socket);
    return 1;
}

void WindSock::MessageSend(SOCKET socket, char* buffer, unsigned int bufferSize) {
    send(socket, buffer, bufferSize, 0);
}

int WindSock::MessageReceive(SOCKET socket, char* buffer, unsigned int bufferSize) {
    return recv(socket, buffer, bufferSize, 0);
}

