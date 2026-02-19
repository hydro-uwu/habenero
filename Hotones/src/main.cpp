#include <raylib.h>
#include <raymath.h>
#include <GFX/Player.hpp>
#include <GFX/SceneManager.hpp>
#include <GFX/LoadingScene.hpp>
#include <GFX/SimpleScene.hpp>
#include <GFX/GameScene.hpp>
#include <GFX/ScriptedScene.hpp>
#include <GFX/MainMenuScene.hpp>
#include <imgui/imgui.h>
#include <imgui/rlImGui.h>
#include <SFX/AudioSystem.hpp>
#include <Assets/AssetLoader.hpp>
#include <server/NetworkManager.hpp>
#include <server/Server.hpp>
#include <Scripting/CupLoader.hpp>
#include <Scripting/CupPackage.hpp>
#include <Physics/PhysicsSystem.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#if defined(_WIN32)
#include <crtdbg.h>
#endif

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void DrawLevel(void);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------


int main(int argc, char** argv)
{
    // ── Command-line argument parsing ───────────────────────────────────────
    bool        isServer    = false;
    uint16_t    serverPort  = Hotones::Net::DEFAULT_PORT;
    std::string connectHost;
    uint16_t    connectPort = Hotones::Net::DEFAULT_PORT;
    std::string playerName  = "Player";
    std::string pakPath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--server") {
            isServer = true;
        } else if (arg == "--port" && i + 1 < argc) {
            serverPort = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--connect" && i + 1 < argc) {
            connectHost = argv[++i];
        } else if (arg == "--cport" && i + 1 < argc) {
            connectPort = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--name" && i + 1 < argc) {
            playerName = argv[++i];
        } else if (arg == "--pak" && i + 1 < argc) {
            pakPath = argv[++i];
        }
    }
    TraceLog(LOG_DEBUG, "CLI args: isServer=%d serverPort=%d connectHost=%s connectPort=%d playerName=%s pak=%s",
             isServer ? 1 : 0, (int)serverPort, connectHost.c_str(), (int)connectPort, playerName.c_str(), pakPath.c_str());
    SetTraceLogLevel(LOG_WARNING); // Reduce raylib log spam (can be set to LOG_INFO for more details)
    // ── Headless server mode (no window needed) ─────────────────────────────
    if (isServer) {
        Hotones::RunHeadlessServer(serverPort, pakPath);
        return 0;
    }
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1280;
    const int screenHeight = 768;

    InitWindow(screenWidth, screenHeight, "Habanero Hotel - Hotones");
    TraceLog(LOG_INFO, "Window initialized %dx%d", screenWidth, screenHeight);

    // Enable CRT debug heap checks on Windows to catch heap corruption early
#if defined(_WIN32) && defined(_DEBUG)
    int dbgFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    dbgFlags |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(dbgFlags);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
#endif
    // Initialize audio subsystem so game systems can play sounds (footsteps etc.)
    bool audioOk = Ho_tones::InitAudioSystem();
    TraceLog(audioOk ? LOG_INFO : LOG_WARNING, "Audio system %s", audioOk ? "initialized" : "failed to initialize");

    TraceLog(LOG_INFO, "Initializing physics subsystem");
    Hotones::Physics::InitPhysics();
    TraceLog(LOG_INFO, "Physics subsystem initialized");

    // ── Cup pack (client mode) ───────────────────────────────────────────────
    // Open and initialise a .cup / directory pack when --pak is given.
    // The pack's CupLoader drives the ScriptedScene each frame.
    std::unique_ptr<Hotones::Scripting::CupPackage> g_pak;
    std::unique_ptr<Hotones::Scripting::CupLoader>  g_script;
    // Async pack loader state
    std::thread              g_packThread;
    std::atomic<bool>        g_packLoaded{false};
    std::atomic<bool>        g_packFailed{false};
    std::atomic<bool>        g_pakExtracted{false};
    std::mutex               g_packErrMutex;
    std::string              g_packError;

    if (!pakPath.empty()) {
        TraceLog(LOG_INFO, "Pak requested: %s", pakPath.c_str());
        g_pak    = std::make_unique<Hotones::Scripting::CupPackage>();
        g_script = std::make_unique<Hotones::Scripting::CupLoader>();

        // Launch background thread to only open/extract the package.
        // Lua initialisation and script loading must run on the main thread
        // (Lua and raylib are not thread-safe together), so we only extract
        // the archive here and let the main loop perform `CupLoader::init()`
        // and `loadPak()` after extraction completes.
        Hotones::Scripting::CupPackage* pakPtr = g_pak.get();
        std::string packPathCopy = pakPath;
        TraceLog(LOG_INFO, "Starting background pack extraction: %s", packPathCopy.c_str());
        g_packThread = std::thread([pakPtr, packPathCopy, &g_pakExtracted, &g_packFailed, &g_packErrMutex, &g_packError]() {
            if (!pakPtr->open(packPathCopy)) {
                std::lock_guard<std::mutex> lk(g_packErrMutex);
                g_packError = "Failed to open pack: " + packPathCopy;
                g_packFailed.store(true);
                return;
            }
            g_pakExtracted.store(true);
            TraceLog(LOG_INFO, "Pack extracted: %s", packPathCopy.c_str());
        });
    }

    // Initialize player and camera
    Hotones::Player player;
    player.RegisterSounds();
    Camera camera = { 0 };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.position = (Vector3){
        player.body.position.x,
        player.body.position.y + (Hotones::Player::BOTTOM_HEIGHT + player.headLerp),
        player.body.position.z,
    };

    player.AttachCamera(&camera);
    TraceLog(LOG_DEBUG, "Player and camera initialized; camera pos=(%.2f,%.2f,%.2f)",
             camera.position.x, camera.position.y, camera.position.z);
    // Scene manager + scenes
    Hotones::SceneManager sceneMgr;
    sceneMgr.Add("menu",    [](){ return std::make_unique<Hotones::MainMenuScene>(); });
    sceneMgr.Add("loading", [](){ return std::make_unique<Hotones::LoadingScene>(); });
    sceneMgr.Add("game",    [](){ return std::make_unique<Hotones::GameScene>(); });

    // When a pack is loaded, register a ScriptedScene driven by that pack.
    if (g_script) {
        Hotones::Scripting::CupLoader* rawScript = g_script.get();
        sceneMgr.Add("scripted", [rawScript](){
            return std::make_unique<Hotones::ScriptedScene>(rawScript);
        });
        // Start at the loading screen while the pack loads in background.
        TraceLog(LOG_INFO, "Registered scripted scene; switching to loading screen");
        sceneMgr.SwitchTo("loading");
    } else {
        TraceLog(LOG_INFO, "No pack provided; switching to main menu");
        sceneMgr.SwitchTo("menu"); // normal boot
    }

    // ── Network manager ─────────────────────────────────────────────────────
    Hotones::Net::NetworkManager netMgr;
    if (!connectHost.empty()) {
        netMgr.Connect(connectHost, connectPort, playerName);
    }
    // Rate-limit player-update sends to ~20 Hz
    float netSendTimer = 0.f;
    static constexpr float NET_SEND_INTERVAL = 1.f / 20.f;

    // Cursor starts enabled (menu). GameScene::Init() calls DisableCursor().

    SetTargetFPS(60);       // Set our game to run at 60 frames-per-second
    TraceLog(LOG_DEBUG, "Target FPS set to 60");
    // Initialize rlImGui (optional system-installed integration)
    rlImGuiSetup(true);
    //--------------------------------------------------------------------------------------
    bool showDebugUI = false;
    
    TraceLog(LOG_INFO, "Entering main loop");
    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        TraceLog(LOG_DEBUG, "Main loop iteration start — frameTime=%.6f scene=%s", GetFrameTime(), sceneMgr.GetCurrentName().c_str());
        // Update
        //----------------------------------------------------------------------------------
        if (IsKeyPressed(KEY_F1)) {
            showDebugUI = !showDebugUI;
            TraceLog(LOG_DEBUG, "F1 pressed — debug UI=%d", showDebugUI ? 1 : 0);
        }

        // Only tick the standalone player while actually playing
        if (sceneMgr.GetCurrentName() == "game") {
            TraceLog(LOG_TRACE, "Player.Update() about to run");
            player.Update();
            TraceLog(LOG_TRACE, "Player.Update() finished");
        }
        TraceLog(LOG_TRACE, "SceneManager.Update() about to run (current=%s)", sceneMgr.GetCurrentName().c_str());
        sceneMgr.Update();
        TraceLog(LOG_TRACE, "SceneManager.Update() finished (current=%s)", sceneMgr.GetCurrentName().c_str());

        // ── Scene transitions ────────────────────────────────────────────────
        // Menu finished → start networking then fade to loading screen
        if (sceneMgr.GetCurrentName() == "menu" && sceneMgr.GetCurrent() && sceneMgr.GetCurrent()->IsFinished()) {
            auto* menu = dynamic_cast<Hotones::MainMenuScene*>(sceneMgr.GetCurrent());
            if (menu) {
                if (menu->GetAction() == Hotones::MainMenuScene::Action::Quit) {
                    break; // exit game loop cleanly
                }
                playerName = menu->GetPlayerName();
                if (menu->GetAction() == Hotones::MainMenuScene::Action::Host) {
                    serverPort = menu->GetConnectPort();
                    TraceLog(LOG_INFO, "Starting server on port %d", serverPort);
                    netMgr.StartServer(serverPort);
                } else if (menu->GetAction() == Hotones::MainMenuScene::Action::Join) {
                    connectHost = menu->GetConnectHost();
                    connectPort = menu->GetConnectPort();
                    TraceLog(LOG_INFO, "Joining server %s:%d as %s", connectHost.c_str(), connectPort, playerName.c_str());
                    netMgr.Connect(connectHost, connectPort, playerName);
                }
                TraceLog(LOG_INFO, "Switching to loading scene");
                sceneMgr.SwitchWithTransition("loading", 1.0f);
            }
        }

        // Loading finished → enter game (or scripted scene if a pack is loaded)
        if (sceneMgr.GetCurrentName() == "loading" && sceneMgr.GetCurrent() && sceneMgr.GetCurrent()->IsFinished()) {
            // Only enter the scripted scene if the pack finished loading successfully.
            if (g_script) {
                if (g_packFailed.load()) {
                    std::lock_guard<std::mutex> lk(g_packErrMutex);
                    TraceLog(LOG_ERROR, "%s", g_packError.c_str());
                    break; // exit game loop
                }
                // If the package extraction finished but Lua hasn't been
                // initialised/loaded yet, do it now on the main thread.
                if (g_pakExtracted.load() && !g_packLoaded.load()) {
                    // Perform Lua init/load on main thread
                    if (!g_script->init() || !g_script->loadPak(*g_pak)) {
                        std::lock_guard<std::mutex> lk(g_packErrMutex);
                        g_packError = "Failed to initialise pack (main thread).";
                        g_packFailed.store(true);
                        TraceLog(LOG_ERROR, "%s", g_packError.c_str());
                        break;
                    }
                    g_packLoaded.store(true);
                    TraceLog(LOG_INFO, "Pack initialised on main thread and marked loaded");
                }
                    if (!g_packLoaded.load()) {
                        // still extracting/initialising; keep showing loading screen
                        TraceLog(LOG_DEBUG, "Pack still loading/extracting");
                    } else {
                        TraceLog(LOG_INFO, "Switching to scripted scene");
                        sceneMgr.SwitchWithTransition("scripted", 1.0f);
                    }
            } else {
                    TraceLog(LOG_INFO, "Switching to game scene");
                    sceneMgr.SwitchWithTransition("game", 1.0f);
            }
        }

        // ── Network tick ────────────────────────────────────────────────────
        TraceLog(LOG_TRACE, "Network.Update() about to run");
        netMgr.Update();
        TraceLog(LOG_TRACE, "Network.Update() finished");
        netSendTimer += GetFrameTime();
        bool netActive = netMgr.IsConnected()
                       || netMgr.GetMode() == Hotones::Net::NetworkManager::Mode::Server;
        if (netActive && netSendTimer >= NET_SEND_INTERVAL) {
            netSendTimer = 0.f;
            Hotones::Player* p = nullptr;
            if (auto* gs = dynamic_cast<Hotones::GameScene*>(sceneMgr.GetCurrent()))
                p = gs->GetPlayer();
            else if (auto* ss = dynamic_cast<Hotones::ScriptedScene*>(sceneMgr.GetCurrent()))
                p = ss->GetPlayer();
            if (p) {
                netMgr.SendPlayerUpdate(
                    p->body.position.x, p->body.position.y, p->body.position.z,
                    p->lookRotation.x,  p->lookRotation.y
                );
            }
        }
        // Pass NetworkManager to GameScene / ScriptedScene every frame
        {
            Hotones::GameScene* gs = dynamic_cast<Hotones::GameScene*>(sceneMgr.GetCurrent());
            if (gs) gs->SetNetworkManager(&netMgr);
            Hotones::ScriptedScene* ss = dynamic_cast<Hotones::ScriptedScene*>(sceneMgr.GetCurrent());
            if (ss) ss->SetNetworkManager(&netMgr);
        }
        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        TraceLog(LOG_TRACE, "BeginDrawing() about to run");
        BeginDrawing();

            // Let scene manager draw current scene (GameScene handles its own 3D camera)
            sceneMgr.Draw();

            // ImGui debug overlay
            if (showDebugUI) {
                rlImGuiBegin();
                ImGui::Begin("Debug (F1 to toggle)");
                ImGui::Text("Scene: %s", sceneMgr.GetCurrentName().c_str());
                // Network status
                if (netMgr.GetMode() == Hotones::Net::NetworkManager::Mode::Client) {
                    if (netMgr.IsConnected()) {
                        ImGui::Text("Net: connected  (ID %d,  remote players: %d)",
                                    netMgr.GetLocalId(),
                                    static_cast<int>(netMgr.GetRemotePlayers().size()));
                    } else {
                        ImGui::TextColored({1,1,0,1}, "Net: connecting to %s...", connectHost.c_str());
                    }
                } else {
                    ImGui::TextDisabled("Net: offline (use --connect <ip>)");
                }
                ImGui::Separator();
                Hotones::Scene *cur = sceneMgr.GetCurrent();
                if (cur) {
                    Hotones::GameScene *gs = dynamic_cast<Hotones::GameScene*>(cur);
                    if (gs) {
                        // Player info
                        Hotones::Player *p = gs->GetPlayer();
                        Vector3 pos = p->body.position;
                        Vector3 vel = p->body.velocity;
                        ImGui::Text("Player pos: %.3f, %.3f, %.3f", pos.x, pos.y, pos.z);
                        ImGui::Text("Player vel: %.3f, %.3f, %.3f", vel.x, vel.y, vel.z);
                        bool worldDbg = gs->IsWorldDebug();
                        if (ImGui::Checkbox("World debug (F2)", &worldDbg)) {
                            gs->SetWorldDebug(worldDbg);
                        }
                    }
                }
                ImGui::End();
                rlImGuiEnd();
            }

        EndDrawing();
        TraceLog(LOG_TRACE, "EndDrawing() finished");
        TraceLog(LOG_DEBUG, "Main loop iteration end — scene=%s", sceneMgr.GetCurrentName().c_str());
        //----------------------------------------------------------------------------------
    }
    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Ensure background pack loader finished before tearing down subsystems
    if (g_packThread.joinable()) {
        TraceLog(LOG_INFO, "Waiting for pack extraction thread to finish");
        g_packThread.join();
        TraceLog(LOG_INFO, "Pack extraction thread joined");
    }
    TraceLog(LOG_INFO, "Shutting down physics subsystem");
    Hotones::Physics::ShutdownPhysics();
    TraceLog(LOG_INFO, "Physics shutdown complete");
    // Shutdown audio before closing the window
    TraceLog(LOG_INFO, "Shutting down audio system");
    Ho_tones::ShutdownAudioSystem();
    TraceLog(LOG_INFO, "Audio shutdown complete");

    rlImGuiShutdown();

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

    
//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
// Draw game level
static void DrawLevel(void)
{
    const int floorExtent = 25;
    const float tileSize = 5.0f;
    const Color tileColor1 = (Color){ 150, 200, 200, 255 };

    // Floor tiles
    for (int y = -floorExtent; y < floorExtent; y++)
    {
        for (int x = -floorExtent; x < floorExtent; x++)
        {
            if ((y & 1) && (x & 1))
            {
                DrawPlane((Vector3){ x*tileSize, 0.0f, y*tileSize}, (Vector2){ tileSize, tileSize }, tileColor1);
            }
            else if (!(y & 1) && !(x & 1))
            {
                DrawPlane((Vector3){ x*tileSize, 0.0f, y*tileSize}, (Vector2){ tileSize, tileSize }, LIGHTGRAY);
            }
        }
    }

    const Vector3 towerSize = (Vector3){ 16.0f, 32.0f, 16.0f };
    const Color towerColor = (Color){ 150, 200, 200, 255 };

    Vector3 towerPos = (Vector3){ 16.0f, 16.0f, 16.0f };
    DrawCubeV(towerPos, towerSize, towerColor);
    DrawCubeWiresV(towerPos, towerSize, DARKBLUE);

    towerPos.x *= -1;
    DrawCubeV(towerPos, towerSize, towerColor);
    DrawCubeWiresV(towerPos, towerSize, DARKBLUE);

    towerPos.z *= -1;
    DrawCubeV(towerPos, towerSize, towerColor);
    DrawCubeWiresV(towerPos, towerSize, DARKBLUE);

    towerPos.x *= -1;
    DrawCubeV(towerPos, towerSize, towerColor);
    DrawCubeWiresV(towerPos, towerSize, DARKBLUE);

    // Red sun
    DrawSphere((Vector3){ 300.0f, 300.0f, 0.0f }, 100.0f, (Color){ 255, 0, 0, 255 });
}

// touch: force rebuild after TransitionScene changes
