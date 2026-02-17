    // Feature toggles
    #pragma once
    #ifndef PLAYER_HPP
    #define PLAYER_HPP
    
    #include <raylib.h>

    #include <raymath.h>
    #include <SoundBus.hpp>
    #include <memory>
    // Forward-declare the global SoundBus accessor from the audio system
    namespace Ho_tones { class SoundBus; SoundBus& GetSoundBus(); }
    
    namespace Hotones {
        
        class Player {
            public:
            // Movement constants
            bool enableSourceBhop = false; // If true, allows Source-style bhop bug
    static constexpr float GRAVITY = 32.0f;
    static constexpr float MAX_SPEED = 200.0f;
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
    void AttachCamera(Camera3D* camera);
    // Attach the world model for collision checks
    void AttachWorld(std::shared_ptr<class CollidableModel> world);
    void Render();

    // Enable or disable Source bhop bug
    void SetSourceBhopEnabled(bool enabled) { enableSourceBhop = enabled; }
    bool IsSourceBhopEnabled() const { return enableSourceBhop; }

    void RegisterSounds() {
        Ho_tones::GetSoundBus().LoadSoundFile("footstep", "assets/sounds/footsteps/hardboot_generic1.wav");
        Ho_tones::GetSoundBus().LoadSoundFile("footstep", "assets/sounds/footsteps/hardboot_generic2.wav");
        Ho_tones::GetSoundBus().LoadSoundFile("footstep", "assets/sounds/footsteps/hardboot_generic3.wav");
        Ho_tones::GetSoundBus().LoadSoundFile("footstep", "assets/sounds/footsteps/hardboot_generic4.wav");
        Ho_tones::GetSoundBus().LoadSoundFile("footstep", "assets/sounds/footsteps/hardboot_generic5.wav");
        Ho_tones::GetSoundBus().LoadSoundFile("footstep", "assets/sounds/footsteps/hardboot_generic6.wav");
        Ho_tones::GetSoundBus().LoadSoundFile("footstep", "assets/sounds/footsteps/hardboot_generic7.wav");
        Ho_tones::GetSoundBus().LoadSoundFile("footstep", "assets/sounds/footsteps/hardboot_generic8.wav");
        Ho_tones::GetSoundBus().LoadSoundFile("footstep", "assets/sounds/footsteps/hardboot_generic9.wav");
    }
    // Body state
    Body body;
    Vector2 lookRotation;
    Vector2 sensitivity;
    
    // Animation/Transition state
    float headTimer;
    float walkLerp;
    float headLerp;
    Vector2 lean;
    // previous head sine used to trigger footstep events
    float prevHeadSin = 0.0f;

private:
    Camera3D* m_attachedCamera;
    // Shared pointer to world model for collision resolution
    std::shared_ptr<class CollidableModel> m_worldModel = nullptr;

    void UpdateBody(char side, char forward, bool jumpPressed, bool crouchHold);
    void UpdateCamera();
};

} // namespace Hotones
#endif // PLAYER_HPP
