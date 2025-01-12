#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include "miniaudio.h"
#include <string>
#include <iostream>
#include "glm/glm.hpp"

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    bool initialize();
    void shutdown();

    bool loadRainSound(const std::string& filePath);
    void playRain(bool loop = true);
    void stopRain();
    void setRainVolume(float volume);
    void setRainPosition(const glm::vec3& position);
    void increaseRainVolume(float delta = 0.1f);
    void decreaseRainVolume(float delta = 0.1f);
    float getRainVolume() const;

    bool loadThunderSound(const std::string& filePath);
    void playThunder();
    void setThunderVolume(float volume);
    void setThunderPosition(const glm::vec3& position);
    void increaseThunderVolume(float delta = 0.1f);
    void decreaseThunderVolume(float delta = 0.1f);
    float getThunderVolume() const;

    bool loadFireSound(const std::string& filePath);
    void playFire(bool loop = true);
    void stopFire();
    void setFireVolume(float volume);
    void setFirePosition(const glm::vec3& position);
    void increaseFireVolume(float delta = 0.1f);
    void decreaseFireVolume(float delta = 0.1f);
    float getFireVolume() const;
	void updateListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);

private:
    ma_engine engine;

    ma_sound rainSound;
    bool rainLoaded;
    float rainVolume;

    ma_sound thunderSound;
    bool thunderLoaded;
    float thunderVolume;

    ma_sound fireSound;
    bool fireLoaded;
    float fireVolume;
};

#endif
