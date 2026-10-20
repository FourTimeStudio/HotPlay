#include "App.hpp"
#include "PlatformUtils.hpp"
#include "TrayManager.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <filesystem>

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#else
#include <X11/Xlib.h>
#endif

namespace fs = std::filesystem;

static App* g_appPtr = nullptr;
static GLFWwindow* g_windowPtr = nullptr;

static void restoreWindow() {
    if (g_windowPtr) {
        glfwShowWindow(g_windowPtr);
        glfwRestoreWindow(g_windowPtr);
        glfwFocusWindow(g_windowPtr);
    }
}

static void loadSystemCyrillicFont(ImGuiIO& io) {
    const std::vector<std::string> fontCandidates = {
        "font.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\tahoma.ttf"
    };

    for (const auto& path : fontCandidates) {
        if (fs::exists(path)) {
            ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), 16.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
            if (font) {
                PlatformUtils::log("Загружен шрифт с поддержкой кириллицы: " + path);
                return;
            }
        }
    }
    io.Fonts->AddFontDefault();
}

int main() {
#if !defined(_WIN32)
    XInitThreads();
#endif

    PlatformUtils::initLogger();
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    GLFWwindow* window = glfwCreateWindow(1080, 680, "HotPlay - Soundboard", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    g_windowPtr = window;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetWindowCloseCallback(window, [](GLFWwindow* w) {
        if (g_appPtr && g_appPtr->shouldMinimizeToTray()) {
            glfwSetWindowShouldClose(w, GLFW_FALSE);
            g_appPtr->minimizeToTray();
        } else {
            glfwSetWindowShouldClose(w, GLFW_TRUE);
        }
    });

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    loadSystemCyrillicFont(io);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    void* nativeHwnd = nullptr;
#if defined(_WIN32)
    nativeHwnd = reinterpret_cast<void*>(glfwGetWin32Window(window));
#endif

    App app;
    g_appPtr = &app;
    app.init(nativeHwnd);

    TrayManager::get().init(window, restoreWindow, []() {}, []() {
        if (g_windowPtr) glfwSetWindowShouldClose(g_windowPtr, GLFW_TRUE);
    });

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        TrayManager::get().update();

        app.update();

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        app.render();

        ImGui::Render();
        int dw, dh;
        glfwGetFramebufferSize(window, &dw, &dh);
        glViewport(0, 0, dw, dh);
        glClearColor(0.063f, 0.063f, 0.071f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    TrayManager::get().shutdown();
    g_appPtr = nullptr;
    g_windowPtr = nullptr;

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}