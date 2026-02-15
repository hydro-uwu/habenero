#pragma once

#include <raylib.h>
#include <Hotones.hpp>
#include <cmath>

namespace Hotones {

class Player {
public:
    Vector3 position;
    float yaw;            // degrees
    float pitch;          // degrees
    float speed;          // units per second
    float mouseSensitivity;
    float radius;

    Player()
        : position{0.0f, 0.5f, 0.0f}, yaw(0.0f), pitch(0.0f), speed(4.0f), mouseSensitivity(0.15f), radius(0.35f) {}

    ~Player() = default;

    void update() {
        float dt = GetFrameTime();

        // Mouse look (hold right mouse button)
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 md = GetMouseDelta();
            yaw += md.x * mouseSensitivity;
            pitch -= md.y * mouseSensitivity;
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;
        }

        // Movement relative to yaw
        float yawRad = yaw * DEG2RAD;
        Vector3 forward = { sinf(yawRad), 0.0f, cosf(yawRad) };
        Vector3 right = { cosf(yawRad), 0.0f, -sinf(yawRad) };

        Vector3 move = { 0.0f, 0.0f, 0.0f };
        if (IsKeyDown(KEY_W)) { move.x += forward.x; move.z += forward.z; }
        if (IsKeyDown(KEY_S)) { move.x -= forward.x; move.z -= forward.z; }
        if (IsKeyDown(KEY_A)) { move.x -= right.x;   move.z -= right.z; }
        if (IsKeyDown(KEY_D)) { move.x += right.x;   move.z += right.z; }

        float currSpeed = speed * (IsKeyDown(KEY_LEFT_SHIFT) ? 2.0f : 1.0f);
        float len = sqrtf(move.x*move.x + move.z*move.z);
        if (len > 0.0001f) {
            move.x = (move.x / len) * currSpeed * dt;
            move.z = (move.z / len) * currSpeed * dt;
            position.x += move.x;
            position.z += move.z;
        }

        // Keep player on ground level (y)
        // position.y = 0.5f; // uncomment if fixed-ground desired

        // Update shared camera (third-person follow)
        if (Hotones::core::camera) {
            Vector3 camOffset = { 0.0f, 1.5f, -3.0f };
            float cosY = cosf(yawRad);
            float sinY = sinf(yawRad);
            Vector3 rotated = {
                camOffset.x * cosY - camOffset.z * sinY,
                camOffset.y,
                camOffset.x * sinY + camOffset.z * cosY
            };

            Hotones::core::camera->position = { position.x + rotated.x, position.y + rotated.y + 0.25f, position.z + rotated.z };
            Hotones::core::camera->target = { position.x, position.y + 0.5f, position.z };
            Hotones::core::camera->up = { 0.0f, 1.0f, 0.0f };
        }
    }

    void render() {
        // Simple visual representation
        DrawCube(position, radius * 2.0f, 1.0f, radius * 2.0f, BLUE);
        DrawCubeWires(position, radius * 2.0f, 1.0f, radius * 2.0f, DARKBLUE);

        // Direction indicator
        float yawRad = yaw * DEG2RAD;
        Vector3 dir = { sinf(yawRad), 0.0f, cosf(yawRad) };
        DrawLine3D(position, { position.x + dir.x, position.y + 0.5f, position.z + dir.z }, RED);
    }
};

} // namespace Hotones
