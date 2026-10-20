#pragma once

#include <string>
#include <vector>

namespace PlatformUtils {
    void initLogger();
    void log(const std::string& message);

    std::wstring utf8ToWide(const std::string& str);
    std::string wideToUtf8(const std::wstring& wstr);

    std::vector<std::string> openAudioFileDialog();
    void showSystemNotification(const std::string& title, const std::string& message);
}