#include "App.hpp"
#include "GlobalHotkeys.hpp"
#include "PlatformUtils.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <random>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <unordered_set>

namespace fs = std::filesystem;

static std::string formatTime(float seconds) {
    int totalSec = std::max(0, static_cast<int>(seconds));
    int m = totalSec / 60;
    int s = totalSec % 60;
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << m << ":" 
       << std::setfill('0') << std::setw(2) << s;
    return ss.str();
}

static bool isSupportedMediaExtension(const std::string& ext) {
    static const std::unordered_set<std::string> exts = {
        ".mp3", ".wav", ".ogg", ".flac", ".m4a", ".aac", ".wma", 
        ".opus", ".aiff", ".aif", ".ape", ".alac", ".mid", ".midi",
        ".mp4", ".mkv", ".webm", ".avi", ".mov", ".wmv", ".flv", 
        ".vob", ".ts", ".m4v", ".3gp", ".ogv"
    };
    return exts.find(ext) != exts.end();
}

App::App() {}

App::~App() {
    config.language = I18n::getCode(I18n::currentLang());
    config.save("hotplay_config.json");
    PlatformUtils::log("Конфигурация успешно сохранена в hotplay_config.json");
}

void App::preloadActiveFolderTracks() {
    if (activeFolderIdx >= 0 && activeFolderIdx < (int)config.folders.size()) {
        for (const auto& s : config.folders[activeFolderIdx].sounds) {
            audio.preload(s.filePath);
        }
    }
}

void App::deleteFolder(int idx) {
    if (idx < 0 || idx >= (int)config.folders.size()) return;

    for (const auto& s : config.folders[idx].sounds) {
        audio.stop(s.filePath);
    }

    config.folders.erase(config.folders.begin() + idx);

    if (config.folders.empty()) {
        Folder def;
        def.name = I18n::tr(Str::DEFAULT_FOLDER);
        config.folders.push_back(def);
        activeFolderIdx = 0;
    } else {
        if (activeFolderIdx >= (int)config.folders.size()) {
            activeFolderIdx = (int)config.folders.size() - 1;
        }
    }

    preloadActiveFolderTracks();
    GlobalHotkeys::get().updateBindings(config.folders[activeFolderIdx].sounds);
}

void App::minimizeToTray() {
    GLFWwindow* win = glfwGetCurrentContext();
    if (win) {
        glfwHideWindow(win);
        PlatformUtils::showSystemNotification("HotPlay", I18n::tr(Str::TRAY_MINIMIZED_MSG));
    }
}

void App::init(void* nativeWindowHandle) {
    PlatformUtils::log("Инициализация приложения App...");
    audio.init();
    config.load("hotplay_config.json");

    if (config.folders.empty()) {
        Folder defaultFolder;
        defaultFolder.name = I18n::tr(Str::DEFAULT_FOLDER);
        config.folders.push_back(defaultFolder);
    }

    applyFlatStudioTheme();
    
    audio.setMasterVolume(config.master_volume / 100.0f);
    audio.setEQ(config.eq_low, config.eq_mid, config.eq_high, config.eq_enabled);

    preloadActiveFolderTracks();

    GlobalHotkeys::get().init(nativeWindowHandle, [this](int trackIdx) {
        this->triggerTrack(trackIdx);
    });

    if (activeFolderIdx >= 0 && activeFolderIdx < (int)config.folders.size()) {
        GlobalHotkeys::get().updateBindings(config.folders[activeFolderIdx].sounds);
    }
}

void App::applyFlatStudioTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 1.0f;
    style.PopupRounding     = 0.0f;
    style.GrabRounding      = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.TabRounding       = 0.0f;

    style.WindowBorderSize  = 0.0f;
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;

    style.WindowPadding     = ImVec2(8.0f, 8.0f);
    style.FramePadding      = ImVec2(7.0f, 4.0f);
    style.ItemSpacing       = ImVec2(6.0f, 6.0f);
    style.ItemInnerSpacing  = ImVec2(5.0f, 4.0f);
    style.ScrollbarSize     = 9.0f;
    style.GrabMinSize       = 8.0f;

    ImVec4* colors = style.Colors;
    
    colors[ImGuiCol_WindowBg]           = ImVec4(0.063f, 0.063f, 0.071f, 1.00f);
    colors[ImGuiCol_ChildBg]            = ImVec4(0.082f, 0.082f, 0.090f, 1.00f);
    colors[ImGuiCol_PopupBg]            = ImVec4(0.098f, 0.098f, 0.110f, 0.98f);
    
    colors[ImGuiCol_Border]             = ImVec4(0.180f, 0.180f, 0.200f, 1.00f);
    colors[ImGuiCol_BorderShadow]       = ImVec4(0.000f, 0.000f, 0.000f, 0.00f);

    colors[ImGuiCol_FrameBg]            = ImVec4(0.110f, 0.110f, 0.120f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.150f, 0.150f, 0.170f, 1.00f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.190f, 0.190f, 0.210f, 1.00f);

    colors[ImGuiCol_TitleBg]            = ImVec4(0.063f, 0.063f, 0.071f, 1.00f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.090f, 0.090f, 0.100f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]   = ImVec4(0.063f, 0.063f, 0.071f, 1.00f);

    colors[ImGuiCol_Button]             = ImVec4(0.137f, 0.137f, 0.153f, 1.00f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.550f, 0.110f, 0.110f, 1.00f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.780f, 0.150f, 0.150f, 1.00f);

    colors[ImGuiCol_Header]             = ImVec4(0.400f, 0.080f, 0.080f, 0.50f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.550f, 0.110f, 0.110f, 0.80f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.750f, 0.140f, 0.140f, 1.00f);

    colors[ImGuiCol_SliderGrab]         = ImVec4(0.780f, 0.150f, 0.150f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.950f, 0.220f, 0.220f, 1.00f);
    colors[ImGuiCol_CheckMark]          = ImVec4(0.950f, 0.220f, 0.220f, 1.00f);

    colors[ImGuiCol_TableHeaderBg]      = ImVec4(0.110f, 0.110f, 0.125f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]  = ImVec4(0.180f, 0.180f, 0.200f, 1.00f);
    colors[ImGuiCol_TableBorderLight]   = ImVec4(0.130f, 0.130f, 0.145f, 0.60f);
    colors[ImGuiCol_TableRowBg]         = ImVec4(0.082f, 0.082f, 0.090f, 1.00f);
    colors[ImGuiCol_TableRowBgAlt]      = ImVec4(0.094f, 0.094f, 0.102f, 1.00f);

    colors[ImGuiCol_Text]               = ImVec4(0.920f, 0.920f, 0.940f, 1.00f);
    colors[ImGuiCol_TextDisabled]       = ImVec4(0.450f, 0.450f, 0.480f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]     = ImVec4(0.700f, 0.120f, 0.120f, 0.45f);

    colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.070f, 0.070f, 0.078f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.200f, 0.200f, 0.220f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]=ImVec4(0.300f, 0.300f, 0.330f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]= ImVec4(0.400f, 0.400f, 0.440f, 1.00f);
    colors[ImGuiCol_Separator]          = ImVec4(0.180f, 0.180f, 0.200f, 1.00f);
}

void App::onFilesDropped(const std::vector<std::string>& filePaths) {
    if (activeFolderIdx < 0 || activeFolderIdx >= (int)config.folders.size()) return;

    for (const auto& pathStr : filePaths) {
        fs::path p(pathStr);
        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (isSupportedMediaExtension(ext)) {
            SoundTrack st;
            st.name = p.stem().string();
            st.filePath = pathStr;
            st.volume = 1.0f;
            config.folders[activeFolderIdx].sounds.push_back(st);
            audio.preload(pathStr);
            PlatformUtils::log("Добавлен медиафайл: " + pathStr);
        }
    }
}

void App::playTrack(int trackIndex, int folderIndex) {
    int fIdx = (folderIndex >= 0) ? folderIndex : activeFolderIdx;
    if (fIdx < 0 || fIdx >= (int)config.folders.size()) return;
    auto& sounds = config.folders[fIdx].sounds;
    if (trackIndex < 0 || trackIndex >= (int)sounds.size()) return;

    if (!playingFilePath.empty() && playingFilePath != sounds[trackIndex].filePath) {
        audio.stop(playingFilePath);
    }

    playingFolderIdx = fIdx;
    playingTrackIdx = trackIndex;
    playingFilePath = sounds[trackIndex].filePath;
    playingTrackName = sounds[trackIndex].name;
    lastPlayedRandomIdx = trackIndex;

    audio.play(playingFilePath, sounds[trackIndex].volume);
    PlatformUtils::showSystemNotification("HotPlay", playingTrackName);
}

void App::playNextTrack() {
    int targetFolder = (playingFolderIdx >= 0 && playingFolderIdx < (int)config.folders.size()) 
                       ? playingFolderIdx : activeFolderIdx;
    if (targetFolder < 0 || targetFolder >= (int)config.folders.size()) return;
    const auto& sounds = config.folders[targetFolder].sounds;
    if (sounds.empty()) return;

    if (config.playback_mode == PlaybackMode::Shuffle) {
        if (sounds.size() == 1) {
            playTrack(0, targetFolder);
            return;
        }
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(0, static_cast<int>(sounds.size()) - 1);
        int nextIdx = dis(gen);
        while (nextIdx == lastPlayedRandomIdx && sounds.size() > 1) {
            nextIdx = dis(gen);
        }
        lastPlayedRandomIdx = nextIdx;
        playTrack(nextIdx, targetFolder);
    } else {
        int curIdx = (playingFolderIdx == targetFolder) ? playingTrackIdx : -1;
        int next = (curIdx + 1) % static_cast<int>(sounds.size());
        playTrack(next, targetFolder);
    }
}

void App::playPrevTrack() {
    int targetFolder = (playingFolderIdx >= 0 && playingFolderIdx < (int)config.folders.size()) 
                       ? playingFolderIdx : activeFolderIdx;
    if (targetFolder < 0 || targetFolder >= (int)config.folders.size()) return;
    const auto& sounds = config.folders[targetFolder].sounds;
    if (sounds.empty()) return;

    int curIdx = (playingFolderIdx == targetFolder) ? playingTrackIdx : 0;
    int prev = (curIdx - 1 + static_cast<int>(sounds.size())) % static_cast<int>(sounds.size());
    playTrack(prev, targetFolder);
}

void App::triggerTrack(int trackIdx) {
    if (activeFolderIdx < 0 || activeFolderIdx >= (int)config.folders.size()) return;
    auto& sounds = config.folders[activeFolderIdx].sounds;
    if (trackIdx < 0 || trackIdx >= (int)sounds.size()) return;

    audio.togglePause(sounds[trackIdx].filePath, sounds[trackIdx].volume);
    if (audio.isPlaying(sounds[trackIdx].filePath)) {
        playingFolderIdx = activeFolderIdx;
        playingTrackIdx = trackIdx;
        playingFilePath = sounds[trackIdx].filePath;
        playingTrackName = sounds[trackIdx].name;
        PlatformUtils::showSystemNotification("HotPlay", playingTrackName);
    }
}

void App::update() {
    GlobalHotkeys::get().pollEvents();

    if (!playingFilePath.empty() && !audio.isPaused(playingFilePath) && !audio.isLoading(playingFilePath)) {
        if (audio.hasEnded(playingFilePath)) {
            switch (config.playback_mode) {
                case PlaybackMode::Normal:
                    playingFilePath.clear();
                    playingTrackName.clear();
                    playingTrackIdx = -1;
                    break;
                case PlaybackMode::RepeatTrack:
                    if (playingFolderIdx >= 0 && playingFolderIdx < (int)config.folders.size()) {
                        auto& sounds = config.folders[playingFolderIdx].sounds;
                        if (playingTrackIdx >= 0 && playingTrackIdx < (int)sounds.size()) {
                            audio.play(playingFilePath, sounds[playingTrackIdx].volume);
                            PlatformUtils::showSystemNotification("HotPlay", playingTrackName);
                        }
                    }
                    break;
                case PlaybackMode::LoopFolder:
                case PlaybackMode::Shuffle:
                    playNextTrack();
                    break;
            }
            return;
        }
    }

    bool currentActive = false;
    if (!playingFilePath.empty()) {
        if (audio.isPlaying(playingFilePath) || audio.isPaused(playingFilePath) || audio.isLoading(playingFilePath)) {
            currentActive = true;
        }
    }

    if (!currentActive) {
        bool found = false;
        for (int f = 0; f < (int)config.folders.size() && !found; ++f) {
            for (int t = 0; t < (int)config.folders[f].sounds.size(); ++t) {
                const auto& path = config.folders[f].sounds[t].filePath;
                if (audio.isPlaying(path) || audio.isPaused(path) || audio.isLoading(path)) {
                    playingFolderIdx = f;
                    playingTrackIdx = t;
                    playingFilePath = path;
                    playingTrackName = config.folders[f].sounds[t].name;
                    found = true;
                    currentActive = true;
                    break;
                }
            }
        }
        if (!found) {
            playingFilePath.clear();
            playingTrackName.clear();
            playingTrackIdx = -1;
        }
    }
}

void App::render() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::Begin("HotPlayRoot", nullptr, 
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

    renderFoldersPanel();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.20f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.20f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.25f, 0.25f, 1.0f));
    ImGui::Button("##vsplitter", ImVec2(4.0f, -72.0f));
    ImGui::PopStyleColor(3);

    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    if (ImGui::IsItemActive()) {
        config.folders_panel_width += ImGui::GetIO().MouseDelta.x;
        if (config.folders_panel_width < 140.0f) config.folders_panel_width = 140.0f;
        if (config.folders_panel_width > 500.0f) config.folders_panel_width = 500.0f;
    }

    ImGui::SameLine();
    renderTracksPanel();
    renderBottomPlayerBar();
    renderHotkeyModal();
    renderSettingsModal();
    renderEqualizerModal();

    ImGui::End();
}

void App::renderFoldersPanel() {
    ImGui::BeginChild("FoldersPanel", ImVec2(config.folders_panel_width, -72), true);

    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.75f, 1.0f), "%s", I18n::tr(Str::FOLDERS));
    ImGui::Separator();

    float btnAreaWidth = 56.0f;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnAreaWidth);
    ImGui::InputTextWithHint("##newfolder", I18n::tr(Str::NEW_FOLDER_HINT), newFolderNameBuf, sizeof(newFolderNameBuf));
    ImGui::SameLine();
    
    if (ImGui::Button("+", ImVec2(24.0f, 0))) {
        if (strlen(newFolderNameBuf) > 0) {
            Folder f;
            f.name = newFolderNameBuf;
            config.folders.push_back(f);
            newFolderNameBuf[0] = '\0';
        }
    }

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.08f, 0.08f, 0.8f));
    if (ImGui::Button("-", ImVec2(24.0f, 0))) {
        deleteFolder(activeFolderIdx);
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", I18n::tr(Str::DEL_FOLDER_TOOLTIP));
    }

    ImGui::Spacing();

    for (int i = 0; i < (int)config.folders.size(); ++i) {
        ImGui::PushID(i);
        bool isSelected = (activeFolderIdx == i);

        ImVec2 p0 = ImGui::GetCursorScreenPos();
        float availW = ImGui::GetContentRegionAvail().x;
        float itemH = 26.0f;
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        if (isSelected) {
            drawList->AddRectFilled(p0, ImVec2(p0.x + availW, p0.y + itemH), IM_COL32(50, 16, 20, 220));
            drawList->AddRectFilled(p0, ImVec2(p0.x + 3.0f, p0.y + itemH), IM_COL32(229, 57, 53, 255));
        }

        std::string label = "  " + config.folders[i].name;
        if (ImGui::Selectable(label.c_str(), isSelected, 0, ImVec2(availW, itemH))) {
            activeFolderIdx = i;
            preloadActiveFolderTracks();
            GlobalHotkeys::get().updateBindings(config.folders[activeFolderIdx].sounds);
        }

        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem(I18n::tr(Str::DEL_FOLDER_CTX))) {
                deleteFolder(i);
                ImGui::EndPopup();
                ImGui::PopID();
                break;
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("DND_FOLDER_REORDER", &i, sizeof(int));
            ImGui::Text(I18n::tr(Str::MOVE_FOLDER), config.folders[i].name.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_FOLDER_REORDER")) {
                int sourceIdx = *(const int*)payload->Data;
                if (sourceIdx != i) {
                    std::swap(config.folders[sourceIdx], config.folders[i]);
                    if (activeFolderIdx == sourceIdx) activeFolderIdx = i;
                    else if (activeFolderIdx == i) activeFolderIdx = sourceIdx;
                    preloadActiveFolderTracks();
                    GlobalHotkeys::get().updateBindings(config.folders[activeFolderIdx].sounds);
                }
            }
            ImGui::EndDragDropTarget();
        }

        std::string countStr = "[" + std::to_string(config.folders[i].sounds.size()) + "]";
        float countWidth = ImGui::CalcTextSize(countStr.c_str()).x;
        float countX = p0.x + availW - countWidth - 8.0f;
        float countY = p0.y + (itemH - ImGui::GetFontSize()) * 0.5f;
        ImU32 badgeCol = isSelected ? IM_COL32(220, 220, 230, 240) : IM_COL32(110, 110, 120, 200);
        drawList->AddText(ImVec2(countX, countY), badgeCol, countStr.c_str());

        ImGui::PopID();
    }

    ImGui::EndChild();
}

void App::renderTracksPanel() {
    ImGui::BeginChild("TracksPanel", ImVec2(0, -72), true);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.10f, 0.10f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.75f, 0.15f, 0.15f, 0.8f));
    if (ImGui::Button(I18n::tr(Str::ADD_TRACKS))) {
        auto files = PlatformUtils::openAudioFileDialog();
        onFilesDropped(files);
        GlobalHotkeys::get().updateBindings(config.folders[activeFolderIdx].sounds);
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();

    Str modeStrs[] = { Str::MODE_NORMAL, Str::MODE_REPEAT, Str::MODE_LOOP, Str::MODE_SHUFFLE };
    ImVec4 modeBorders[] = {
        ImVec4(0.35f, 0.35f, 0.38f, 1.0f),
        ImVec4(0.85f, 0.20f, 0.20f, 1.0f),
        ImVec4(0.20f, 0.70f, 0.30f, 1.0f),
        ImVec4(0.20f, 0.55f, 0.90f, 1.0f)
    };

    int currentModeInt = (int)config.playback_mode;
    ImGui::PushStyleColor(ImGuiCol_Border, modeBorders[currentModeInt]);
    if (ImGui::Button(I18n::tr(modeStrs[currentModeInt]))) {
        config.playback_mode = (PlaybackMode)((currentModeInt + 1) % 4);
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    if (ImGui::Button("|<")) playPrevTrack();
    ImGui::SameLine();
    if (ImGui::Button(">|")) playNextTrack();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.08f, 0.08f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.20f, 0.20f, 0.9f));
    if (ImGui::Button(I18n::tr(Str::STOP_ALL))) {
        audio.stopAll();
        playingFilePath.clear();
        playingTrackName.clear();
        playingTrackIdx = -1;
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.16f, 0.19f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.40f, 0.40f, 0.45f, 0.8f));
    if (ImGui::Button(I18n::tr(Str::EQ_BTN))) {
        showEqualizerModal = true;
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 200.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.35f, 0.40f, 0.8f));
    if (ImGui::Button(I18n::tr(Str::BTN_MINIMIZE_TRAY), ImVec2(85.0f, 0))) {
        minimizeToTray();
    }

    ImGui::SameLine();
    if (ImGui::Button(I18n::tr(Str::SETTINGS_BTN), ImVec2(105.0f, 0))) {
        showSettingsModal = true;
    }
    ImGui::PopStyleColor(2);

    ImGui::Separator();

    if (activeFolderIdx < 0 || activeFolderIdx >= (int)config.folders.size()) {
        ImGui::EndChild();
        return;
    }

    auto& tracks = config.folders[activeFolderIdx].sounds;
    static ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | 
                                        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("TracksTable", 5, tableFlags)) {
        ImGui::TableSetupColumn(I18n::tr(Str::COL_TITLE), ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(I18n::tr(Str::COL_HOTKEY), ImGuiTableColumnFlags_WidthFixed, 130.0f);
        ImGui::TableSetupColumn(I18n::tr(Str::COL_VOLUME), ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn(I18n::tr(Str::COL_STATE), ImGuiTableColumnFlags_WidthFixed, 95.0f);
        ImGui::TableSetupColumn(I18n::tr(Str::COL_DEL), ImGuiTableColumnFlags_WidthFixed, 32.0f);
        ImGui::TableHeadersRow();

        for (int i = 0; i < (int)tracks.size(); ++i) {
            ImGui::PushID(i);
            ImGui::TableNextRow();

            bool isTrackPlaying = audio.isPlaying(tracks[i].filePath);
            bool isTrackPaused = audio.isPaused(tracks[i].filePath);
            bool isTrackLoading = audio.isLoading(tracks[i].filePath);

            ImGui::TableSetColumnIndex(0);
            if (editingTrackIdx == i) {
                ImGui::SetNextItemWidth(-1);
                if (justStartedEditing) {
                    ImGui::SetKeyboardFocusHere(0);
                    justStartedEditing = false;
                }
                bool entered = ImGui::InputText("##editTitle", renameBuffer, sizeof(renameBuffer), 
                    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
                if (entered || (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0))) {
                    if (strlen(renameBuffer) > 0) tracks[i].name = renameBuffer;
                    editingTrackIdx = -1;
                }
            } else {
                std::string titleStr = (isTrackPlaying ? "> " : (isTrackPaused ? "|| " : "  ")) + tracks[i].name;
                if (isTrackPlaying) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.25f, 0.25f, 1.0f));
                } else if (isTrackPaused) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.70f, 0.20f, 1.0f));
                }

                if (ImGui::Selectable(titleStr.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        editingTrackIdx = i;
                        justStartedEditing = true;
                        strncpy(renameBuffer, tracks[i].name.c_str(), sizeof(renameBuffer) - 1);
                        renameBuffer[sizeof(renameBuffer) - 1] = '\0';
                    }
                }
                if (isTrackPlaying || isTrackPaused) ImGui::PopStyleColor();

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", I18n::tr(Str::TOOLTIP_TRACK));
                }

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                    ImGui::SetDragDropPayload("DND_TRACK_REORDER", &i, sizeof(int));
                    ImGui::Text(I18n::tr(Str::MOVE_TRACK), tracks[i].name.c_str());
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_TRACK_REORDER")) {
                        int src = *(const int*)payload->Data;
                        if (src != i) {
                            std::swap(tracks[src], tracks[i]);
                            GlobalHotkeys::get().updateBindings(tracks);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
            }

            ImGui::TableSetColumnIndex(1);
            std::string hkLabel = tracks[i].hotkey.isEmpty() ? I18n::tr(Str::BTN_BIND) : tracks[i].hotkey.toString();
            if (ImGui::Button(hkLabel.c_str(), ImVec2(-1, 0))) {
                isAssigningHotkey = true;
                targetTrackIndex = i;
                accumulatedCombo = HotkeyCombo();
                hadAnyKeyDown = false;
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderFloat("##vol", &tracks[i].volume, 0.0f, 1.0f, "%.2f")) {
                audio.setVolume(tracks[i].filePath, tracks[i].volume);
            }

            ImGui::TableSetColumnIndex(3);
            if (isTrackLoading) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.70f, 0.20f, 1.0f));
                ImGui::Button(I18n::tr(Str::BTN_LOAD), ImVec2(-1, 0));
                ImGui::PopStyleColor();
            } else if (isTrackPlaying) {
                float halfW = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.60f, 0.12f, 0.12f, 1.0f));
                if (ImGui::Button("||", ImVec2(halfW, 0))) {
                    audio.pause(tracks[i].filePath);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", I18n::tr(Str::BTN_PAUSE));
                ImGui::PopStyleColor();

                ImGui::SameLine(0, 4.0f);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.08f, 0.08f, 1.0f));
                if (ImGui::Button("[]", ImVec2(halfW, 0))) {
                    audio.stop(tracks[i].filePath);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", I18n::tr(Str::BTN_STOP));
                ImGui::PopStyleColor();
            } else if (isTrackPaused) {
                float halfW = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.55f, 0.25f, 1.0f));
                if (ImGui::Button(" >", ImVec2(halfW, 0))) {
                    audio.resume(tracks[i].filePath);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", I18n::tr(Str::BTN_RESUME));
                ImGui::PopStyleColor();

                ImGui::SameLine(0, 4.0f);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.08f, 0.08f, 1.0f));
                if (ImGui::Button("[]", ImVec2(halfW, 0))) {
                    audio.stop(tracks[i].filePath);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", I18n::tr(Str::BTN_STOP));
                ImGui::PopStyleColor();
            } else {
                if (ImGui::Button(I18n::tr(Str::BTN_PLAY), ImVec2(-1, 0))) {
                    playTrack(i, activeFolderIdx);
                }
            }

            ImGui::TableSetColumnIndex(4);
            if (ImGui::Button("X", ImVec2(-1, 0))) {
                audio.stop(tracks[i].filePath);
                tracks.erase(tracks.begin() + i);
                GlobalHotkeys::get().updateBindings(tracks);
                ImGui::PopID();
                break;
            }

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

void App::renderBottomPlayerBar() {
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 68.0f);
    ImGui::BeginChild("BottomPlayerBar", ImVec2(0, 64), true, ImGuiWindowFlags_NoScrollbar);

    std::string currentTrackPath = playingFilePath;
    std::string currentTrackName = playingTrackName.empty() ? I18n::tr(Str::IDLE) : playingTrackName;

    float curSec = audio.getCurrentTime(currentTrackPath);
    float totalSec = audio.getTotalTime(currentTrackPath);
    float effectiveTime = isUserDraggingSeek ? seekScratchTime : curSec;

    char nowBuffer[256];
    snprintf(nowBuffer, sizeof(nowBuffer), I18n::tr(Str::NOW_PLAYING), currentTrackName.c_str());
    ImGui::TextColored(ImVec4(0.9f, 0.25f, 0.25f, 1.0f), "%s", nowBuffer);
    ImGui::SameLine();

    if (!currentTrackPath.empty()) {
        bool isPlayingCurrent = audio.isPlaying(currentTrackPath);
        bool isPausedCurrent = audio.isPaused(currentTrackPath);

        const char* playPauseIcon = isPlayingCurrent ? " || " : " > ";
        if (ImGui::Button(playPauseIcon, ImVec2(28, 0))) {
            if (isPlayingCurrent) audio.pause(currentTrackPath);
            else if (isPausedCurrent) audio.resume(currentTrackPath);
            else audio.play(currentTrackPath, 1.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button(" [] ", ImVec2(28, 0))) {
            audio.stop(currentTrackPath);
            playingFilePath.clear();
            playingTrackName.clear();
            playingTrackIdx = -1;
        }
        ImGui::SameLine();
    }

    if (isUserDraggingSeek) {
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "| %s / %s [Seek]", 
                           formatTime(effectiveTime).c_str(), formatTime(totalSec).c_str());
    } else {
        ImGui::TextDisabled("| %s / %s", formatTime(effectiveTime).c_str(), formatTime(totalSec).c_str());
    }

    float reservedRight = 215.0f;
    float seekWidth = ImGui::GetContentRegionAvail().x - reservedRight;
    if (seekWidth < 80.0f) seekWidth = 80.0f;

    ImVec2 p0 = ImGui::GetCursorScreenPos();
    float sliderHeight = 22.0f;

    float barWidth = std::clamp(config.visualizer_bar_width, 2.0f, 18.0f);
    float barGap = std::max(1.0f, std::round(barWidth * 0.35f));
    int numBars = std::max(6, static_cast<int>(seekWidth / (barWidth + barGap)));

    std::vector<float> spectrum = audio.getVisualizerSpectrum(numBars);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(p0, ImVec2(p0.x + seekWidth, p0.y + sliderHeight), IM_COL32(18, 18, 20, 255));
    drawList->AddRect(p0, ImVec2(p0.x + seekWidth, p0.y + sliderHeight), IM_COL32(45, 45, 50, 255));

    float progressRatio = (totalSec > 0.0f) ? std::clamp(effectiveTime / totalSec, 0.0f, 1.0f) : 0.0f;
    float progressX = p0.x + progressRatio * seekWidth;

    for (int b = 0; b < numBars && b < (int)spectrum.size(); ++b) {
        float xStart = p0.x + b * (barWidth + barGap);
        float xEnd = xStart + barWidth;
        if (xEnd > p0.x + seekWidth - 1.0f) break;

        float h = spectrum[b] * (sliderHeight - 4.0f);
        if (h < 2.0f) h = 2.0f;

        ImVec2 barTop = ImVec2(xStart, p0.y + sliderHeight - 2.0f - h);
        ImVec2 barBottom = ImVec2(xEnd, p0.y + sliderHeight - 2.0f);

        ImU32 barColor = (xEnd <= progressX) ? IM_COL32(229, 57, 53, 235) : IM_COL32(75, 22, 26, 180);
        drawList->AddRectFilled(barTop, barBottom, barColor);
    }

    drawList->AddRectFilled(
        ImVec2(progressX - 1.0f, p0.y),
        ImVec2(progressX + 2.0f, p0.y + sliderHeight),
        IM_COL32(255, 75, 75, 255)
    );

    if (isUserDraggingSeek && totalSec > 0.0f) {
        std::string badgeText = formatTime(seekScratchTime);
        ImVec2 badgeTextSize = ImGui::CalcTextSize(badgeText.c_str());
        float badgeW = badgeTextSize.x + 10.0f;
        float badgeH = badgeTextSize.y + 4.0f;
        ImVec2 badgePos = ImVec2(progressX - badgeW * 0.5f, p0.y - badgeH - 4.0f);

        if (badgePos.x < p0.x) badgePos.x = p0.x;
        if (badgePos.x + badgeW > p0.x + seekWidth) badgePos.x = p0.x + seekWidth - badgeW;

        drawList->AddRectFilled(badgePos, ImVec2(badgePos.x + badgeW, badgePos.y + badgeH), IM_COL32(15, 15, 18, 245), 2.0f);
        drawList->AddRect(badgePos, ImVec2(badgePos.x + badgeW, badgePos.y + badgeH), IM_COL32(229, 57, 53, 255), 2.0f);
        drawList->AddText(ImVec2(badgePos.x + 5.0f, badgePos.y + 2.0f), IM_COL32(255, 255, 255, 255), badgeText.c_str());
    }

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1, 1, 1, 0.04f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(1, 1, 1, 0.08f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0, 0, 0, 0));

    ImGui::SetNextItemWidth(seekWidth);
    float seekPos = isUserDraggingSeek ? seekScratchTime : curSec;
    if (ImGui::SliderFloat("##seek", &seekPos, 0.0f, totalSec > 0.0f ? totalSec : 1.0f, "")) {
        isUserDraggingSeek = true;
        seekScratchTime = seekPos;
        if (!currentTrackPath.empty() && totalSec > 0.0f) {
            audio.seek(currentTrackPath, seekScratchTime);
        }
    }

    if (ImGui::IsItemHovered() && !isUserDraggingSeek && totalSec > 0.0f) {
        float mouseX = ImGui::GetIO().MousePos.x;
        float hoverRatio = std::clamp((mouseX - p0.x) / seekWidth, 0.0f, 1.0f);
        ImGui::SetTooltip("%s", formatTime(hoverRatio * totalSec).c_str());
    }

    if (ImGui::IsItemDeactivated()) {
        if (isUserDraggingSeek && !currentTrackPath.empty() && totalSec > 0.0f) {
            audio.seek(currentTrackPath, seekScratchTime);
        }
        isUserDraggingSeek = false;
    }

    ImGui::PopStyleColor(5);

    ImGui::SameLine();

    ImGui::SetNextItemWidth(95.0f);
    if (ImGui::SliderFloat("##masterVol", &config.master_volume, 0.0f, 100.0f, I18n::tr(Str::VOL_FORMAT))) {
        audio.setMasterVolume(config.master_volume / 100.0f);
    }

    ImGui::SameLine();

    ImGui::SetNextItemWidth(90.0f);
    if (ImGui::SliderFloat("##barWidth", &config.visualizer_bar_width, 2.0f, 16.0f, I18n::tr(Str::BARS_FORMAT))) {
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", I18n::tr(Str::TOOLTIP_BARS));
    }

    ImGui::EndChild();
}

void App::renderEqualizerModal() {
    if (showEqualizerModal && !ImGui::IsPopupOpen(I18n::tr(Str::EQ_TITLE))) {
        ImGui::OpenPopup(I18n::tr(Str::EQ_TITLE));
    }

    if (ImGui::BeginPopupModal(I18n::tr(Str::EQ_TITLE), &showEqualizerModal, ImGuiWindowFlags_AlwaysAutoResize)) {
        bool eqChanged = false;

        if (ImGui::Checkbox(I18n::tr(Str::EQ_ENABLE), &config.eq_enabled)) {
            eqChanged = true;
        }

        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("%s", I18n::tr(Str::EQ_LOW));
        ImGui::SetNextItemWidth(260.0f);
        if (ImGui::SliderFloat("##eqLow", &config.eq_low, -12.0f, 12.0f, "%+.1f dB")) {
            eqChanged = true;
        }

        ImGui::Text("%s", I18n::tr(Str::EQ_MID));
        ImGui::SetNextItemWidth(260.0f);
        if (ImGui::SliderFloat("##eqMid", &config.eq_mid, -12.0f, 12.0f, "%+.1f dB")) {
            eqChanged = true;
        }

        ImGui::Text("%s", I18n::tr(Str::EQ_HIGH));
        ImGui::SetNextItemWidth(260.0f);
        if (ImGui::SliderFloat("##eqHigh", &config.eq_high, -12.0f, 12.0f, "%+.1f dB")) {
            eqChanged = true;
        }

        if (eqChanged) {
            audio.setEQ(config.eq_low, config.eq_mid, config.eq_high, config.eq_enabled);
        }

        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Button(I18n::tr(Str::EQ_RESET), ImVec2(120, 0))) {
            config.eq_low = 0.0f;
            config.eq_mid = 0.0f;
            config.eq_high = 0.0f;
            audio.setEQ(0.0f, 0.0f, 0.0f, config.eq_enabled);
        }

        ImGui::SameLine();
        if (ImGui::Button(I18n::tr(Str::CLOSE), ImVec2(120, 0))) {
            showEqualizerModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void App::renderSettingsModal() {
    if (showSettingsModal && !ImGui::IsPopupOpen(I18n::tr(Str::SETTINGS_TITLE))) {
        ImGui::OpenPopup(I18n::tr(Str::SETTINGS_TITLE));
    }

    if (ImGui::BeginPopupModal(I18n::tr(Str::SETTINGS_TITLE), &showSettingsModal, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", I18n::tr(Str::LANG_SELECT));
        ImGui::Separator();
        ImGui::Spacing();

        int currentLangInt = static_cast<int>(I18n::currentLang());
        if (ImGui::RadioButton("Русский", currentLangInt == 0)) {
            I18n::setLanguage(AppLanguage::RU);
            config.language = "ru";
        }
        if (ImGui::RadioButton("English", currentLangInt == 1)) {
            I18n::setLanguage(AppLanguage::EN);
            config.language = "en";
        }
        if (ImGui::RadioButton("Українська", currentLangInt == 2)) {
            I18n::setLanguage(AppLanguage::UA);
            config.language = "ua";
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Checkbox(I18n::tr(Str::TRAY_MINIMIZE_CHECKBOX), &config.minimize_to_tray);

        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Button(I18n::tr(Str::CLOSE), ImVec2(120, 0))) {
            showSettingsModal = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void App::renderHotkeyModal() {
    if (isAssigningHotkey && !ImGui::IsPopupOpen(I18n::tr(Str::HOTKEY_TITLE))) {
        ImGui::OpenPopup(I18n::tr(Str::HOTKEY_TITLE));
    }

    if (ImGui::BeginPopupModal(I18n::tr(Str::HOTKEY_TITLE), &isAssigningHotkey, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(I18n::tr(Str::HOTKEY_DESC));
        ImGui::Separator();
        ImGui::Spacing();

        ImGuiIO& io = ImGui::GetIO();

        bool anyDown = false;
        if (io.KeyCtrl)  { accumulatedCombo.ctrl = true;  anyDown = true; }
        if (io.KeyShift) { accumulatedCombo.shift = true; anyDown = true; }
        if (io.KeyAlt)   { accumulatedCombo.alt = true;   anyDown = true; }
        if (io.KeySuper) { accumulatedCombo.super = true; anyDown = true; }

        GLFWwindow* win = glfwGetCurrentContext();
        for (int k = GLFW_KEY_SPACE; k <= GLFW_KEY_LAST; ++k) {
            if (k >= GLFW_KEY_LEFT_SHIFT && k <= GLFW_KEY_RIGHT_SUPER) continue;

            if (win && glfwGetKey(win, k) == GLFW_PRESS) {
                accumulatedCombo.key = k;
                anyDown = true;
                break;
            }
        }

        if (anyDown) {
            hadAnyKeyDown = true;
        }

        std::string preview = accumulatedCombo.toString();
        if (preview == "None" || preview.empty()) {
            preview = I18n::tr(Str::HOTKEY_HINT);
        }
        ImGui::TextColored(ImVec4(0.898f, 0.224f, 0.224f, 1.0f), "%s", preview.c_str());

        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Button(I18n::tr(Str::HOTKEY_CLEAR), ImVec2(90, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            if (targetTrackIndex >= 0 && targetTrackIndex < (int)config.folders[activeFolderIdx].sounds.size()) {
                config.folders[activeFolderIdx].sounds[targetTrackIndex].hotkey = HotkeyCombo();
                GlobalHotkeys::get().updateBindings(config.folders[activeFolderIdx].sounds);
            }
            isAssigningHotkey = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button(I18n::tr(Str::HOTKEY_CANCEL), ImVec2(90, 0))) {
            isAssigningHotkey = false;
            ImGui::CloseCurrentPopup();
        }

        if (hadAnyKeyDown && !anyDown) {
            if (targetTrackIndex >= 0 && targetTrackIndex < (int)config.folders[activeFolderIdx].sounds.size()) {
                config.folders[activeFolderIdx].sounds[targetTrackIndex].hotkey = accumulatedCombo;
                GlobalHotkeys::get().updateBindings(config.folders[activeFolderIdx].sounds);
            }
            isAssigningHotkey = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}