#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "AudioEngine.hpp"
#include "PlatformUtils.hpp"
#include "kiss_fftr.h"

#include <cmath>
#include <mutex>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <cstring>
#include <filesystem>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace fs = std::filesystem;

constexpr int FFT_SIZE = 512;
constexpr int RING_BUFFER_SIZE = 4096;

enum class TrackStatus {
    NotLoaded,
    Loading,
    Ready,
    Failed
};

struct TrackState {
    ma_sound sound;
    ma_audio_buffer buffer;
    std::vector<float> pcmData;
    std::atomic<TrackStatus> status{TrackStatus::NotLoaded};
    std::atomic<bool> playWhenReady{false};
    std::atomic<bool> isPaused{false};
    std::atomic<float> requestedVolume{1.0f};
    bool wasStarted = false;
    ma_uint32 sampleRate = 44100;
};

struct Biquad {
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    float x1_l = 0.0f, x2_l = 0.0f, y1_l = 0.0f, y2_l = 0.0f;
    float x1_r = 0.0f, x2_r = 0.0f, y1_r = 0.0f, y2_r = 0.0f;

    void reset() {
        x1_l = x2_l = y1_l = y2_l = 0.0f;
        x1_r = x2_r = y1_r = y2_r = 0.0f;
    }

    void setLowShelf(float f0, float gainDb, float sRate = 44100.0f) {
        if (std::abs(gainDb) < 0.05f) {
            b0 = 1.0f; b1 = b2 = a1 = a2 = 0.0f;
            return;
        }
        float A = std::pow(10.0f, gainDb / 40.0f);
        float w0 = 2.0f * 3.14159265f * f0 / sRate;
        float cw = std::cos(w0);
        float sw = std::sin(w0);
        float alpha = (sw / 2.0f) * 1.41421356f;

        float a0_inv = 1.0f / ((A + 1.0f) + (A - 1.0f) * cw + 2.0f * std::sqrt(A) * alpha);
        b0 = (A * ((A + 1.0f) - (A - 1.0f) * cw + 2.0f * std::sqrt(A) * alpha)) * a0_inv;
        b1 = (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cw)) * a0_inv;
        b2 = (A * ((A + 1.0f) - (A - 1.0f) * cw - 2.0f * std::sqrt(A) * alpha)) * a0_inv;
        a1 = (-2.0f * ((A - 1.0f) + (A + 1.0f) * cw)) * a0_inv;
        a2 = ((A + 1.0f) + (A - 1.0f) * cw - 2.0f * std::sqrt(A) * alpha) * a0_inv;
    }

    void setPeaking(float f0, float gainDb, float Q = 1.0f, float sRate = 44100.0f) {
        if (std::abs(gainDb) < 0.05f) {
            b0 = 1.0f; b1 = b2 = a1 = a2 = 0.0f;
            return;
        }
        float A = std::pow(10.0f, gainDb / 40.0f);
        float w0 = 2.0f * 3.14159265f * f0 / sRate;
        float cw = std::cos(w0);
        float sw = std::sin(w0);
        float alpha = sw / (2.0f * Q);

        float a0_inv = 1.0f / (1.0f + alpha / A);
        b0 = (1.0f + alpha * A) * a0_inv;
        b1 = (-2.0f * cw) * a0_inv;
        b2 = (1.0f - alpha * A) * a0_inv;
        a1 = (-2.0f * cw) * a0_inv;
        a2 = (1.0f - alpha / A) * a0_inv;
    }

    void setHighShelf(float f0, float gainDb, float sRate = 44100.0f) {
        if (std::abs(gainDb) < 0.05f) {
            b0 = 1.0f; b1 = b2 = a1 = a2 = 0.0f;
            return;
        }
        float A = std::pow(10.0f, gainDb / 40.0f);
        float w0 = 2.0f * 3.14159265f * f0 / sRate;
        float cw = std::cos(w0);
        float sw = std::sin(w0);
        float alpha = (sw / 2.0f) * 1.41421356f;

        float a0_inv = 1.0f / ((A + 1.0f) - (A - 1.0f) * cw + 2.0f * std::sqrt(A) * alpha);
        b0 = (A * ((A + 1.0f) + (A - 1.0f) * cw + 2.0f * std::sqrt(A) * alpha)) * a0_inv;
        b1 = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cw)) * a0_inv;
        b2 = (A * ((A + 1.0f) + (A - 1.0f) * cw - 2.0f * std::sqrt(A) * alpha)) * a0_inv;
        a1 = (2.0f * ((A - 1.0f) - (A + 1.0f) * cw)) * a0_inv;
        a2 = ((A + 1.0f) - (A - 1.0f) * cw - 2.0f * std::sqrt(A) * alpha) * a0_inv;
    }

    inline void process(float& inL, float& inR) {
        float y_l = b0 * inL + b1 * x1_l + b2 * x2_l - a1 * y1_l - a2 * y2_l;
        x2_l = x1_l; x1_l = inL;
        y2_l = y1_l; y1_l = y_l;
        inL = y_l;

        float y_r = b0 * inR + b1 * x1_r + b2 * x2_r - a1 * y1_r - a2 * y2_r;
        x2_r = x1_r; x1_r = inR;
        y2_r = y1_r; y1_r = y_r;
        inR = y_r;
    }
};

struct AudioEngine::Impl {
    ma_engine engine;
    ma_device device;
    bool isInitialized = false;
    float masterVolume = 1.0f;

    std::unordered_map<std::string, std::shared_ptr<TrackState>> tracks;
    std::mutex tracksMutex;
    std::mutex engineMutex;

    Biquad eqLow;
    Biquad eqMid;
    Biquad eqHigh;
    bool eqEnabled = true;
    std::mutex eqMutex;

    kiss_fftr_cfg fftConfig = nullptr;
    float ringBuffer[RING_BUFFER_SIZE] = {0.0f};
    int ringWriteIndex = 0;
    std::mutex ringMutex;
    std::vector<float> smoothedBars = std::vector<float>(16, 0.02f);

#if defined(_WIN32)
    static std::wstring getFFmpegPath() {
        wchar_t exePath[MAX_PATH];
        if (GetModuleFileNameW(NULL, exePath, MAX_PATH)) {
            fs::path p(exePath);
            fs::path localFfmpeg = p.parent_path() / "ffmpeg.exe";
            if (fs::exists(localFfmpeg)) {
                return localFfmpeg.wstring();
            }
        }
        return L"ffmpeg.exe";
    }
#endif

    std::shared_ptr<TrackState> getOrCreateTrack(const std::string& filePath) {
        std::lock_guard<std::mutex> lock(tracksMutex);
        auto it = tracks.find(filePath);
        if (it != tracks.end()) {
            return it->second;
        }

        auto state = std::make_shared<TrackState>();
        state->status = TrackStatus::Loading;
        tracks[filePath] = state;

        std::thread([this, filePath, state]() {
            fs::path p(filePath);
            std::string ext = p.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            bool isPureAudio = (ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".ogg");
            bool decodeSuccess = false;
            std::vector<float> localPcm;

            if (isPureAudio) {
                ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 44100);
                ma_decoder decoder;
                ma_result res;

#if defined(_WIN32)
                std::wstring wpath = PlatformUtils::utf8ToWide(filePath);
                res = ma_decoder_init_file_w(wpath.c_str(), &decoderConfig, &decoder);
#else
                res = ma_decoder_init_file(filePath.c_str(), &decoderConfig, &decoder);
#endif

                if (res == MA_SUCCESS) {
                    constexpr ma_uint64 CHUNK_FRAMES = 8192;
                    float chunk[CHUNK_FRAMES * 2];
                    ma_uint64 framesRead = 0;

                    do {
                        ma_result r = ma_decoder_read_pcm_frames(&decoder, chunk, CHUNK_FRAMES, &framesRead);
                        if (framesRead > 0) {
                            size_t old = localPcm.size();
                            localPcm.resize(old + framesRead * 2);
                            std::memcpy(localPcm.data() + old, chunk, framesRead * 2 * sizeof(float));
                        }
                        if (r != MA_SUCCESS || framesRead == 0) break;
                    } while (framesRead == CHUNK_FRAMES);

                    ma_decoder_uninit(&decoder);
                    if (!localPcm.empty()) decodeSuccess = true;
                }
            }

            if (!decodeSuccess) {
#if defined(_WIN32)
                std::wstring ffmpegExe = getFFmpegPath();
                std::wstring wpath = PlatformUtils::utf8ToWide(filePath);

                HANDLE hReadPipe, hWritePipe;
                SECURITY_ATTRIBUTES sa;
                sa.nLength = sizeof(SECURITY_ATTRIBUTES);
                sa.bInheritHandle = TRUE;
                sa.lpSecurityDescriptor = NULL;

                if (CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
                    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

                    STARTUPINFOW si;
                    PROCESS_INFORMATION pi;
                    ZeroMemory(&si, sizeof(si));
                    si.cb = sizeof(si);
                    si.hStdOutput = hWritePipe;
                    si.hStdError = NULL;
                    si.hStdInput = NULL;
                    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
                    si.wShowWindow = SW_HIDE;

                    std::wstring cmdLine = L"\"" + ffmpegExe + L"\" -v error -nostdin -i \"" + wpath + L"\" -vn -f f32le -ac 2 -ar 44100 -";
                    std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
                    cmdBuf.push_back(0);

                    if (CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                        CloseHandle(hWritePipe);

                        constexpr DWORD BUF_SIZE = 8192;
                        float buf[BUF_SIZE];
                        DWORD bytesRead = 0;
                        while (ReadFile(hReadPipe, buf, sizeof(buf), &bytesRead, NULL) && bytesRead > 0) {
                            size_t numFloats = bytesRead / sizeof(float);
                            localPcm.insert(localPcm.end(), buf, buf + numFloats);
                        }

                        WaitForSingleObject(pi.hProcess, INFINITE);
                        CloseHandle(pi.hProcess);
                        CloseHandle(pi.hThread);
                        CloseHandle(hReadPipe);

                        if (!localPcm.empty()) decodeSuccess = true;
                    } else {
                        CloseHandle(hWritePipe);
                        CloseHandle(hReadPipe);
                        PlatformUtils::log("AudioEngine Error: ffmpeg.exe не найден! Для MP4/видео положите ffmpeg.exe рядом с HotPlay.exe");
                    }
                }
#else
                std::string cmd = "ffmpeg -v error -nostdin -i \"" + filePath + "\" -vn -f f32le -ac 2 -ar 44100 -";
                FILE* pipe = popen(cmd.c_str(), "r");
                if (pipe) {
                    constexpr size_t BUF_SIZE = 8192;
                    float buf[BUF_SIZE];
                    size_t readFloats = 0;
                    while ((readFloats = fread(buf, sizeof(float), BUF_SIZE, pipe)) > 0) {
                        localPcm.insert(localPcm.end(), buf, buf + readFloats);
                    }
                    pclose(pipe);
                    if (!localPcm.empty()) decodeSuccess = true;
                }
#endif
            }

            if (!decodeSuccess || localPcm.empty()) {
                PlatformUtils::log("AudioEngine: Не удалось распаковать аудио: " + filePath);
                state->status = TrackStatus::Failed;
                return;
            }

            state->pcmData = std::move(localPcm);
            ma_uint64 totalFrames = state->pcmData.size() / 2;

            {
                std::lock_guard<std::mutex> eLock(engineMutex);
                if (!isInitialized) {
                    state->status = TrackStatus::Failed;
                    return;
                }

                ma_audio_buffer_config bufConfig = ma_audio_buffer_config_init(
                    ma_format_f32, 2, totalFrames, state->pcmData.data(), NULL
                );
                if (ma_audio_buffer_init(&bufConfig, &state->buffer) != MA_SUCCESS) {
                    state->status = TrackStatus::Failed;
                    return;
                }

                if (ma_sound_init_from_data_source(&engine, &state->buffer, 0, NULL, &state->sound) != MA_SUCCESS) {
                    ma_audio_buffer_uninit(&state->buffer);
                    state->status = TrackStatus::Failed;
                    return;
                }

                state->sampleRate = 44100;
                state->status = TrackStatus::Ready;

                if (state->playWhenReady.load()) {
                    ma_sound_seek_to_pcm_frame(&state->sound, 0);
                    ma_sound_set_volume(&state->sound, state->requestedVolume.load() * masterVolume);
                    ma_sound_start(&state->sound);
                    state->wasStarted = true;
                    state->isPaused = false;
                    state->playWhenReady = false;
                }
            }
            PlatformUtils::log("AudioEngine: Медиафайл готов к воспроизведению: " + filePath);
        }).detach();

        return state;
    }

    void pushSamples(const float* frames, ma_uint32 frameCount, ma_uint32 channels) {
        std::lock_guard<std::mutex> lock(ringMutex);
        for (ma_uint32 i = 0; i < frameCount; ++i) {
            float mono = 0.0f;
            if (channels == 1) {
                mono = frames[i];
            } else if (channels >= 2) {
                mono = (frames[i * channels] + frames[i * channels + 1]) * 0.5f;
            }
            ringBuffer[ringWriteIndex] = mono;
            ringWriteIndex = (ringWriteIndex + 1) % RING_BUFFER_SIZE;
        }
    }
};

static void audio_device_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    (void)pInput;
    auto* pImpl = static_cast<AudioEngine::Impl*>(pDevice->pUserData);
    if (!pImpl || !pImpl->isInitialized) {
        std::memset(pOutput, 0, frameCount * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels));
        return;
    }

    {
        std::lock_guard<std::mutex> lock(pImpl->engineMutex);
        ma_engine_read_pcm_frames(&pImpl->engine, pOutput, frameCount, NULL);
    }

    float* fOut = static_cast<float*>(pOutput);
    if (pImpl->eqEnabled) {
        std::lock_guard<std::mutex> lock(pImpl->eqMutex);
        for (ma_uint32 i = 0; i < frameCount; ++i) {
            float& left = fOut[i * 2];
            float& right = fOut[i * 2 + 1];
            pImpl->eqLow.process(left, right);
            pImpl->eqMid.process(left, right);
            pImpl->eqHigh.process(left, right);
        }
    }

    pImpl->pushSamples(fOut, frameCount, pDevice->playback.channels);
}

AudioEngine::AudioEngine() : pImpl(std::make_unique<Impl>()) {
    pImpl->fftConfig = kiss_fftr_alloc(FFT_SIZE, 0, nullptr, nullptr);
    setEQ(0.0f, 0.0f, 0.0f, true);
}

AudioEngine::~AudioEngine() {
    stopAll();
    if (pImpl->isInitialized) {
        ma_device_uninit(&pImpl->device);
        {
            std::lock_guard<std::mutex> lock(pImpl->tracksMutex);
            for (auto& [path, st] : pImpl->tracks) {
                if (st->status == TrackStatus::Ready) {
                    ma_sound_uninit(&st->sound);
                    ma_audio_buffer_uninit(&st->buffer);
                }
            }
        }
        ma_engine_uninit(&pImpl->engine);
    }
    if (pImpl->fftConfig) {
        kiss_fftr_free(pImpl->fftConfig);
    }
}

bool AudioEngine::init() {
    std::lock_guard<std::mutex> lock(pImpl->engineMutex);

    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = ma_format_f32;
    deviceConfig.playback.channels = 2;
    deviceConfig.sampleRate        = 44100;
    deviceConfig.dataCallback      = audio_device_callback;
    deviceConfig.pUserData         = pImpl.get();

    if (ma_device_init(NULL, &deviceConfig, &pImpl->device) != MA_SUCCESS) {
        return false;
    }

    ma_engine_config engineConfig = ma_engine_config_init();
    engineConfig.noDevice   = MA_TRUE;
    engineConfig.channels   = pImpl->device.playback.channels;
    engineConfig.sampleRate = pImpl->device.sampleRate;

    if (ma_engine_init(&engineConfig, &pImpl->engine) != MA_SUCCESS) {
        ma_device_uninit(&pImpl->device);
        return false;
    }

    if (ma_device_start(&pImpl->device) != MA_SUCCESS) {
        ma_engine_uninit(&pImpl->engine);
        ma_device_uninit(&pImpl->device);
        return false;
    }

    pImpl->isInitialized = true;
    PlatformUtils::log("AudioEngine: Успешно инициализирован.");
    return true;
}

void AudioEngine::setEQ(float lowDb, float midDb, float highDb, bool enabled) {
    std::lock_guard<std::mutex> lock(pImpl->eqMutex);
    pImpl->eqEnabled = enabled;
    pImpl->eqLow.setLowShelf(150.0f, lowDb, 44100.0f);
    pImpl->eqMid.setPeaking(1000.0f, midDb, 1.0f, 44100.0f);
    pImpl->eqHigh.setHighShelf(5000.0f, highDb, 44100.0f);
}

void AudioEngine::preload(const std::string& filePath) {
    pImpl->getOrCreateTrack(filePath);
}

void AudioEngine::play(const std::string& filePath, float volume) {
    auto track = pImpl->getOrCreateTrack(filePath);
    if (!track) return;

    if (track->status == TrackStatus::Ready) {
        std::lock_guard<std::mutex> lock(pImpl->engineMutex);
        ma_sound_stop(&track->sound);
        ma_sound_seek_to_pcm_frame(&track->sound, 0);
        ma_sound_set_volume(&track->sound, volume * pImpl->masterVolume);
        ma_sound_start(&track->sound);
        track->wasStarted = true;
        track->isPaused = false;
    } else {
        track->requestedVolume = volume;
        track->playWhenReady = true;
    }
}

void AudioEngine::pause(const std::string& filePath) {
    std::shared_ptr<TrackState> track;
    {
        std::lock_guard<std::mutex> lock(pImpl->tracksMutex);
        auto it = pImpl->tracks.find(filePath);
        if (it != pImpl->tracks.end()) track = it->second;
    }

    if (track && track->status == TrackStatus::Ready) {
        std::lock_guard<std::mutex> lock(pImpl->engineMutex);
        ma_sound_stop(&track->sound);
        track->isPaused = true;
    }
}

void AudioEngine::resume(const std::string& filePath) {
    std::shared_ptr<TrackState> track;
    {
        std::lock_guard<std::mutex> lock(pImpl->tracksMutex);
        auto it = pImpl->tracks.find(filePath);
        if (it != pImpl->tracks.end()) track = it->second;
    }

    if (track && track->status == TrackStatus::Ready) {
        std::lock_guard<std::mutex> lock(pImpl->engineMutex);
        ma_sound_start(&track->sound);
        track->isPaused = false;
        track->wasStarted = true;
    }
}

void AudioEngine::togglePause(const std::string& filePath, float volume) {
    if (isPlaying(filePath)) {
        pause(filePath);
    } else if (isPaused(filePath)) {
        resume(filePath);
    } else {
        play(filePath, volume);
    }
}

void AudioEngine::togglePlay(const std::string& filePath, float volume) {
    if (isPlaying(filePath) || isPaused(filePath)) {
        stop(filePath);
    } else {
        play(filePath, volume);
    }
}

void AudioEngine::stop(const std::string& filePath) {
    std::shared_ptr<TrackState> track;
    {
        std::lock_guard<std::mutex> lock(pImpl->tracksMutex);
        auto it = pImpl->tracks.find(filePath);
        if (it != pImpl->tracks.end()) track = it->second;
    }

    if (track) {
        track->playWhenReady = false;
        track->isPaused = false;
        if (track->status == TrackStatus::Ready) {
            std::lock_guard<std::mutex> lock(pImpl->engineMutex);
            ma_sound_stop(&track->sound);
            ma_sound_seek_to_pcm_frame(&track->sound, 0);
            track->wasStarted = false;
        }
    }
}

void AudioEngine::stopAll() {
    std::lock_guard<std::mutex> tLock(pImpl->tracksMutex);
    std::lock_guard<std::mutex> eLock(pImpl->engineMutex);
    for (auto& [path, st] : pImpl->tracks) {
        st->playWhenReady = false;
        st->isPaused = false;
        if (st->status == TrackStatus::Ready) {
            ma_sound_stop(&st->sound);
            ma_sound_seek_to_pcm_frame(&st->sound, 0);
            st->wasStarted = false;
        }
    }
}

bool AudioEngine::isPlaying(const std::string& filePath) {
    std::shared_ptr<TrackState> track;
    {
        std::lock_guard<std::mutex> lock(pImpl->tracksMutex);
        auto it = pImpl->tracks.find(filePath);
        if (it != pImpl->tracks.end()) track = it->second;
    }

    if (track && track->status == TrackStatus::Ready) {
        std::lock_guard<std::mutex> lock(pImpl->engineMutex);
        return ma_sound_is_playing(&track->sound) == MA_TRUE;
    }
    return false;
}

bool AudioEngine::isPaused(const std::string& filePath) {
    std::shared_ptr<TrackState> track;
    {
        std::lock_guard<std::mutex> lock(pImpl->tracksMutex);
        auto it = pImpl->tracks.find(filePath);
        if (it != pImpl->tracks.end()) track = it->second;
    }
    return track ? track->isPaused.load() : false;
}

bool AudioEngine::isAnyPlaying() {
    std::lock_guard<std::mutex> tLock(pImpl->tracksMutex);
    std::lock_guard<std::mutex> eLock(pImpl->engineMutex);
    for (auto& [path, st] : pImpl->tracks) {
        if (st->status == TrackStatus::Ready && ma_sound_is_playing(&st->sound) == MA_TRUE) {
            return true;
        }
    }
    return false;
}

bool AudioEngine::isLoading(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(pImpl->tracksMutex);
    auto it = pImpl->tracks.find(filePath);
    if (it != pImpl->tracks.end()) {
        return it->second->status == TrackStatus::Loading;
    }
    return false;
}

bool AudioEngine::hasEnded(const std::string& filePath) {
    std::shared_ptr<TrackState> track;
    {
        std::lock_guard<std::mutex> lock(pImpl->tracksMutex);
        auto it = pImpl->tracks.find(filePath);
        if (it != pImpl->tracks.end()) track = it->second;
    }

    if (track && track->status == TrackStatus::Ready && track->wasStarted && !track->isPaused.load()) {
        std::lock_guard<std::mutex> lock(pImpl->engineMutex);
        ma_uint64 cursor = 0;
        ma_sound_get_cursor_in_pcm_frames(&track->sound, &cursor);
        ma_uint64 totalFrames = track->pcmData.size() / 2;

        bool isAtEnd = (ma_sound_at_end(&track->sound) == MA_TRUE);
        bool isCursorPastEnd = (totalFrames > 0 && cursor >= totalFrames);
        bool isStoppedNaturally = (ma_sound_is_playing(&track->sound) == MA_FALSE && cursor >= totalFrames);

        if (isAtEnd || isCursorPastEnd || isStoppedNaturally) {
            track->wasStarted = false;
            return true;
        }
    }
    return false;
}

void AudioEngine::setVolume(const std::string& filePath, float volume) {
    std::shared_ptr<TrackState> track;
    {
        std::lock_guard<std::mutex> lock(pImpl->tracksMutex);
        auto it = pImpl->tracks.find(filePath);
        if (it != pImpl->tracks.end()) track = it->second;
    }

    if (track) {
        track->requestedVolume = volume;
        if (track->status == TrackStatus::Ready) {
            std::lock_guard<std::mutex> lock(pImpl->engineMutex);
            ma_sound_set_volume(&track->sound, volume * pImpl->masterVolume);
        }
    }
}

float AudioEngine::getCurrentTime(const std::string& filePath) {
    std::shared_ptr<TrackState> track;
    {
        std::lock_guard<std::mutex> lock(pImpl->tracksMutex);
        auto it = pImpl->tracks.find(filePath);
        if (it != pImpl->tracks.end()) track = it->second;
    }

    if (track && track->status == TrackStatus::Ready) {
        std::lock_guard<std::mutex> lock(pImpl->engineMutex);
        ma_uint64 cursor = 0;
        ma_sound_get_cursor_in_pcm_frames(&track->sound, &cursor);
        return static_cast<float>(cursor) / static_cast<float>(track->sampleRate);
    }
    return 0.0f;
}

float AudioEngine::getTotalTime(const std::string& filePath) {
    std::shared_ptr<TrackState> track;
    {
        std::lock_guard<std::mutex> lock(pImpl->tracksMutex);
        auto it = pImpl->tracks.find(filePath);
        if (it != pImpl->tracks.end()) track = it->second;
    }

    if (track && !track->pcmData.empty()) {
        return static_cast<float>(track->pcmData.size() / 2) / static_cast<float>(track->sampleRate);
    }
    return 0.0f;
}

void AudioEngine::seek(const std::string& filePath, float targetTimeSeconds) {
    std::shared_ptr<TrackState> track;
    {
        std::lock_guard<std::mutex> lock(pImpl->tracksMutex);
        auto it = pImpl->tracks.find(filePath);
        if (it != pImpl->tracks.end()) track = it->second;
    }

    if (track && track->status == TrackStatus::Ready) {
        std::lock_guard<std::mutex> lock(pImpl->engineMutex);
        ma_uint64 frame = static_cast<ma_uint64>(targetTimeSeconds * track->sampleRate);
        ma_sound_seek_to_pcm_frame(&track->sound, frame);
    }
}

void AudioEngine::setMasterVolume(float volume) {
    std::lock_guard<std::mutex> lock(pImpl->engineMutex);
    pImpl->masterVolume = std::clamp(volume, 0.0f, 1.0f);
    ma_engine_set_volume(&pImpl->engine, pImpl->masterVolume);
}

float AudioEngine::getMasterVolume() const {
    return pImpl->masterVolume;
}

std::vector<float> AudioEngine::getVisualizerSpectrum(int numBars) {
    if (numBars <= 0) numBars = 16;

    if (pImpl->smoothedBars.size() != static_cast<size_t>(numBars)) {
        pImpl->smoothedBars.assign(numBars, 0.02f);
    }

    if (!isAnyPlaying() || !pImpl->fftConfig) {
        for (auto& bar : pImpl->smoothedBars) {
            bar = std::max(0.02f, bar * 0.82f);
        }
        return pImpl->smoothedBars;
    }

    float inSamples[FFT_SIZE];
    {
        std::lock_guard<std::mutex> lock(pImpl->ringMutex);
        int readIdx = (pImpl->ringWriteIndex - FFT_SIZE + RING_BUFFER_SIZE) % RING_BUFFER_SIZE;
        for (int i = 0; i < FFT_SIZE; ++i) {
            float hann = 0.5f * (1.0f - std::cos(2.0f * 3.14159265f * i / (FFT_SIZE - 1)));
            inSamples[i] = pImpl->ringBuffer[(readIdx + i) % RING_BUFFER_SIZE] * hann;
        }
    }

    kiss_fft_cpx outBins[FFT_SIZE / 2 + 1];
    kiss_fftr(pImpl->fftConfig, inSamples, outBins);

    std::vector<float> rawBars(numBars, 0.0f);
    const float f_min = 35.0f;
    const float f_max = 18000.0f;
    const float hzPerBin = 44100.0f / FFT_SIZE;

    for (int b = 0; b < numBars; ++b) {
        float f_start = f_min * std::pow(f_max / f_min, static_cast<float>(b) / numBars);
        float f_end   = f_min * std::pow(f_max / f_min, static_cast<float>(b + 1) / numBars);

        int startBin = std::clamp(static_cast<int>(f_start / hzPerBin), 1, FFT_SIZE / 2);
        int endBin   = std::clamp(static_cast<int>(f_end / hzPerBin), startBin, FFT_SIZE / 2);

        float sumMag = 0.0f;
        float peakMag = 0.0f;
        int count = 0;

        for (int k = startBin; k <= endBin; ++k) {
            float mag = std::sqrt(outBins[k].r * outBins[k].r + outBins[k].i * outBins[k].i) / (FFT_SIZE / 4.0f);
            sumMag += mag;
            if (mag > peakMag) peakMag = mag;
            count++;
        }

        float avgMag = (count > 0) ? (sumMag / count) : 0.0f;
        float finalMag = 0.65f * avgMag + 0.35f * peakMag;

        float db = 20.0f * std::log10(std::max(finalMag, 1e-4f));
        float normalized = (db + 45.0f) / 45.0f;
        rawBars[b] = std::clamp(normalized, 0.02f, 1.0f);
    }

    for (int b = 0; b < numBars; ++b) {
        if (rawBars[b] > pImpl->smoothedBars[b]) {
            pImpl->smoothedBars[b] = pImpl->smoothedBars[b] * 0.25f + rawBars[b] * 0.75f;
        } else {
            pImpl->smoothedBars[b] = pImpl->smoothedBars[b] * 0.80f + rawBars[b] * 0.20f;
        }
        pImpl->smoothedBars[b] = std::clamp(pImpl->smoothedBars[b], 0.02f, 1.0f);
    }

    return pImpl->smoothedBars;
}