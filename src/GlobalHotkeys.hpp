#pragma once

#include "Config.hpp"
#include <functional>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>

class GlobalHotkeys {
public:
    using HotkeyCallback = std::function<void(int identifier)>;

    static GlobalHotkeys& get();

    void init(void* nativeWindowHandle, HotkeyCallback callback);
    void shutdown();
    void updateBindings(const std::vector<SoundTrack>& tracks);
    void pollEvents();

private:
    GlobalHotkeys() = default;
    ~GlobalHotkeys();

    void* m_windowHandle = nullptr;
    HotkeyCallback m_callback;

    std::mutex m_queueMutex;
    std::vector<int> m_triggeredIndices;

#if defined(_WIN32)
    int m_registeredWinCount = 0;
    static int glfwToWin32VK(int glfwKey);
#else
    std::thread m_x11Thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_bindingsDirty{false};
    std::vector<SoundTrack> m_cachedTracks;
    std::mutex m_tracksMutex;

    void x11Worker();
    static unsigned long glfwToX11KeySym(int glfwKey);
#endif
};