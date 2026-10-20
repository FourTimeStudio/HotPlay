#pragma once

#include "Config.hpp"
#include "AudioEngine.hpp"
#include "Localization.hpp"
#include "imgui.h"
#include <string>
#include <vector>

class App {
public:
    App();
    ~App();

    void init(void* nativeWindowHandle);
    void render();
    void update();
    void onFilesDropped(const std::vector<std::string>& filePaths);
    void triggerTrack(int trackIdx);

    bool shouldMinimizeToTray() const { return config.minimize_to_tray; }
    void minimizeToTray();

private:
    void applyFlatStudioTheme();
    void renderFoldersPanel();
    void renderTracksPanel();
    void renderBottomPlayerBar();
    void renderHotkeyModal();
    void renderSettingsModal();
    void renderEqualizerModal();

    void playTrack(int trackIndex, int folderIndex = -1);
    void playNextTrack();
    void playPrevTrack();
    void preloadActiveFolderTracks();
    void deleteFolder(int folderIdx);

    AppConfig config;
    AudioEngine audio;

    int activeFolderIdx = 0;

    int playingFolderIdx = -1;
    int playingTrackIdx = -1;
    std::string playingFilePath;
    std::string playingTrackName;
    int lastPlayedRandomIdx = -1;

    int editingTrackIdx = -1;
    bool justStartedEditing = false;
    char renameBuffer[128] = "";

    bool isUserDraggingSeek = false;
    float seekScratchTime = 0.0f;

    char newFolderNameBuf[128] = "";
    bool isAssigningHotkey = false;
    int targetTrackIndex = -1;
    HotkeyCombo accumulatedCombo;
    bool hadAnyKeyDown = false;

    bool showSettingsModal = false;
    bool showEqualizerModal = false;
};