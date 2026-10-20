cmake_minimum_required(VERSION 3.20)
project(HotPlay CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.4
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        0.11.21
)
FetchContent_GetProperties(miniaudio)
if(NOT miniaudio_POPULATED)
    FetchContent_Populate(miniaudio)
endif()

FetchContent_Declare(
    kissfft
    GIT_REPOSITORY https://github.com/mborgerding/kissfft.git
    GIT_TAG        131.1.0
)
FetchContent_GetProperties(kissfft)
if(NOT kissfft_POPULATED)
    FetchContent_Populate(kissfft)
endif()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)

find_file(KISS_FFT_C kiss_fft.c PATHS ${kissfft_SOURCE_DIR} NO_DEFAULT_PATH)
find_file(KISS_FFTR_C kiss_fftr.c PATHS ${kissfft_SOURCE_DIR} ${kissfft_SOURCE_DIR}/tools NO_DEFAULT_PATH)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${miniaudio_SOURCE_DIR}
    ${kissfft_SOURCE_DIR}
    ${kissfft_SOURCE_DIR}/tools
    ${glfw_SOURCE_DIR}/include
    ${nlohmann_json_SOURCE_DIR}/include
)

set(SOURCES
    src/main.cpp
    src/App.cpp
    src/AudioEngine.cpp
    src/PlatformUtils.cpp
    src/GlobalHotkeys.cpp
    src/TrayManager.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp
    ${KISS_FFT_C}
    ${KISS_FFTR_C}
)

add_executable(HotPlay ${SOURCES})
target_link_libraries(HotPlay PRIVATE nlohmann_json::nlohmann_json glfw)

if(WIN32)
    target_compile_options(HotPlay PRIVATE -Wall -Wextra)
    target_link_options(HotPlay PRIVATE -static -static-libgcc -static-libstdc++ -mwindows)

    target_link_libraries(HotPlay PRIVATE
        opengl32
        gdi32
        user32
        shell32
        winmm
        ole32
        comdlg32
        imm32
    )
else()
    find_package(OpenGL REQUIRED)
    find_package(ALSA REQUIRED)
    find_package(Threads REQUIRED)
    find_package(X11 REQUIRED)

    target_link_libraries(HotPlay PRIVATE
        OpenGL::GL
        ${CMAKE_DL_LIBS}
        Threads::Threads
        ${ALSA_LIBRARIES}
        ${X11_LIBRARIES}
        m
    )
endif()