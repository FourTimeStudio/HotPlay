#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <GLFW/glfw3.h>
#include <nlohmann/json.hpp>
#include "Localization.hpp"

using json = nlohmann::json;

enum class PlaybackMode {
    Normal = 0,
    RepeatTrack,
    LoopFolder,
    Shuffle
};

struct HotkeyCombo {
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    bool super = false;
    int key = GLFW_KEY_UNKNOWN;

    bool isEmpty() const {
        return key == GLFW_KEY_UNKNOWN && !ctrl && !shift && !alt && !super;
    }

    bool operator==(const HotkeyCombo& o) const {
        return ctrl == o.ctrl && shift == o.shift && alt == o.alt && 
               super == o.super && key == o.key;
    }

    bool operator!=(const HotkeyCombo& o) const {
        return !(*this == o);
    }

    std::string toString() const {
        if (isEmpty()) return "None";
        std::stringstream ss;
        if (ctrl)  ss << "Ctrl + ";
        if (shift) ss << "Shift + ";
        if (alt)   ss << "Alt + ";
        if (super) ss << "Super + ";

        if (key != GLFW_KEY_UNKNOWN) {
            const char* keyName = glfwGetKeyName(key, 0);
            if (keyName && keyName[0] != '\0') {
                std::string upperKey = keyName;
                for (auto& c : upperKey) c = (char)::toupper(c);
                ss << upperKey;
            } else {
                switch (key) {
                    case GLFW_KEY_SPACE:         ss << "Space"; break;
                    case GLFW_KEY_ENTER:         ss << "Enter"; break;
                    case GLFW_KEY_TAB:           ss << "Tab"; break;
                    case GLFW_KEY_BACKSPACE:     ss << "Backspace"; break;
                    case GLFW_KEY_INSERT:        ss << "Insert"; break;
                    case GLFW_KEY_DELETE:        ss << "Delete"; break;
                    case GLFW_KEY_RIGHT:         ss << "Right"; break;
                    case GLFW_KEY_LEFT:          ss << "Left"; break;
                    case GLFW_KEY_DOWN:          ss << "Down"; break;
                    case GLFW_KEY_UP:            ss << "Up"; break;
                    case GLFW_KEY_PAGE_UP:       ss << "PageUp"; break;
                    case GLFW_KEY_PAGE_DOWN:     ss << "PageDown"; break;
                    case GLFW_KEY_HOME:          ss << "Home"; break;
                    case GLFW_KEY_END:           ss << "End"; break;
                    case GLFW_KEY_CAPS_LOCK:     ss << "CapsLock"; break;
                    case GLFW_KEY_SCROLL_LOCK:   ss << "ScrollLock"; break;
                    case GLFW_KEY_NUM_LOCK:      ss << "NumLock"; break;
                    case GLFW_KEY_PRINT_SCREEN:  ss << "PrintScreen"; break;
                    case GLFW_KEY_PAUSE:         ss << "Pause"; break;
                    default:
                        if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F25) {
                            ss << "F" << (key - GLFW_KEY_F1 + 1);
                        } else if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) {
                            ss << "Num " << (key - GLFW_KEY_KP_0);
                        } else {
                            ss << "Key_" << key;
                        }
                        break;
                }
            }
        } else {
            std::string str = ss.str();
            if (str.length() >= 3) str.resize(str.length() - 3);
            return str;
        }
        return ss.str();
    }
};

struct SoundTrack {
    std::string name;
    std::string filePath;
    HotkeyCombo hotkey;
    float volume = 1.0f;

    bool operator==(const SoundTrack& o) const {
        return name == o.name && filePath == o.filePath && 
               hotkey == o.hotkey && volume == o.volume;
    }

    bool operator!=(const SoundTrack& o) const {
        return !(*this == o);
    }
};

struct Folder {
    std::string name;
    std::vector<SoundTrack> sounds;
};

struct AppConfig {
    PlaybackMode playback_mode = PlaybackMode::Normal;
    float folders_panel_width = 240.0f;
    float visualizer_bar_width = 5.0f;
    float master_volume = 100.0f;
    bool minimize_to_tray = false;
    std::string language = "eng";

    bool eq_enabled = true;
    float eq_low = 0.0f;
    float eq_mid = 0.0f;
    float eq_high = 0.0f;

    std::vector<Folder> folders;

    void save(const std::string& path) const {
        json j;
        j["playback_mode"] = static_cast<int>(playback_mode);
        j["folders_panel_width"] = folders_panel_width;
        j["visualizer_bar_width"] = visualizer_bar_width;
        j["master_volume"] = master_volume;
        j["minimize_to_tray"] = minimize_to_tray;
        j["language"] = language;

        j["eq_enabled"] = eq_enabled;
        j["eq_low"] = eq_low;
        j["eq_mid"] = eq_mid;
        j["eq_high"] = eq_high;

        j["folders"] = json::array();

        for (const auto& folder : folders) {
            json fJson;
            fJson["name"] = folder.name;
            fJson["sounds"] = json::array();
            for (const auto& s : folder.sounds) {
                json sJson;
                sJson["name"] = s.name;
                sJson["filePath"] = s.filePath;
                sJson["volume"] = s.volume;
                sJson["hotkey"] = {
                    {"ctrl", s.hotkey.ctrl},
                    {"shift", s.hotkey.shift},
                    {"alt", s.hotkey.alt},
                    {"super", s.hotkey.super},
                    {"key", s.hotkey.key}
                };
                fJson["sounds"].push_back(sJson);
            }
            j["folders"].push_back(fJson);
        }

        std::ofstream file(path);
        if (file.is_open()) {
            file << j.dump(4);
        }
    }

    void load(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return;

        try {
            json j;
            file >> j;
            if (j.contains("playback_mode")) {
                playback_mode = static_cast<PlaybackMode>(j["playback_mode"].get<int>());
            }
            if (j.contains("folders_panel_width")) {
                folders_panel_width = j["folders_panel_width"].get<float>();
            }
            if (j.contains("visualizer_bar_width")) {
                visualizer_bar_width = j["visualizer_bar_width"].get<float>();
            }
            if (j.contains("master_volume")) {
                master_volume = std::clamp(j["master_volume"].get<float>(), 0.0f, 100.0f);
            }
            if (j.contains("minimize_to_tray")) {
                minimize_to_tray = j["minimize_to_tray"].get<bool>();
            }
            if (j.contains("language")) {
                language = j["language"].get<std::string>();
                I18n::setLanguage(I18n::fromCode(language));
            }
            if (j.contains("eq_enabled")) {
                eq_enabled = j["eq_enabled"].get<bool>();
            }
            if (j.contains("eq_low")) {
                eq_low = std::clamp(j["eq_low"].get<float>(), -12.0f, 12.0f);
            }
            if (j.contains("eq_mid")) {
                eq_mid = std::clamp(j["eq_mid"].get<float>(), -12.0f, 12.0f);
            }
            if (j.contains("eq_high")) {
                eq_high = std::clamp(j["eq_high"].get<float>(), -12.0f, 12.0f);
            }

            if (j.contains("folders") && j["folders"].is_array()) {
                folders.clear();
                for (const auto& fJson : j["folders"]) {
                    Folder folder;
                    folder.name = fJson.value("name", "General");
                    if (fJson.contains("sounds") && fJson["sounds"].is_array()) {
                        for (const auto& sJson : fJson["sounds"]) {
                            SoundTrack track;
                            track.name = sJson.value("name", "Track");
                            track.filePath = sJson.value("filePath", "");
                            track.volume = sJson.value("volume", 1.0f);

                            if (sJson.contains("hotkey")) {
                                if (sJson["hotkey"].is_number_integer()) {
                                    track.hotkey.key = sJson["hotkey"].get<int>();
                                } else if (sJson["hotkey"].is_object()) {
                                    const auto& hk = sJson["hotkey"];
                                    track.hotkey.ctrl = hk.value("ctrl", false);
                                    track.hotkey.shift = hk.value("shift", false);
                                    track.hotkey.alt = hk.value("alt", false);
                                    track.hotkey.super = hk.value("super", false);
                                    track.hotkey.key = hk.value("key", GLFW_KEY_UNKNOWN);
                                }
                            }
                            folder.sounds.push_back(track);
                        }
                    }
                    folders.push_back(folder);
                }
            }
        } catch (...) {}
    }
};