#include "GlobalHotkeys.hpp"
#include "PlatformUtils.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static WNDPROC g_originalWndProc = nullptr;

int GlobalHotkeys::glfwToWin32VK(int key) {
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) return 'A' + (key - GLFW_KEY_A);
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) return '0' + (key - GLFW_KEY_0);
    if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F24) return VK_F1 + (key - GLFW_KEY_F1);
    if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) return VK_NUMPAD0 + (key - GLFW_KEY_KP_0);

    switch (key) {
        case GLFW_KEY_SPACE: return VK_SPACE;
        case GLFW_KEY_ENTER: return VK_RETURN;
        case GLFW_KEY_TAB: return VK_TAB;
        case GLFW_KEY_ESCAPE: return VK_ESCAPE;
        case GLFW_KEY_BACKSPACE: return VK_BACK;
        case GLFW_KEY_INSERT: return VK_INSERT;
        case GLFW_KEY_DELETE: return VK_DELETE;
        case GLFW_KEY_PAGE_UP: return VK_PRIOR;
        case GLFW_KEY_PAGE_DOWN: return VK_NEXT;
        case GLFW_KEY_HOME: return VK_HOME;
        case GLFW_KEY_END: return VK_END;
        case GLFW_KEY_LEFT: return VK_LEFT;
        case GLFW_KEY_RIGHT: return VK_RIGHT;
        case GLFW_KEY_UP: return VK_UP;
        case GLFW_KEY_DOWN: return VK_DOWN;
        default: return 0;
    }
}
#else
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <unistd.h>

unsigned long GlobalHotkeys::glfwToX11KeySym(int key) {
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) return XK_a + (key - GLFW_KEY_A);
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) return XK_0 + (key - GLFW_KEY_0);
    if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12) return XK_F1 + (key - GLFW_KEY_F1);

    switch (key) {
        case GLFW_KEY_SPACE: return XK_space;
        case GLFW_KEY_ENTER: return XK_Return;
        case GLFW_KEY_TAB: return XK_Tab;
        case GLFW_KEY_ESCAPE: return XK_Escape;
        case GLFW_KEY_BACKSPACE: return XK_BackSpace;
        case GLFW_KEY_INSERT: return XK_Insert;
        case GLFW_KEY_DELETE: return XK_Delete;
        case GLFW_KEY_PAGE_UP: return XK_Page_Up;
        case GLFW_KEY_PAGE_DOWN: return XK_Page_Down;
        case GLFW_KEY_HOME: return XK_Home;
        case GLFW_KEY_END: return XK_End;
        case GLFW_KEY_LEFT: return XK_Left;
        case GLFW_KEY_RIGHT: return XK_Right;
        case GLFW_KEY_UP: return XK_Up;
        case GLFW_KEY_DOWN: return XK_Down;
        default: return 0;
    }
}
#endif

GlobalHotkeys& GlobalHotkeys::get() {
    static GlobalHotkeys instance;
    return instance;
}

GlobalHotkeys::~GlobalHotkeys() {
    shutdown();
}

void GlobalHotkeys::init(void* nativeWindowHandle, HotkeyCallback callback) {
    m_windowHandle = nativeWindowHandle;
    m_callback = callback;

#if defined(_WIN32)
    HWND hwnd = static_cast<HWND>(m_windowHandle);
    if (hwnd && !g_originalWndProc) {
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        g_originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hwnd, GWLP_WNDPROC, 
            reinterpret_cast<LONG_PTR>(+[](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
                if (uMsg == WM_HOTKEY) {
                    auto* self = reinterpret_cast<GlobalHotkeys*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
                    if (self) {
                        std::lock_guard<std::mutex> lock(self->m_queueMutex);
                        self->m_triggeredIndices.push_back(static_cast<int>(wParam));
                    }
                    return 0;
                }
                return CallWindowProc(g_originalWndProc, hWnd, uMsg, wParam, lParam);
            })
        ));
    }
#else
    if (!m_running) {
        m_running = true;
        m_bindingsDirty = true;
        m_x11Thread = std::thread(&GlobalHotkeys::x11Worker, this);
    }
#endif
    PlatformUtils::log("GlobalHotkeys: Модуль фонового перехвата клавиатуры инициализирован.");
}

void GlobalHotkeys::shutdown() {
#if defined(_WIN32)
    HWND hwnd = static_cast<HWND>(m_windowHandle);
    if (hwnd) {
        for (int i = 0; i < m_registeredWinCount; ++i) UnregisterHotKey(hwnd, i);
        if (g_originalWndProc) {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_originalWndProc));
            g_originalWndProc = nullptr;
        }
    }
    m_registeredWinCount = 0;
#else
    if (m_running) {
        m_running = false;
        if (m_x11Thread.joinable()) m_x11Thread.join();
    }
#endif
}

void GlobalHotkeys::updateBindings(const std::vector<SoundTrack>& tracks) {
#if defined(_WIN32)
    HWND hwnd = static_cast<HWND>(m_windowHandle);
    if (!hwnd) return;

    for (int i = 0; i < m_registeredWinCount; ++i) UnregisterHotKey(hwnd, i);
    m_registeredWinCount = 0;

    for (size_t i = 0; i < tracks.size(); ++i) {
        const auto& hk = tracks[i].hotkey;
        if (hk.isEmpty() || hk.key == GLFW_KEY_UNKNOWN) continue;

        UINT mod = MOD_NOREPEAT;
        if (hk.ctrl)  mod |= MOD_CONTROL;
        if (hk.shift) mod |= MOD_SHIFT;
        if (hk.alt)   mod |= MOD_ALT;
        if (hk.super) mod |= MOD_WIN;

        int vk = glfwToWin32VK(hk.key);
        if (vk != 0 && RegisterHotKey(hwnd, static_cast<int>(i), mod, vk)) {
            m_registeredWinCount = std::max(m_registeredWinCount, static_cast<int>(i) + 1);
        }
    }
#else
    {
        std::lock_guard<std::mutex> lock(m_tracksMutex);
        m_cachedTracks = tracks;
    }
    m_bindingsDirty = true;
#endif
}

#if !defined(_WIN32)
void GlobalHotkeys::x11Worker() {
    Display* dpy = XOpenDisplay(NULL);
    if (!dpy) {
        PlatformUtils::log("GlobalHotkeys: Не удалось открыть X11 Display для фонового перехвата.");
        return;
    }

    Window root = DefaultRootWindow(dpy);
    std::vector<SoundTrack> localTracks;

    const unsigned int lockMasks[] = { 0, Mod2Mask, LockMask, Mod2Mask | LockMask };

    auto rebind = [&]() {
        XUngrabKey(dpy, AnyKey, AnyModifier, root);
        for (size_t i = 0; i < localTracks.size(); ++i) {
            const auto& hk = localTracks[i].hotkey;
            if (hk.isEmpty() || hk.key == GLFW_KEY_UNKNOWN) continue;

            KeySym sym = glfwToX11KeySym(hk.key);
            if (!sym) continue;
            KeyCode code = XKeysymToKeycode(dpy, sym);
            if (!code) continue;

            unsigned int baseMod = 0;
            if (hk.ctrl)  baseMod |= ControlMask;
            if (hk.shift) baseMod |= ShiftMask;
            if (hk.alt)   baseMod |= Mod1Mask;
            if (hk.super) baseMod |= Mod4Mask;

            for (unsigned int mask : lockMasks) {
                XGrabKey(dpy, code, baseMod | mask, root, False, GrabModeAsync, GrabModeAsync);
            }
        }
        XFlush(dpy);
    };

    while (m_running) {
        if (m_bindingsDirty.exchange(false)) {
            {
                std::lock_guard<std::mutex> lock(m_tracksMutex);
                localTracks = m_cachedTracks;
            }
            rebind();
        }

        while (XPending(dpy) > 0) {
            XEvent ev;
            XNextEvent(dpy, &ev);
            if (ev.type == KeyPress) {
                unsigned int cleanState = ev.xkey.state & (ControlMask | ShiftMask | Mod1Mask | Mod4Mask);
                for (size_t i = 0; i < localTracks.size(); ++i) {
                    const auto& hk = localTracks[i].hotkey;
                    if (hk.isEmpty()) continue;

                    unsigned int baseMod = 0;
                    if (hk.ctrl)  baseMod |= ControlMask;
                    if (hk.shift) baseMod |= ShiftMask;
                    if (hk.alt)   baseMod |= Mod1Mask;
                    if (hk.super) baseMod |= Mod4Mask;

                    KeySym sym = glfwToX11KeySym(hk.key);
                    KeyCode code = XKeysymToKeycode(dpy, sym);

                    if (code == ev.xkey.keycode && baseMod == cleanState) {
                        std::lock_guard<std::mutex> qLock(m_queueMutex);
                        m_triggeredIndices.push_back(static_cast<int>(i));
                        break;
                    }
                }
            }
        }
        usleep(20000);
    }

    XUngrabKey(dpy, AnyKey, AnyModifier, root);
    XCloseDisplay(dpy);
}
#endif

void GlobalHotkeys::pollEvents() {
    std::vector<int> triggers;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        triggers.swap(m_triggeredIndices);
    }
    for (int idx : triggers) {
        if (m_callback) m_callback(idx);
    }
}