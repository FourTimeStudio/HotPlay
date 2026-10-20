# HotPlay

<p align="center">
  <strong>Fast, Lightweight & Cross-Platform Soundboard and Audio Player</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-blue.svg" alt="C++17" />
  <img src="https://img.shields.io/badge/GUI-Dear%20ImGui-orange.svg" alt="Dear ImGui" />
  <img src="https://img.shields.io/badge/Audio-miniaudio-green.svg" alt="miniaudio" />
  <img src="https://img.shields.io/badge/FFT-KissFFT-red.svg" alt="KissFFT" />
  <img src="https://img.shields.io/badge/Platform-Linux%20%7C%20Windows-lightgrey.svg" alt="Linux and Windows" />
</p>

---

## Overview

**HotPlay** is a high-performance, low-latency soundboard and media playback application built in modern C++17. It features a studio-grade dark theme, low resource footprint, global system-wide hotkeys, dynamic audio visualization, 3-band equalizer, and background system tray integration.

HotPlay can trigger sound clips or music tracks instantly without interrupting full-screen games, streaming software (OBS, Discord), or workstation applications.

---

## Key Features

- **Global System-Wide Hotkeys**:
  - Bind any key combination (`Ctrl`, `Shift`, `Alt`, `Super`/`Win` + key) to play, pause, resume, or toggle sounds.
  - Native low-overhead hotkey listeners for both **Linux (X11)** and **Windows (Win32)** that capture input even when other applications or games are focused.

- **Broad Audio & Video Container Support**:
  - **Native audio decoding** via `miniaudio`: `.mp3`, `.wav`, `.ogg`, `.flac`.
  - **Universal media decoding** via seamless `FFmpeg` pipe streaming: `.mp4`, `.mkv`, `.webm`, `.avi`, `.mov`, `.wmv`, `.flv`, `.m4a`, `.aac`, `.opus`, and more.
  - Drag-and-drop audio or video files directly into the application window.

- **Folder & Sound Organization**:
  - Multi-tab folder management (e.g., *Memes*, *Music*, *Sound FX*, *Ambience*).
  - Drag-and-drop track reordering within playlists.
  - Inline track renaming (double-click on title).
  - Individual volume adjustments per track.

- **Real-Time Spectrum Visualizer & Interactive Seek Bar**:
  - Hardware-efficient real-time FFT spectrum visualizer powered by `KissFFT`, integrated directly into the progress scrubber.
  - Configurable visualizer bar width and spacing.
  - Smooth seeking with time indicator badges and mouse hover tooltips.

- **3-Band Master Equalizer**:
  - Real-time digital biquad filter (Bass 150 Hz, Mid 1 kHz, Treble 5 kHz) with ±12 dB adjustment and instant reset.

- **Playback Modes**:
  - **Normal**: Plays selected track once.
  - **Repeat [1]**: Continuously loops the active track.
  - **Loop [All]**: Loops sequentially through all tracks in the active folder.
  - **Shuffle**: Non-repeating randomized playback across the folder.

- **System Tray & Desktop Notifications**:
  - Minimizes cleanly to the system notification area on window close (`[X]`) or via the *To Tray* button.
  - Native desktop notifications (`notify-send` on Linux, Shell notification on Windows) when tracks begin playing.

- **Multi-Language Interface (i18n)**:
  - English (`en`)
  - Russian (`ru`)
  - Ukrainian (`ua`)
  - Instantly switchable at runtime with state saved across launches.

---

## Tech Stack & Architecture

- **Language**: C++17
- **GUI Framework**: [Dear ImGui](https://github.com/ocornut/imgui) (OpenGL2 / GLFW backend)
- **Audio Engine**: [miniaudio](https://github.com/mackron/miniaudio) (low-latency PCM pipeline with custom Biquad EQ filters)
- **Fast Fourier Transform**: [KissFFT](https://github.com/mborgerding/kissfft)
- **Windowing & Input**: [GLFW 3.4](https://github.com/glfw/glfw)
- **Configuration & Persistence**: [nlohmann/json](https://github.com/nlohmann/json)
- **Build System**: CMake 3.20+ (all core third-party C++ libraries are fetched automatically via `FetchContent`)

---

## Prerequisites

### Linux (Debian / Ubuntu / Mint)

Install the necessary development libraries and tools:

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config \
    libx11-dev libasound2-dev libgl1-mesa-dev libxcursor-dev \
    libxinerama-dev libxi-dev libxrandr-dev ffmpeg libnotify-bin
```

### Windows (Cross-compiling from Linux or Native MinGW)

For cross-compiling from Linux to Windows using MinGW-w64:

```bash
sudo apt install -y mingw-w64
```

For video container decoding (`.mp4`, `.mkv`, etc.) on Windows, place `ffmpeg.exe` in the same directory as `HotPlay.exe` or ensure it is available in your system `PATH`.

---

## Building

### 1. Build for Linux

```bash
mkdir -p build && cd build
cmake ..
cmake --build . -j$(nproc)
```

Run HotPlay:
```bash
./HotPlay
```

### 2. Cross-Compile for Windows (from Linux)

HotPlay is statically linked for Windows (`-static -static-libgcc -static-libstdc++ -mwindows`), producing a portable, standalone executable with zero external runtime DLL requirements:

```bash
mkdir -p build-win && cd build-win
cmake .. \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc-win32 \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++-win32 \
  -DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32
cmake --build . -j$(nproc)
```

The resulting binary will be generated at `build-win/HotPlay.exe`.

---

## Getting Started

1. **Add Sounds**:
   - Click **+ Add Tracks** to open the system file chooser, or drag and drop audio/video files directly into the window.
2. **Assign Hotkeys**:
   - Click the **[ BIND ]** button next to any track.
   - Press and hold your desired key combination (e.g. `Ctrl + Alt + 1`, `F8`, `Shift + Num 5`) and release.
   - Press the hotkey anytime — even in game — to play or toggle pause.
3. **Organize Playlists**:
   - Create new folders using the **New folder...** input on the left panel.
   - Drag and drop tracks to reorder them within a folder.
4. **Tune Audio**:
   - Open the **Equalizer** to adjust Bass, Mid, and Treble frequencies.
   - Adjust the visualizer bar width with the bottom slider to match your display preferences.
5. **Background Mode**:
   - Check **Minimize to system tray on close** in Settings.
   - Close the window to keep HotPlay running quietly in the tray.

---

## Configuration

HotPlay automatically persists all user settings, playlists, volume levels, and hotkey bindings in `hotplay_config.json` in the working directory:

```json
{
    "playback_mode": 0,
    "master_volume": 100.0,
    "minimize_to_tray": true,
    "language": "eng",
    "eq_enabled": true,
    "eq_low": 2.0,
    "eq_mid": 0.0,
    "eq_high": 1.5,
    "folders": [ ... ]
}
```

---

## License

This project is licensed under the [MIT License](LICENSE) (or Public Domain where applicable). See individual third-party libraries for their respective licenses.
