#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "AudioManager.h"

#ifndef MA_SOUND_FLAG_NONE
#define MA_SOUND_FLAG_NONE 0
#endif

AudioManager::AudioManager()
    : rainLoaded(false), rainVolume(0.5f),
    thunderLoaded(false), thunderVolume(0.5f),
    fireLoaded(false), fireVolume(0.5f) {}

AudioManager::~AudioManager() {
    shutdown();
}

bool AudioManager::initialize() {
    ma_result result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize audio engine." << std::endl;
        return false;
    }
    return true;
}

void AudioManager::shutdown() {
    if (rainLoaded) {
        ma_sound_uninit(&rainSound);
        rainLoaded = false;
    }
    if (thunderLoaded) {
        ma_sound_uninit(&thunderSound);
        thunderLoaded = false;
    }
    if (fireLoaded) {
        ma_sound_uninit(&fireSound);
        fireLoaded = false;
    }
    ma_engine_uninit(&engine);
}

bool AudioManager::loadRainSound(const std::string& filePath) {
    ma_result result = ma_sound_init_from_file(&engine, filePath.c_str(), MA_SOUND_FLAG_STREAM, NULL, NULL, &rainSound);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to load rain sound: " << filePath << std::endl;
        return false;
    }

    ma_sound_set_spatialization_enabled(&rainSound, MA_FALSE);

    rainLoaded = true;
    setRainVolume(rainVolume);
    return true;
}

void AudioManager::playRain(bool loop) {
    if (!rainLoaded) {
        std::cerr << "Rain sound not loaded." << std::endl;
        return;
    }
    ma_sound_set_looping(&rainSound, loop ? MA_TRUE : MA_FALSE);
    ma_result result = ma_sound_start(&rainSound);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to play rain sound." << std::endl;
    }
}

void AudioManager::stopRain() {
    if (!rainLoaded) {
        return;
    }
    ma_sound_stop(&rainSound);
}

void AudioManager::setRainVolume(float volume) {
    if (rainLoaded) {
        rainVolume = glm::clamp(volume, 0.0f, 1.0f);
        ma_sound_set_volume(&rainSound, rainVolume);
    }
}

void AudioManager::setRainPosition(const glm::vec3& position) {
    if (rainLoaded) {
        ma_sound_set_position(&rainSound, position.x, position.y, position.z);
    }
}

void AudioManager::increaseRainVolume(float delta) {
    setRainVolume(rainVolume + delta);
    std::cout << "Rain Volume: " << rainVolume << std::endl;
}

void AudioManager::decreaseRainVolume(float delta) {
    setRainVolume(rainVolume - delta);
    std::cout << "Rain Volume: " << rainVolume << std::endl;
}

float AudioManager::getRainVolume() const {
    return rainVolume;
}


bool AudioManager::loadThunderSound(const std::string& filePath) {
    ma_result result = ma_sound_init_from_file(&engine, filePath.c_str(), 0, NULL, NULL, &thunderSound);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to load thunder sound: " << filePath << std::endl;
        return false;
    }

    ma_sound_set_spatialization_enabled(&thunderSound, MA_FALSE);

    thunderLoaded = true;
    setThunderVolume(thunderVolume);
    return true;
}

void AudioManager::playThunder() {
    if (!thunderLoaded) {
        std::cerr << "Thunder sound not loaded." << std::endl;
        return;
    }
    ma_result result = ma_sound_start(&thunderSound);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to play thunder sound." << std::endl;
    }
}

void AudioManager::setThunderVolume(float volume) {
    if (thunderLoaded) {
        thunderVolume = glm::clamp(volume, 0.0f, 1.0f);
        ma_sound_set_volume(&thunderSound, thunderVolume);
    }
}

void AudioManager::setThunderPosition(const glm::vec3& position) {
    if (thunderLoaded) {
        ma_sound_set_position(&thunderSound, position.x, position.y, position.z);
    }
}

void AudioManager::increaseThunderVolume(float delta) {
    setThunderVolume(thunderVolume + delta);
    std::cout << "Thunder Volume: " << thunderVolume << std::endl;
}

void AudioManager::decreaseThunderVolume(float delta) {
    setThunderVolume(thunderVolume - delta);
    std::cout << "Thunder Volume: " << thunderVolume << std::endl;
}

float AudioManager::getThunderVolume() const {
    return thunderVolume;
}


bool AudioManager::loadFireSound(const std::string& filePath) {
    ma_result result = ma_sound_init_from_file(&engine, filePath.c_str(), MA_SOUND_FLAG_STREAM, NULL, NULL, &fireSound);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to load fire sound: " << filePath << std::endl;
        return false;
    }

    ma_sound_set_spatialization_enabled(&fireSound, MA_TRUE);


    fireLoaded = true;
    setFireVolume(fireVolume);
    return true;
}

void AudioManager::playFire(bool loop) {
    if (!fireLoaded) {
        std::cerr << "Fire sound not loaded." << std::endl;
        return;
    }
    ma_sound_set_looping(&fireSound, loop ? MA_TRUE : MA_FALSE);
    ma_result result = ma_sound_start(&fireSound);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to play fire sound." << std::endl;
    }
}

void AudioManager::stopFire() {
    if (!fireLoaded) {
        return;
    }
    ma_sound_stop(&fireSound);
}

void AudioManager::setFireVolume(float volume) {
    if (fireLoaded) {
        fireVolume = glm::clamp(volume, 0.0f, 1.0f);
        ma_sound_set_volume(&fireSound, fireVolume);
    }
}

void AudioManager::setFirePosition(const glm::vec3& position) {
    if (fireLoaded) {
        ma_sound_set_position(&fireSound, position.x, position.y, position.z);
    }
}

void AudioManager::increaseFireVolume(float delta) {
    setFireVolume(fireVolume + delta);
    std::cout << "Fire Volume: " << fireVolume << std::endl;
}

void AudioManager::decreaseFireVolume(float delta) {
    setFireVolume(fireVolume - delta);
    std::cout << "Fire Volume: " << fireVolume << std::endl;

}

float AudioManager::getFireVolume() const {
    return fireVolume;
}

void AudioManager::updateListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
{
    ma_engine_listener_set_position(&engine, 0, position.x, position.y, position.z);
    ma_engine_listener_set_direction(&engine, 0, forward.x, forward.y, forward.z);
    ma_engine_listener_set_world_up(&engine, 0, up.x, up.y, up.z);

    ma_sound_set_attenuation_model(&fireSound, ma_attenuation_model_exponential);

    ma_sound_set_rolloff(&fireSound, 1.0f);

    ma_sound_set_min_distance(&fireSound, 2.0f);

}

