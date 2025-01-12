// Frustum.hpp

#ifndef FRUSTUM_HPP
#define FRUSTUM_HPP

#include "glm/glm.hpp"
#include <array>

namespace gps {

    struct Frustum {

        std::array<glm::vec4, 6> planes;

        void update(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
            glm::mat4 vp = projectionMatrix * viewMatrix;

            // Left Plane
            planes[0] = glm::vec4(
                vp[0][3] + vp[0][0],
                vp[1][3] + vp[1][0],
                vp[2][3] + vp[2][0],
                vp[3][3] + vp[3][0]
            );

            // Right Plane
            planes[1] = glm::vec4(
                vp[0][3] - vp[0][0],
                vp[1][3] - vp[1][0],
                vp[2][3] - vp[2][0],
                vp[3][3] - vp[3][0]
            );

            // Bottom Plane
            planes[2] = glm::vec4(
                vp[0][3] + vp[0][1],
                vp[1][3] + vp[1][1],
                vp[2][3] + vp[2][1],
                vp[3][3] + vp[3][1]
            );

            // Top Plane
            planes[3] = glm::vec4(
                vp[0][3] - vp[0][1],
                vp[1][3] - vp[1][1],
                vp[2][3] - vp[2][1],
                vp[3][3] - vp[3][1]
            );

            // Near Plane
            planes[4] = glm::vec4(
                vp[0][3] + vp[0][2],
                vp[1][3] + vp[1][2],
                vp[2][3] + vp[2][2],
                vp[3][3] + vp[3][2]
            );

            // Far Plane
            planes[5] = glm::vec4(
                vp[0][3] - vp[0][2],
                vp[1][3] - vp[1][2],
                vp[2][3] - vp[2][2],
                vp[3][3] - vp[3][2]
            );

            // Normalize the planes
            for (auto& plane : planes) {
                float length = glm::length(glm::vec3(plane));
                plane /= length;
            }
        }

        // Check if an AABB is visible within the frustum
        bool isVisible(const glm::vec3& min, const glm::vec3& max) const {
            for (const auto& plane : planes) {
                glm::vec3 positive = min;

                if (plane.x >= 0) positive.x = max.x;
                if (plane.y >= 0) positive.y = max.y;
                if (plane.z >= 0) positive.z = max.z;

                if (glm::dot(glm::vec3(plane), positive) + plane.w < 0)
                    return false;
            }
            return true;
        }
    };

}

#endif // FRUSTUM_HPP
