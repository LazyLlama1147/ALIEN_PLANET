#ifndef CAMERA_MODULE_H
#define CAMERA_MODULE_H

#include "glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

// Directional commands for camera control
enum MovementDirection {
    MOVE_FORWARD,
    MOVE_BACKWARD,
    MOVE_LEFT,
    MOVE_RIGHT
};

// Camera defaults
const float DEFAULT_YAW = -90.0f;
const float DEFAULT_PITCH = 0.0f;
const float DEFAULT_SPEED = 32.0f;
const float DEFAULT_SENSITIVITY = 0.05f;
const float DEFAULT_ZOOM = 45.0f;


class CameraController {
public:
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;
    float moveSpeed;
    float sensitivity;
    float zoomLevel;
    float zoom = 45.0f;

    CameraController(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 upDir = glm::vec3(0.0f, 1.0f, 0.0f), float y = DEFAULT_YAW, float p = DEFAULT_PITCH)
        : direction(glm::vec3(0.0f, 0.0f, -1.0f)), moveSpeed(DEFAULT_SPEED), sensitivity(DEFAULT_SENSITIVITY), zoomLevel(DEFAULT_ZOOM) {
        position = pos;
        worldUp = upDir;
        yaw = y;
        pitch = p;
        updateVectors();
    }

    CameraController(float px, float py, float pz, float ux, float uy, float uz, float y, float p)
        : direction(glm::vec3(0.0f, 0.0f, -1.0f)), moveSpeed(DEFAULT_SPEED), sensitivity(DEFAULT_SENSITIVITY), zoomLevel(DEFAULT_ZOOM) {
        position = glm::vec3(px, py, pz);
        worldUp = glm::vec3(ux, uy, uz);
        yaw = y;
        pitch = p;
        updateVectors();
    }

    glm::mat4 getViewMatrix() {
        return glm::lookAt(position, position + direction, up);
    }

    void handleKeyboard(MovementDirection dir, float deltaTime) {
        float velocity = moveSpeed * deltaTime;
        if (dir == MOVE_FORWARD) position += direction * velocity;
        if (dir == MOVE_BACKWARD) position -= direction * velocity;
        if (dir == MOVE_LEFT) position -= right * velocity;
        if (dir == MOVE_RIGHT) position += right * velocity;
    }

    void handleMouse(float xOffset, float yOffset, GLboolean limitPitch = true) {
        xOffset *= sensitivity;
        yOffset *= sensitivity;

        yaw += xOffset;
        pitch += yOffset;

        if (limitPitch) {
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;
        }

        updateVectors();
    }

    void handleMouseScroll(float yoffset) {
    zoom -= yoffset;
        if (zoom < 1.0f) zoom = 1.0f;
        if (zoom > 45.0f) zoom = 45.0f;
    }

private:
    void updateVectors() {
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction = glm::normalize(newFront);
        right = glm::normalize(glm::cross(direction, worldUp));
        up = glm::normalize(glm::cross(right, direction));
    }
};

#endif
