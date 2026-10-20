#pragma once

#include <functional>

class TrayManager {
public:
    using Callback = std::function<void()>;

    static TrayManager& get();

    void init(void* glfwWindowPtr, Callback onRestore, Callback onStopAll, Callback onExit);
    void update();
    void shutdown();

private:
    TrayManager() = default;
    ~TrayManager();

    void* m_glfwWindow = nullptr;
    Callback m_onRestore;
    Callback m_onStopAll;
    Callback m_onExit;

#if defined(_WIN32)
    void* m_hwnd = nullptr;
    bool m_iconCreated = false;
#else
    void* m_display = nullptr;
    unsigned long m_trayWin = 0;
    unsigned long m_trayOwner = 0;
    unsigned long m_pixmap = 0;
    unsigned long m_colormap = 0;
    int m_depth = 24;
    bool m_isDocked = false;
#endif
};