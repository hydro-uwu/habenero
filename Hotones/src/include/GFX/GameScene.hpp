#pragma once

#include <GFX/Scene.hpp>
#include <GFX/Player.hpp>
#include <memory>
#include <raylib.h>
#include <GFX/CollidableModel.hpp>

// Forward-declare to avoid pulling socket headers into every TU
namespace Hotones::Net { class NetworkManager; }

namespace Hotones {

class GameScene : public Scene {
public:
    GameScene();
    ~GameScene() override;

    void Init() override;
    void Update() override;
    void Draw() override;
    void Unload() override;

    // Expose player access for debug UI
    Player* GetPlayer() { return &player; }
    void SetWorldDebug(bool enabled) { worldDebug = enabled; if (worldModel) worldModel->SetDebug(worldDebug); }
    bool IsWorldDebug() const { return worldDebug; }

    // Pass a non-owning pointer to the active NetworkManager so remote
    // players are rendered inside the scene's existing 3-D pass.
    void SetNetworkManager(Net::NetworkManager* nm) { m_netMgr = nm; }

private:
    Hotones::Player player;
    Camera camera;
    // Main world model
    std::shared_ptr<CollidableModel> worldModel;
    bool worldDebug = false;
    Net::NetworkManager* m_netMgr = nullptr;

    void DrawLevel();
};

} // namespace Hotones
