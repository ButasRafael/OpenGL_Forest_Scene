
#ifndef WATER_TILE_HPP
#define WATER_TILE_HPP

struct WaterTile {
    static const float TILE_SIZE;
    float x;
    float z;
    float height;

    WaterTile(float centerX, float centerZ, float heightLevel)
        : x(centerX), z(centerZ), height(heightLevel) {}

    // Getters
    float getX() const {
        return x;
    }

    float getZ() const {
        return z;
    }

    float getHeight() const {
        return height;
    }
};

#endif
