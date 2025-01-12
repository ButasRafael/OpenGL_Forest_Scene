#ifndef Camera_hpp
#define Camera_hpp

#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"

namespace gps {

    enum MOVE_DIRECTION { MOVE_FORWARD, MOVE_BACKWARD, MOVE_RIGHT, MOVE_LEFT, MOVE_UP, MOVE_DOWN };

    class Camera {

    public:
        Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp);

        glm::mat4 getViewMatrix();
        void move(MOVE_DIRECTION direction, float speed);
        void rotate(float pitch, float yaw);

        void setMouseSensitivity(float sensitivity) { this->mouseSensitivity = sensitivity; }

        void updateMousePosition(float xpos, float ypos);

        void setPosition(glm::vec3 newPosition);
		void setPitch(float newPitch);
        void setTarget(glm::vec3 newTarget);
        void setUpDirection(glm::vec3 newUp);

        void invertPitch();

        glm::vec3 getPosition() const;
        glm::vec3 getFront() const;
        glm::vec3 getRight() const;
        glm::vec3 getUp() const;
		glm::vec3 getTarget() const;
		glm::vec3 getUpDirection() const;
		float getPitch() const;
		float getYaw() const;



        void resetMouse();
        void setLastMousePosition(float x, float y);

        void zoom(float yoffset);
        float getFov() const { return fov; } 

    private:
        glm::vec3 cameraPosition;
        glm::vec3 cameraTarget;
        glm::vec3 cameraFrontDirection;
        glm::vec3 cameraRightDirection;
        glm::vec3 cameraUpDirection;

        float yaw;
        float pitch;
        float mouseSensitivity;
        float lastX, lastY;
        bool firstMouse;

        float fov;
        void updateCameraVectors();
    };
}

#endif
