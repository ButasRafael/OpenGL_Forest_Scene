#include "Camera.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/vector_angle.hpp"

namespace gps {

    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = glm::normalize(cameraUp);

        this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraFrontDirection, this->cameraUpDirection));

        this->yaw = -90.0f;
        this->pitch = 0.0f;
        this->mouseSensitivity = 0.1f;
        this->firstMouse = true;

        this->fov = 45.0f;
    }

    
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, cameraUpDirection);
    }

    void Camera::move(MOVE_DIRECTION direction, float speed) {
        switch (direction) {
        case MOVE_FORWARD:
            cameraPosition += speed * cameraFrontDirection;
            break;
        case MOVE_BACKWARD:
            cameraPosition -= speed * cameraFrontDirection;
            break;
        case MOVE_RIGHT:
            cameraPosition += speed * cameraRightDirection;
            break;
        case MOVE_LEFT:
            cameraPosition -= speed * cameraRightDirection;
            break;
        case MOVE_UP:
            cameraPosition += speed * cameraUpDirection;
            break;
        case MOVE_DOWN:
            cameraPosition -= speed * cameraUpDirection;
            break;
        }
    }

    void Camera::rotate(float pitch, float yaw) {
        glm::vec3 newDirection;
        newDirection.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newDirection.y = sin(glm::radians(pitch));
        newDirection.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFrontDirection = glm::normalize(newDirection);

        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
    }

    void Camera::updateMousePosition(float xpos, float ypos) {
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float offsetX = xpos - lastX;
        float offsetY = lastY - ypos;
        lastX = xpos;
        lastY = ypos;

        offsetX *= mouseSensitivity;
        offsetY *= mouseSensitivity;

        yaw += offsetX;
        pitch += offsetY;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;


        rotate(pitch, yaw);
    }

    void Camera::setPosition(glm::vec3 newPosition) {
        cameraPosition = newPosition;
    }

	void Camera::setPitch(float newPitch) {
		pitch = newPitch;
	}
    glm::vec3 Camera::getPosition() const {
        return cameraPosition;
    }

    glm::vec3 Camera::getFront() const {
        return cameraFrontDirection;
    }

    glm::vec3 Camera::getRight() const {
        return cameraRightDirection;
    }

    glm::vec3 Camera::getUp() const {
        return cameraUpDirection;
    }

	float Camera::getPitch() const {
		return pitch;
	}

    float Camera::getYaw() const {
		return yaw;
    }

	glm::vec3 Camera::getTarget() const {
		return cameraTarget;
	}

	glm::vec3 Camera::getUpDirection() const {
		return cameraUpDirection;
	}

    void Camera::setTarget(glm::vec3 newTarget) {
        cameraTarget = newTarget;
        cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
    }

    void Camera::setUpDirection(glm::vec3 newUp) {
        cameraUpDirection = glm::normalize(newUp);
        updateCameraVectors();
    }

    void Camera::resetMouse() {
        firstMouse = true;
    }

    void Camera::setLastMousePosition(float x, float y) {
        lastX = x;
        lastY = y;
    }

    void Camera::zoom(float yoffset) {
        fov -= yoffset;
        if (fov < 1.0f)
            fov = 1.0f;
        if (fov > 45.0f)
            fov = 45.0f;
    }

    void Camera::invertPitch() {
        pitch = -pitch;
    }

    void Camera::updateCameraVectors() {
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
    }

}
