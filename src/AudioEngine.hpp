#pragma once

#include <string>
#include <memory>
#include <vector>

class AudioEngine {
public:
    struct Impl;

    AudioEngine();
    ~AudioEngine();

    bool init();
    void preload(const std::string& filePath);
    void play(const std::string& filePath, float volume);
    void pause(const std::string& filePath);
    void resume(const std::string& filePath);
    void togglePause(const std::string& filePath, float volume);
    void togglePlay(const std::string& filePath, float volume);
    void stop(const std::string& filePath);
    void stopAll();
    bool isPlaying(const std::string& filePath);
    bool isPaused(const std::string& filePath);
    bool isAnyPlaying();
    bool isLoading(const std::string& filePath);
    bool hasEnded(const std::string& filePath);
    void setVolume(const std::string& filePath, float volume);

    float getCurrentTime(const std::string& filePath);
    float getTotalTime(const std::string& filePath);
    void seek(const std::string& filePath, float targetTimeSeconds);
    void setMasterVolume(float volume);
    float getMasterVolume() const;

    void setEQ(float lowDb, float midDb, float highDb, bool enabled);

    std::vector<float> getVisualizerSpectrum(int numBars = 32);

private:
    std::unique_ptr<Impl> pImpl;
};