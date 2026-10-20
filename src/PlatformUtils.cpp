#include "PlatformUtils.hpp"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#else
#include <cstdlib>
#include <array>
#endif

namespace PlatformUtils {

static std::ofstream g_logFile;

void initLogger() {
    g_logFile.open("hotplay_log.txt", std::ios::out | std::ios::trunc);
    log("=== HotPlay Session Started ===");
}

void log(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "[%Y-%m-%d %H:%M:%S] ") << message;
    std::string formatted = ss.str();

    std::cout << formatted << std::endl;
    if (g_logFile.is_open()) {
        g_logFile << formatted << std::endl;
        g_logFile.flush();
    }
}

std::wstring utf8ToWide(const std::string& str) {
#if defined(_WIN32)
    if (str.empty()) return L"";
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], sizeNeeded);
    return wstr;
#else
    return std::wstring(str.begin(), str.end());
#endif
}

std::string wideToUtf8(const std::wstring& wstr) {
#if defined(_WIN32)
    if (wstr.empty()) return "";
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], sizeNeeded, NULL, NULL);
    return str;
#else
    return std::string(wstr.begin(), wstr.end());
#endif
}

std::vector<std::string> openAudioFileDialog() {
    std::vector<std::string> selectedFiles;

#if defined(_WIN32)
    std::vector<wchar_t> buffer(65536, 0);
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = L"Media Files (*.mp3;*.wav;*.flac;*.ogg;*.mp4;*.m4a;*.mkv)\0*.mp3;*.wav;*.flac;*.ogg;*.mp4;*.m4a;*.mkv\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = buffer.data();
    ofn.nMaxFile = (DWORD)buffer.size();
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn)) {
        wchar_t* ptr = buffer.data();
        std::wstring dir = ptr;
        ptr += dir.length() + 1;

        if (*ptr == L'\0') {
            selectedFiles.push_back(wideToUtf8(dir));
        } else {
            while (*ptr != L'\0') {
                std::wstring filename = ptr;
                std::wstring fullPath = dir + L"\\" + filename;
                selectedFiles.push_back(wideToUtf8(fullPath));
                ptr += filename.length() + 1;
            }
        }
    }
#else
    std::string cmd = "zenity --file-selection --multiple --separator='|' --file-filter='Media Files | *.mp3 *.wav *.flac *.ogg *.mp4 *.m4a *.mkv *.webm' 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buf[512];
        std::string result = "";
        while (fgets(buf, sizeof(buf), pipe) != nullptr) {
            result += buf;
        }
        pclose(pipe);

        if (!result.empty() && result.back() == '\n') result.pop_back();

        std::stringstream ss(result);
        std::string item;
        while (std::getline(ss, item, '|')) {
            if (!item.empty()) selectedFiles.push_back(item);
        }
    }
#endif
    return selectedFiles;
}

void showSystemNotification(const std::string& title, const std::string& message) {
#if defined(_WIN32)
    std::thread([title, message]() {
        NOTIFYICONDATAW nid;
        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = GetConsoleWindow();
        nid.uID = 1337;
        nid.uFlags = NIF_INFO | NIF_MESSAGE;
        nid.dwInfoFlags = NIIF_INFO;

        std::wstring wTitle = utf8ToWide(title);
        std::wstring wMsg = utf8ToWide(message);

        wcsncpy_s(nid.szInfoTitle, wTitle.c_str(), _TRUNCATE);
        wcsncpy_s(nid.szInfo, wMsg.c_str(), _TRUNCATE);

        Shell_NotifyIconW(NIM_ADD, &nid);
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        Shell_NotifyIconW(NIM_DELETE, &nid);
    }).detach();
#else
    std::thread([title, message]() {
        std::string safeTitle = title;
        std::string safeMsg = message;
        for (char& c : safeTitle) if (c == '"') c = '\'';
        for (char& c : safeMsg) if (c == '"') c = '\'';

        std::string cmd = "notify-send -a \"HotPlay\" \"" + safeTitle + "\" \"" + safeMsg + "\" &";
        std::system(cmd.c_str());
    }).detach();
#endif
}

}