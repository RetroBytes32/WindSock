#include <vector>
#include <string>
#include <sstream>

#include <WS2tcpip.h>


class IPAddress {
    
public:
    
    unsigned char addr[4] = {0, 0, 0, 0};
    
    std::string str() {
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
    
};
    


class WindSock {
    
public:
    
    WindSock(void);
    ~WindSock(void);
    
    unsigned int GetLastPort(void)     {return mLastPort;}
    std::string  GetLastHost(void)     {return mLastHost;}
    IPAddress    GetLastAddress(void)  {return mLastAddress;}
    unsigned int GetLastIndex(void)    {return mLastIndex;}
    
    
    // Client
    
    /// Connect to a server.
    SOCKET ConnectToServer(std::string address, unsigned int port);
    
    /// Disconnect from a server.
    int DisconnectFromServer(SOCKET socket);
    
    // Server
    
    /// Start a server listening for incoming connections.
    int InitiateServer(unsigned int port, unsigned int maxConn=200);
    
    /// Check for an incoming connection request.
    SOCKET CheckIncomingConnections(void);
    
    /// Check for incoming messages from any connected client.
    int CheckIncomingMessages(char* buffer, unsigned int bufferSize);
    
    // Active connections
    
    /// Get the number of hosts in the connections list.
    unsigned int GetNumberOfHosts(void) {return mHostList.size();}
    /// Get a host name by its index location in the connections list.
    std::string GetHostIndex(unsigned int index) {return mHostList[index];}
    /// Find a host index location by its name.
    int FindHost(std::string name) {
        for (unsigned int i=0; i < GetNumberOfHosts(); i++) 
            if (GetHostIndex(i) == name) return i;
        return -1;
    }
    
    /// Get the number of ports in the connections list.
    unsigned int GetNumberOfPorts(void) {return mPortList.size();}
    /// Get a port by its index location in the connections list.
    unsigned int GetPortIndex(unsigned int index) {return mPortList[index];}
    
    /// Get the number of sockets in the connections list.
    unsigned int GetNumberOfSockets(void) {return mSocketList.size();}
    /// Get a socket by its index location in the connections list.
    SOCKET GetSocketIndex(unsigned int index) {return mSocketList[index];}
    
    /// Get a buffer string from a socket index location in the connections list.
    std::string GetBufferString(unsigned int index) {return mBufferList[index];}
    /// Get a buffer string from a socket index location in the connections list.
    void ClearBufferString(unsigned int index) {mBufferList[index] = "";}
    
    
    // Messaging
    
    /// Send a message
    void MessageSend(SOCKET socket, char* buffer, unsigned int bufferSize);
    
    /// Receive a message
    int MessageReceive(SOCKET socket, char* buffer, unsigned int bufferSize);
    
    
private:
    
    std::string   mLastHost;
    unsigned int  mLastPort;
    IPAddress     mLastAddress;
    unsigned int  mLastIndex;
    
    bool    mIsConnected;
    SOCKET  mSocket;
    
    std::vector<std::string>       mHostList;
    std::vector<unsigned int>      mPortList;
    std::vector<IPAddress>         mAddressList;
    std::vector<SOCKET>            mSocketList;
    std::vector<std::string>       mBufferList;
    
};


WindSock::WindSock(void) :
    
    mLastHost(""),
    mLastPort(0),
    mLastAddress({127, 0, 0, 1}),
    mLastIndex(0),
    
    mIsConnected(false),
    mSocket(0)
{
    
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);
    
    int WSOK = WSAStartup(ver, &wsData);
    if (WSOK != 0) 
        return;
    
    mSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mSocket == INVALID_SOCKET) {
        WSACleanup();
        return;
    }
    
}

WindSock::~WindSock(void) {
    
    if (mSocket != INVALID_SOCKET) 
        closesocket(mSocket);
    
    WSACleanup();
}

SOCKET WindSock::ConnectToServer(std::string address, unsigned int port) {
    
    if (mIsConnected == true) 
        return -1;
    
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    unsigned long uipAddr = inet_addr(address.c_str());
    hint.sin_addr = (in_addr&)uipAddr;
    
    // Connect to the server
    SOCKET serverSocket = connect(mSocket, (sockaddr*)&hint, sizeof(hint));
    
    if (serverSocket != INVALID_SOCKET) 
        mIsConnected = true;
    
    return serverSocket;
}

int WindSock::DisconnectFromServer(SOCKET socket) {
    if (!mIsConnected) return -1;
    
    mIsConnected = false;
    return closesocket(socket);
}

int WindSock::InitiateServer(unsigned int port, unsigned int maxConn) {
    
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    hint.sin_addr.S_un.S_addr = INADDR_ANY;
    
    bind(mSocket, (sockaddr*)&hint, sizeof(hint));
    listen(mSocket, maxConn);
    
    // Non blocking listening socket
    u_long sockMode = 1;
    ioctlsocket(mSocket, FIONBIO, &sockMode);
    
    if (mSocket == INVALID_SOCKET) 
        return -1;
    
    mIsConnected = true;
    return 1;
}

SOCKET WindSock::CheckIncomingConnections(void) {
    
    sockaddr_in client;
    int clientSz = sizeof(client);
    
    // Listen for a connection
    SOCKET clientSocket = accept(mSocket, (sockaddr*)&client, &clientSz);
    
    if ((clientSocket == WSAEWOULDBLOCK) | (clientSocket == INVALID_SOCKET)) 
        return clientSocket;
    
    char newHost[NI_MAXHOST];
    char newPort[NI_MAXSERV];
    
    ZeroMemory(newHost, NI_MAXHOST);
    ZeroMemory(newPort, NI_MAXSERV);
    
    getnameinfo((sockaddr*)&client, clientSz, newHost, NI_MAXHOST, newPort, NI_MAXSERV, 0);
    
    // Get the IP address
    IPAddress address;
    address.addr[0] = client.sin_addr.S_un.S_un_b.s_b1;
    address.addr[1] = client.sin_addr.S_un.S_un_b.s_b2;
    address.addr[2] = client.sin_addr.S_un.S_un_b.s_b3;
    address.addr[3] = client.sin_addr.S_un.S_un_b.s_b4;
    
    // Add the client to the connection list
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
    
    return clientSocket;
}

int WindSock::CheckIncomingMessages(char* buffer, unsigned int bufferSize) {
    int numberOfBytes = SOCKET_ERROR;
    
    // Check incoming messages
    for (unsigned int i=0; i < mSocketList.size(); i++) {
        SOCKET socket = mSocketList[i];
        
        numberOfBytes = MessageReceive(socket, buffer, bufferSize);
        
        if (numberOfBytes < 0) 
            continue;
        
        // Remember the last accessed host
        mLastIndex   = i;
        mLastHost    = mHostList[i];
        mLastPort    = mPortList[i];
        mLastAddress = mAddressList[i];
        
        // Client has disconnected
        if (numberOfBytes == 0) {
            
            closesocket(socket);
            
            // Remove the client from the server
            mSocketList.erase(mSocketList.begin() + i);
            mHostList.erase(mHostList.begin() + i);
            mPortList.erase(mPortList.begin() + i);
            mAddressList.erase(mAddressList.begin() + i);
            mBufferList.erase(mBufferList.begin() + i);
            continue;
        }
        
        // Message string
        std::string bufferBuf;
        for (int a=0; a < numberOfBytes; a++) 
            bufferBuf += buffer[a];
        
        mBufferList[i] += bufferBuf;
        
        continue;
    }
    
    return numberOfBytes;
}

void WindSock::MessageSend(SOCKET socket, char* buffer, unsigned int bufferSize) {
    send(socket, buffer, bufferSize, 0);
}

int WindSock::MessageReceive(SOCKET socket, char* buffer, unsigned int bufferSize) {
    return recv(socket, buffer, bufferSize, 0);
}

