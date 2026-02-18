#pragma once
// NOTE: No platform socket headers here — they live exclusively in
// NetworkManager.cpp to avoid Windows.h / raylib symbol clashes.

#include <server/Packets.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace Hotones::Net {

static constexpr uint16_t DEFAULT_PORT = 27015;
static constexpr uint8_t  MAX_PLAYERS  = 16;

// ─── Snapshot of a remote player (updated each received PlayerUpdatePacket) ──
struct RemotePlayer {
    uint8_t id     = 0;
    char    name[16] = {};
    float   posX = 0.f, posY = 0.f, posZ = 0.f;
    float   rotX = 0.f, rotY = 0.f; // yaw, pitch
    bool    active = false;
};

// ─── NetworkManager ───────────────────────────────────────────────────────────
//
//  Handles both server and client roles over UDP.
//
//  Threading model:
//   – RecvLoop() runs on a background thread and pushes raw datagrams into
//     m_recvQueue (mutex-protected).
//   – Update() is called once per game frame (main thread) and drains the
//     queue, dispatching packets and invoking callbacks safely.
//
class NetworkManager {
public:
    enum class Mode { None, Server, Client };

    NetworkManager();
    ~NetworkManager();

    // ── Server API ────────────────────────────────────────────────────────────
    bool StartServer(uint16_t port = DEFAULT_PORT);
    void StopServer();
    bool IsServerRunning() const;

    // ── Client API ────────────────────────────────────────────────────────────
    bool Connect(const std::string& host, uint16_t port = DEFAULT_PORT,
                 const std::string& playerName = "Player");
    void Disconnect();
    bool IsConnected() const;

    // Send local player position/rotation to the server (~20 Hz recommended)
    void SendPlayerUpdate(float px, float py, float pz, float rotX, float rotY);

    // ── Shared API ────────────────────────────────────────────────────────────
    void    Update();  // Must be called once per game frame from the main thread
    Mode    GetMode()    const;
    uint8_t GetLocalId() const;
    const std::unordered_map<uint8_t, RemotePlayer>& GetRemotePlayers() const;

    // Callbacks – invoked from Update() on the main thread
    std::function<void(uint8_t id, const char* name)> OnPlayerJoined;
    std::function<void(uint8_t id)>                    OnPlayerLeft;

private:
    // All socket/platform types are hidden here; defined only in NetworkManager.cpp
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Hotones::Net

