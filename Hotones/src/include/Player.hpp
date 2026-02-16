#pragma once

#include <raylib.h>
#include <raymath.h>

namespace Hotones {

class Player {
public:
    // Movement constants
    static constexpr float GRAVITY = 32.0f;
    static constexpr float MAX_SPEED = 20.0f;
    static constexpr float CROUCH_SPEED = 5.0f;
    static constexpr float JUMP_FORCE = 12.0f;
    static constexpr float MAX_ACCEL = 150.0f;
    static constexpr float FRICTION = 0.86f;
    static constexpr float AIR_DRAG = 0.98f;
    static constexpr float CONTROL = 15.0f;
    static constexpr float CROUCH_HEIGHT = 0.0f;
    static constexpr float STAND_HEIGHT = 1.0f;
    static constexpr float BOTTOM_HEIGHT = 0.5f;

    struct Body {
        Vector3 position;
        Vector3 velocity;
        Vector3 dir;
        bool isGrounded;
    };

    Player();
    ~Player() = default;

    void Update();
    void AttachCamera(Camera* camera);
    void Render();

    // Body state
    Body body;
    Vector2 lookRotation;
    Vector2 sensitivity;
    
    // Animation/Transition state
    float headTimer;
    float walkLerp;
    float headLerp;
    Vector2 lean;

private:
    Camera* m_attachedCamera;

    void UpdateBody(char side, char forward, bool jumpPressed, bool crouchHold);
    void UpdateCamera();
};

} // namespace Hotones
