//
// Still under construction... may be chaotic


bool CheckDirectoryExists(std::string directoryName) {
    DWORD dwAttrib = GetFileAttributes( (LPCTSTR)directoryName.data() );
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

int FileCheckExists(std::string filename) {
    std::ifstream fStream(filename.c_str(), std::ios::binary | std::ios::ate);
    if (fStream.is_open()) {
        std::streamsize size = fStream.tellg();
        fStream.close();
        return size;
    }
    return -1;
}

int FileLoadText(std::string filename, std::string& buffer) {
    std::fstream fStream(filename);
    if (!fStream.is_open()) 
        return -1;
    
    std::string tempBuffer;
    
    // Load the data from file
    while ( getline(fStream, tempBuffer) ) 
        if (tempBuffer != "")
            buffer += tempBuffer;
    
    
    
    fStream.close();
    return buffer.size();
}




int FileLoadRaw(std::string filename, char* buffer, unsigned int bufferSize) {
    std::ifstream fStream(filename.c_str(), std::ios::binary | std::ios::ate);
    if (!fStream.is_open()) 
        return -1;
    
    std::streamsize size = fStream.tellg();
    
    fStream.seekg(0, std::ios::beg);
    
    if (!fStream.read(buffer, size)) 
        return -1;
    
    return size;
}



std::string GenerateHTTPStatusLine(std::string statusCode, std::string contentLength, std::string contentType, std::string requestedConnectionState) {
    std::string headerLine;
    headerLine  = "HTTP/1.1 "+statusCode+"\r\n";
    headerLine += "Server: WindSock/0.9.0\r\n";
    headerLine += "Date: Mon, 02 Jan 2012 02:33:17 GMT\r\n";
    headerLine += "Content-Type: "+contentType+"\r\n";
    headerLine += "Content-Length: "+contentLength+"\r\n";
    headerLine += "Connection: "+requestedConnectionState+"\r\n";
    return headerLine;
}




namespace STATECODE {
    // General
    const char ok[]               = "200 OK";
    const char not_found[]        = "404 Not found";
    const char processing[]       = "102 Processing";
    const char no_content[]       = "204 No Content";
    // Errors
    const char bad_request[]      = "400 Bad Request";
    const char unauthorized[]     = "401 Unauthorized";
    const char forbidden[]        = "403 Forbidden";
    // Internal errors
    const char server_error[]     = "500 Internal Server Error";
    const char bad_gateway[]      = "502 Bad Gateway";
    const char gateway_timeout[]  = "504 Gateway Timeout";
    const char unavailable[]      = "503 Service Unavailable";
}

namespace CONTENTTYPE {
    // Text
    const char text_plain[]    = "text/plain";
    const char text_html[]     = "text/html";
    const char text_xml[]      = "text/xml";
    const char text_rtf[]      = "text/rtf";
    const char text_css[]      = "text/css";
    const char text_csv[]      = "text/csv";
    const char text_csvsch[]   = "text/csv-schema";
    const char text_md[]       = "text/markdown";
    const char text_js[]       = "text/javascript";
    // Images
    const char image_jpeg[]    = "image/jpeg";
    const char image_png[]     = "image/png";
    const char image_bmp[]     = "image/bmp";
    const char image_svgxml[]  = "image/svg+xml";
    // Video
    const char video_mp4[]     = "video/mp4";
    const char video_ogg[]     = "video/ogg";
    // Audio
    const char audio_ogg[]     = "audio/ogg";
    // Model
    const char model_obj[]     = "model/obj";
    const char model_mtl[]     = "model/mtl";
    // Application
    const char application_octet_stream[]     = "application/octet-stream";
}

namespace CONNECTION {
    // Close connection after initial communication
    const char close[]         = "close";
    // Keep connection open after initial communication
    const char keep_alive[]    = "keep-alive";
}




class SocketServer {
    
public:
    
    Timer time;
    Logger Log;
    
    WindSock wSock;
    
    std::string buffer;
    
    // Basic HTML rendering.
    
    std::string RenderHTMLHeader(std::string title, std::string colorBackground);
    std::string RenderHTMLTextStyleBegin(std::string colorText, unsigned int size);
    std::string RenderHTMLTextBody(std::string text, unsigned int size);
    std::string RenderHTMLTextStyleEnd(unsigned int size);
    std::string RenderHTMLFooter(void);
    
    /// Check and handle server requests.
    int CheckRequest(void);
    
    
    /// Update timers and time out old connections.
    int CheckTimers(void);
    
    
    SocketServer();
    
    
private:
    
    void ProcessGetRequest(unsigned int index, std::string& clientRequest, std::string& resourceName, unsigned int headerBegin, unsigned int headerEnd);
    
    void ProcessPutRequest(unsigned int index, std::string& clientRequest, std::string& resourceName, unsigned int headerBegin, unsigned int headerEnd);
    
    void ProcessDeleteRequest(unsigned int index, std::string& clientRequest, std::string& resourceName, unsigned int headerBegin, unsigned int headerEnd);
    
    
    
    
    
    
};





SocketServer::SocketServer() {
    Log.Clear();
    
    time.SetRefreshRate(1);
    
    buffer.reserve(32768);
    buffer.resize(8096);
}

int SocketServer::CheckTimers() {
    
    if (time.Update()) {
        unsigned int numberOfTimers = wSock.GetNumberOfTimers();
        
        for (unsigned int i=0; i < numberOfTimers; i++) {
            int currentTimeout = wSock.GetTimerIndex(i);
            currentTimeout--;
            wSock.SetTimerValue(i, currentTimeout);
            
            if (currentTimeout <= 0) {
                
                wSock.DisconnectFromClient(i);
                return -1;
            }
            
        }
        
    }
    return 0;
}



int SocketServer::CheckRequest(void) {
    
    std::string resourceRequest(1024, ' ');
    
    for (unsigned int i=0; i < wSock.GetNumberOfSockets(); i++) {
        std::string clientRequest = wSock.GetBufferString(i);
        
        if (clientRequest == "") 
            continue;
        
        // Check HTTP request
        if (clientRequest.compare(clientRequest.size()-4, 4, "\r\n\r\n") != 0) 
            continue;
        
        // Process HTTP requests
        for (unsigned int a=0; a < 100; a++) {
            
            unsigned int headerEnd = clientRequest.find("HTTP/1.1")-1;
            
            // Check complete request
            if (headerEnd == std::string::npos) 
                continue;
            
            // Determine the request type
            // GET HEAD   POST PUT   DELETE   TRACE   CONNECT
            
            // GET request
            unsigned int headerBegin = clientRequest.find("GET /");
            if (headerBegin != std::string::npos) {
                ProcessGetRequest(i, clientRequest, resourceRequest, headerBegin, headerEnd);
                continue;
            }
            
            // PUT request
            headerBegin = clientRequest.find("PUT /");
            if (headerBegin != std::string::npos) {
                ProcessPutRequest(i, clientRequest, resourceRequest, headerBegin, headerEnd);
                continue;
            }
            
            // DELETE request
            headerBegin = clientRequest.find("DELETE /");
            if (headerBegin != std::string::npos) {
                ProcessDeleteRequest(i, clientRequest, resourceRequest, headerBegin, headerEnd);
                continue;
            }
            
            
        }
        
        wSock.ClearBufferString(i);
        
    }
    
    return 1;
}




std::string SocketServer::RenderHTMLHeader(std::string title, std::string colorBackground) {
    std::string page;
    page  = "<html>";
    page += "<head><title>" + title + "</title></head>";
    page += "<body bgcolor=\"" + colorBackground + "\">";
    return page;
}

std::string SocketServer::RenderHTMLTextStyleBegin(std::string colorText, unsigned int size) {
    std::string page;
    std::string textSz = IntToString(size);
    page  = "<h" + textSz + " style=\"color:" + colorText + ";\">";
    return page;
}

std::string SocketServer::RenderHTMLTextBody(std::string text, unsigned int size) {
    std::string page;
    std::string textSz = IntToString(size);
    page  = "  <center><h" + textSz + ">" + text + "</h" + textSz + "></center>";
    return page;
}

std::string SocketServer::RenderHTMLTextStyleEnd(unsigned int size) {
    std::string page;
    std::string textSz = IntToString(size);
    page  = "</h" + textSz + ">";
    return page;
}

std::string SocketServer::RenderHTMLFooter(void) {
    std::string page;
    page  = "</body>";
    page += "</html>";
    return page;
}


void SocketServer::ProcessGetRequest(unsigned int index, std::string& clientRequest, std::string& resourceName, unsigned int headerBegin, unsigned int headerEnd) {
    std::string statusCode="200";
    
    // Get requested resource name
    resourceName.resize( headerEnd - headerBegin - 5);
    
    clientRequest.copy((char*)resourceName.c_str(), headerEnd - 5, headerBegin + 5);
    resourceName.shrink_to_fit();
    
    // Kill the old request as it has been processed
    clientRequest[headerBegin+1] = ' ';
    
    // Default index request
    if ((resourceName[resourceName.size()-1] == '/') | (resourceName == "")) 
        resourceName += "index.html";
    
    std::string dataBody;
    int fileSize=-1;
    
    // Determine file type
    std::string fileType = StringGetExtFromFilename(resourceName);
    
    // Check resource exists
    //if (FileCheckExists(resourceName) != -1)
    //    return;
    
    // Load text files
    if ((fileType == "html") | 
        (fileType == "htm") | 
        (fileType == "css") | 
        (fileType == "js")) {
        std::string newDataBody;
        fileSize = FileLoadText(resourceName, newDataBody);
        dataBody = newDataBody;
    }
    
    // Load raw binary files
    if ((fileType == "png") | 
        (fileType == "jpg") | 
        (fileType == "ico") | 
        (fileType == "exe")) {
        
        dataBody.resize(100000000);
        fileSize = FileLoadRaw(resourceName, (char*)dataBody.data(), dataBody.size());
        if (fileSize != -1) {
            dataBody.resize(fileSize);
        }
    }
    
    // No file exists, run it as a sub directory
    // containing perhaps an index.html?
    if ((fileSize == -1) & (fileType == "")) {
        resourceName += "/index.html";
        fileType = "html";
        std::string newDataBody;
        fileSize = FileLoadText(resourceName, newDataBody);
        dataBody = newDataBody;
    }
    
    // Requested resource does not exist - Throw a 404
    if (fileSize == -1) {
        std::string newDataBody;
        fileSize = FileLoadText("404.html", newDataBody);
        dataBody = newDataBody;
        statusCode = "404";
    }
    
    // No 404 page, generate a default
    if (fileSize == -1) {
        std::string page;
        page  = RenderHTMLHeader("404 Not found", "black");
        
        page += RenderHTMLTextStyleBegin("white", 1);
        page += RenderHTMLTextBody("404", 1);
        page += RenderHTMLTextStyleEnd(1);
        
        page += RenderHTMLTextStyleBegin("white", 3);
        page += RenderHTMLTextBody("Not found", 1);
        page += RenderHTMLTextStyleEnd(3);
        
        page += RenderHTMLFooter();
        dataBody = page;
        fileSize = dataBody.size();
    }
    
    std::string bodySzStr = IntToString(fileSize);
    std::string headerLine;
    
    // Image content types
    
    if (fileType == "jpg") 
        headerLine = GenerateHTTPStatusLine(STATECODE::ok, bodySzStr, CONTENTTYPE::image_jpeg, CONNECTION::keep_alive);
    if ((fileType == "png") | 
        (fileType == "ico")) 
        headerLine = GenerateHTTPStatusLine(STATECODE::ok, bodySzStr, CONTENTTYPE::image_png, CONNECTION::keep_alive);
    
    // Executable types
    
    if (fileType == "exe") 
        headerLine = GenerateHTTPStatusLine(STATECODE::ok, bodySzStr, CONTENTTYPE::application_octet_stream, CONNECTION::keep_alive);
    
    // Text content types
    
    if (fileType == "css") headerLine = GenerateHTTPStatusLine(STATECODE::ok, bodySzStr, CONTENTTYPE::text_css, CONNECTION::keep_alive);
    if (fileType == "js")  headerLine = GenerateHTTPStatusLine(STATECODE::ok, bodySzStr, CONTENTTYPE::text_js, CONNECTION::keep_alive);
    if ((fileType == "html") | 
        (fileType == "htm")) 
        headerLine = GenerateHTTPStatusLine(STATECODE::ok, bodySzStr, CONTENTTYPE::text_html, CONNECTION::keep_alive);
    
    std::cout << "HTTP request      " << wSock.GetLastAddress().str() << " " << statusCode << " /" << resourceName << std::endl;
    Log.Write( "GET "+wSock.GetLastAddress().str() + " /" + resourceName );
    
    // Send back the status and requested resource data
    std::string status = headerLine + "\r\n" + dataBody;
    wSock.MessageSend(wSock.GetSocketIndex(index), (char*)status.c_str(), status.size());
    
}


void SocketServer::ProcessPutRequest(unsigned int index, std::string& clientRequest, std::string& resourceName, unsigned int headerBegin, unsigned int headerEnd) {
    
    std::cout << clientRequest << std::endl << std::endl;
    
}

void SocketServer::ProcessDeleteRequest(unsigned int index, std::string& clientRequest, std::string& resourceName, unsigned int headerBegin, unsigned int headerEnd) {
    
    std::cout << clientRequest << std::endl << std::endl;
    
}


