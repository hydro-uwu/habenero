#include <Player.hpp>
#include <iostream>
#include <cmath>

namespace Hotones {

Player::Player() {
    body = { 0 };
    lookRotation = { 0 };
    sensitivity = { 0.001f, 0.001f };
    headTimer = 0.0f;
    walkLerp = 0.0f;
    headLerp = STAND_HEIGHT;
    lean = { 0 };
    m_attachedCamera = nullptr;
}

void Player::AttachCamera(Camera* camera) {
    m_attachedCamera = camera;
}

void Player::Update() {
    if (!m_attachedCamera) return;

    Vector2 mouseDelta = GetMouseDelta();
    lookRotation.x -= mouseDelta.x * sensitivity.x;
    lookRotation.y += mouseDelta.y * sensitivity.y;

    char sideway = (char)(IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
    char forward = (char)(IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
    bool crouching = IsKeyDown(KEY_LEFT_CONTROL);
    bool jumpPressed = IsKeyPressed(KEY_SPACE);

    UpdateBody(sideway, forward, jumpPressed, crouching);

    float delta = GetFrameTime();
    headLerp = Lerp(headLerp, (crouching ? CROUCH_HEIGHT : STAND_HEIGHT), 20.0f * delta);
    
    m_attachedCamera->position = (Vector3){
        body.position.x,
        body.position.y + (BOTTOM_HEIGHT + headLerp),
        body.position.z,
    };

    if (body.isGrounded && ((forward != 0) || (sideway != 0))) {
        headTimer += delta * 3.0f;
        walkLerp = Lerp(walkLerp, 1.0f, 10.0f * delta);
        m_attachedCamera->fovy = Lerp(m_attachedCamera->fovy, 55.0f, 5.0f * delta);
    } else {
        walkLerp = Lerp(walkLerp, 0.0f, 10.0f * delta);
        m_attachedCamera->fovy = Lerp(m_attachedCamera->fovy, 60.0f, 5.0f * delta);
    }

    lean.x = Lerp(lean.x, (float)sideway * 0.02f, 10.0f * delta);
    lean.y = Lerp(lean.y, (float)forward * 0.015f, 10.0f * delta);

    UpdateCamera();
}

void Player::UpdateBody(char side, char forward, bool jumpPressed, bool crouchHold) {
    Vector2 input = (Vector2){ (float)side, (float)-forward };

    float delta = GetFrameTime();

    if (!body.isGrounded) body.velocity.y -= GRAVITY * delta;

    if (body.isGrounded && jumpPressed) {
        body.velocity.y = JUMP_FORCE;
        body.isGrounded = false;
    }

    Vector3 front = (Vector3){ sinf(lookRotation.x), 0.f, cosf(lookRotation.x) };
    Vector3 right = (Vector3){ cosf(-lookRotation.x), 0.f, sinf(-lookRotation.x) };

    Vector3 desiredDir = (Vector3){ input.x * right.x + input.y * front.x, 0.0f, input.x * right.z + input.y * front.z, };
    body.dir = Vector3Lerp(body.dir, desiredDir, CONTROL * delta);

    float decel = (body.isGrounded ? FRICTION : AIR_DRAG);
    Vector3 hvel = (Vector3){ body.velocity.x * decel, 0.0f, body.velocity.z * decel };

    float hvelLength = Vector3Length(hvel);
    if (hvelLength < (MAX_SPEED * 0.01f)) hvel = (Vector3){ 0 };

    float speed = Vector3DotProduct(hvel, body.dir);
    float maxSpeed = (crouchHold ? CROUCH_SPEED : MAX_SPEED);
    float accel = Clamp(maxSpeed - speed, 0.f, MAX_ACCEL * delta);
    hvel.x += body.dir.x * accel;
    hvel.z += body.dir.z * accel;

    body.velocity.x = hvel.x;
    body.velocity.z = hvel.z;

    body.position.x += body.velocity.x * delta;
    body.position.y += body.velocity.y * delta;
    body.position.z += body.velocity.z * delta;

    if (body.position.y <= 0.0f) {
        body.position.y = 0.0f;
        body.velocity.y = 0.0f;
        body.isGrounded = true;
    }
}

void Player::UpdateCamera() {
    if (!m_attachedCamera) return;

    const Vector3 up = (Vector3){ 0.0f, 1.0f, 0.0f };
    const Vector3 targetOffset = (Vector3){ 0.0f, 0.0f, -1.0f };

    Vector3 yaw = Vector3RotateByAxisAngle(targetOffset, up, lookRotation.x);

    float maxAngleUp = Vector3Angle(up, yaw);
    maxAngleUp -= 0.001f;
    if (-(lookRotation.y) > maxAngleUp) { lookRotation.y = -maxAngleUp; }

    float maxAngleDown = Vector3Angle(Vector3Negate(up), yaw);
    maxAngleDown *= -1.0f;
    maxAngleDown += 0.001f;
    if (-(lookRotation.y) < maxAngleDown) { lookRotation.y = -maxAngleDown; }

    Vector3 right = Vector3Normalize(Vector3CrossProduct(yaw, up));

    float pitchAngle = -lookRotation.y - lean.y;
    pitchAngle = Clamp(pitchAngle, -PI / 2 + 0.0001f, PI / 2 - 0.0001f);
    Vector3 pitch = Vector3RotateByAxisAngle(yaw, right, pitchAngle);

    float headSin = sinf(headTimer * PI);
    float headCos = cosf(headTimer * PI);
    const float stepRotation = 0.01f;
    m_attachedCamera->up = Vector3RotateByAxisAngle(up, pitch, headSin * stepRotation + lean.x);

    const float bobSide = 0.1f;
    const float bobUp = 0.15f;
    Vector3 bobbing = Vector3Scale(right, headSin * bobSide);
    bobbing.y = fabsf(headCos * bobUp);

    m_attachedCamera->position = Vector3Add(m_attachedCamera->position, Vector3Scale(bobbing, walkLerp));
    m_attachedCamera->target = Vector3Add(m_attachedCamera->position, pitch);
}

void Player::Render() {
    // Currently logic for drawing level is in main, but maybe player wants to draw something?
    // main.cpp doesn't have player rendering in the second half.
}

} // namespace Hotones
