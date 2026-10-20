#include "TrayManager.hpp"
#include "PlatformUtils.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define WM_TRAYICON_MSG (WM_USER + 200)

TrayManager& TrayManager::get() {
    static TrayManager inst;
    return inst;
}

TrayManager::~TrayManager() {
    shutdown();
}

void TrayManager::init(void* glfwWindowPtr, Callback onRestore, Callback onStopAll, Callback onExit) {
    m_glfwWindow = glfwWindowPtr;
    m_onRestore = onRestore;
    m_onStopAll = onStopAll;
    m_onExit = onExit;

    if (!m_glfwWindow) return;
    HWND hwnd = glfwGetWin32Window((GLFWwindow*)m_glfwWindow);
    m_hwnd = (void*)hwnd;

    NOTIFYICONDATAW nid;
    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hwnd;
    nid.uID = 2026;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON_MSG;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"HotPlay");

    Shell_NotifyIconW(NIM_ADD, &nid);
    m_iconCreated = true;
    PlatformUtils::log("TrayManager: Добавлен значок в системный трей Windows.");
}

void TrayManager::update() {}

void TrayManager::shutdown() {
    if (m_iconCreated && m_hwnd) {
        NOTIFYICONDATAW nid;
        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = (HWND)m_hwnd;
        nid.uID = 2026;
        Shell_NotifyIconW(NIM_DELETE, &nid);
        m_iconCreated = false;
    }
}

#else
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cstring>
#include <vector>
#include <cstdlib>
#include <cstdint>

static std::vector<unsigned long> createHotPlayIconData(int size = 24) {
    std::vector<unsigned long> data;
    data.reserve(2 + size * size);
    data.push_back(size);
    data.push_back(size);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            bool inTriangle = false;
            if (x >= 8 && x <= 17) {
                float progress = static_cast<float>(x - 8) / 9.0f;
                int minY = 6 + static_cast<int>(progress * 6.0f);
                int maxY = 18 - static_cast<int>(progress * 6.0f);
                if (y >= minY && y <= maxY) inTriangle = true;
            }

            if (inTriangle) {
                data.push_back(0xFFFFFFFF);
            } else if (x >= 1 && x < size - 1 && y >= 1 && y < size - 1) {
                data.push_back(0xFFE53935);
            } else {
                data.push_back(0x00000000);
            }
        }
    }
    return data;
}

TrayManager& TrayManager::get() {
    static TrayManager inst;
    return inst;
}

TrayManager::~TrayManager() {
    shutdown();
}

void TrayManager::init(void* glfwWindowPtr, Callback onRestore, Callback onStopAll, Callback onExit) {
    m_glfwWindow = glfwWindowPtr;
    m_onRestore = onRestore;
    m_onStopAll = onStopAll;
    m_onExit = onExit;

    Display* dpy = XOpenDisplay(NULL);
    if (!dpy) {
        PlatformUtils::log("TrayManager: Не удалось открыть X11 Display.");
        return;
    }
    m_display = dpy;

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);

    Atom trayAtom = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
    Window trayOwner = XGetSelectionOwner(dpy, trayAtom);
    if (trayOwner == None) {
        PlatformUtils::log("TrayManager: Системный трей X11 не найден.");
        return;
    }
    m_trayOwner = trayOwner;

    Visual* trayVisual = DefaultVisual(dpy, screen);
    int depth = DefaultDepth(dpy, screen);
    Colormap colormap = DefaultColormap(dpy, screen);

    Atom visualAtom = XInternAtom(dpy, "_NET_SYSTEM_TRAY_VISUAL", False);
    Atom actualType;
    int actualFormat;
    unsigned long nItems = 0, bytesAfter = 0;
    unsigned char* prop = nullptr;

    if (XGetWindowProperty(dpy, trayOwner, visualAtom, 0, 1, False, XA_VISUALID,
                           &actualType, &actualFormat, &nItems, &bytesAfter, &prop) == Success && prop) {
        if (nItems > 0) {
            VisualID vid = *reinterpret_cast<VisualID*>(prop);
            XVisualInfo vinfoTemplate;
            vinfoTemplate.visualid = vid;
            int nVinfo = 0;
            XVisualInfo* vinfo = XGetVisualInfo(dpy, VisualIDMask, &vinfoTemplate, &nVinfo);
            if (vinfo && nVinfo > 0) {
                trayVisual = vinfo->visual;
                depth = vinfo->depth;
                colormap = XCreateColormap(dpy, root, trayVisual, AllocNone);
                XFree(vinfo);
            }
        }
        XFree(prop);
    }

    m_depth = depth;
    m_colormap = colormap;

    XSetWindowAttributes swa;
    swa.colormap = colormap;
    swa.background_pixmap = None;
    swa.border_pixel = 0;
    swa.event_mask = ExposureMask | ButtonPressMask | StructureNotifyMask;

    unsigned long valMask = CWColormap | CWBorderPixel | CWEventMask;
    Window trayWin = XCreateWindow(dpy, root, 0, 0, 24, 24, 0,
                                   depth, InputOutput, trayVisual,
                                   valMask, &swa);
    m_trayWin = trayWin;

    uint32_t* imgPixels = static_cast<uint32_t*>(std::malloc(24 * 24 * sizeof(uint32_t)));
    for (int y = 0; y < 24; ++y) {
        for (int x = 0; x < 24; ++x) {
            bool inTriangle = false;
            if (x >= 8 && x <= 17) {
                float progress = static_cast<float>(x - 8) / 9.0f;
                int minY = 6 + static_cast<int>(progress * 6.0f);
                int maxY = 18 - static_cast<int>(progress * 6.0f);
                if (y >= minY && y <= maxY) inTriangle = true;
            }

            if (inTriangle) {
                imgPixels[y * 24 + x] = 0xFFFFFFFF;
            } else if (x >= 1 && x < 23 && y >= 1 && y < 23) {
                imgPixels[y * 24 + x] = (depth == 32) ? 0xFFE53935 : 0x00E53935;
            } else {
                imgPixels[y * 24 + x] = 0x00000000;
            }
        }
    }

    XImage* ximg = XCreateImage(dpy, trayVisual, depth, ZPixmap, 0, 
                                reinterpret_cast<char*>(imgPixels), 24, 24, 32, 0);
    Pixmap pixmap = XCreatePixmap(dpy, trayWin, 24, 24, depth);
    GC gc = XCreateGC(dpy, pixmap, 0, NULL);
    XPutImage(dpy, pixmap, gc, ximg, 0, 0, 0, 0, 24, 24);
    XDestroyImage(ximg);
    XFreeGC(dpy, gc);
    m_pixmap = pixmap;

    XSetWindowBackgroundPixmap(dpy, trayWin, pixmap);
    XClearWindow(dpy, trayWin);

    Atom xembedAtom = XInternAtom(dpy, "_XEMBED_INFO", False);
    unsigned long xembedData[2] = { 0, 1 };
    XChangeProperty(dpy, trayWin, xembedAtom, xembedAtom, 32, PropModeReplace,
                    reinterpret_cast<unsigned char*>(xembedData), 2);

    Atom netWmIcon = XInternAtom(dpy, "_NET_WM_ICON", False);
    auto iconData = createHotPlayIconData(24);
    XChangeProperty(dpy, trayWin, netWmIcon, XA_CARDINAL, 32, PropModeReplace,
                    reinterpret_cast<unsigned char*>(iconData.data()), iconData.size());

    XClassHint classHint;
    classHint.res_name = const_cast<char*>("hotplay");
    classHint.res_class = const_cast<char*>("HotPlay");
    XSetClassHint(dpy, trayWin, &classHint);
    XStoreName(dpy, trayWin, "HotPlay");

    XClientMessageEvent ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.type = ClientMessage;
    ev.window = trayOwner;
    ev.message_type = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
    ev.format = 32;
    ev.data.l[0] = CurrentTime;
    ev.data.l[1] = 0;
    ev.data.l[2] = trayWin;

    XSendEvent(dpy, trayOwner, False, NoEventMask, reinterpret_cast<XEvent*>(&ev));
    XSync(dpy, False);

    m_isDocked = true;
    PlatformUtils::log("TrayManager: Окно успешно пристыковано к трею XFCE (depth: " + std::to_string(depth) + ")");
}

void TrayManager::update() {
    if (!m_display || !m_isDocked || !m_trayWin) return;
    Display* dpy = static_cast<Display*>(m_display);

    while (XPending(dpy) > 0) {
        XEvent ev;
        XNextEvent(dpy, &ev);

        if (ev.type == Expose && ev.xexpose.count == 0) {
            if (m_pixmap) {
                GC gc = XCreateGC(dpy, m_trayWin, 0, NULL);
                XCopyArea(dpy, m_pixmap, m_trayWin, gc, 0, 0, 24, 24, 0, 0);
                XFreeGC(dpy, gc);
                XFlush(dpy);
            }
        } else if (ev.type == ButtonPress) {
            if (m_onRestore) m_onRestore();
        }
    }
}

void TrayManager::shutdown() {
    if (m_display) {
        Display* dpy = static_cast<Display*>(m_display);
        if (m_pixmap) {
            XFreePixmap(dpy, m_pixmap);
            m_pixmap = 0;
        }
        if (m_trayWin) {
            XDestroyWindow(dpy, m_trayWin);
            m_trayWin = 0;
        }
        if (m_colormap) {
            XFreeColormap(dpy, m_colormap);
            m_colormap = 0;
        }
        XCloseDisplay(dpy);
        m_display = nullptr;
        m_isDocked = false;
    }
}
#endif