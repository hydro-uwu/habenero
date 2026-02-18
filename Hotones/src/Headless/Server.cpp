#include <server/Server.hpp>
#include <server/NetworkManager.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

namespace {
    std::atomic<bool> g_serverRunning{ true };
}

static void SignalHandler(int /*sig*/) {
    g_serverRunning = false;
}

namespace Hotones {

void RunHeadlessServer(uint16_t port) {
    std::signal(SIGINT,  SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    Net::NetworkManager server;

    server.OnPlayerJoined = [](uint8_t id, const char* name) {
        std::cout << "[Server] ++ Player " << static_cast<int>(id)
                  << " \"" << name << "\" joined\n";
    };
    server.OnPlayerLeft = [](uint8_t id) {
        std::cout << "[Server] -- Player " << static_cast<int>(id) << " left\n";
    };

    if (!server.StartServer(port)) {
        std::cerr << "[Server] Failed to start on port " << port << "\n";
        return;
    }

    std::cout << "[Server] Dedicated server running on UDP port " << port << "\n";
    std::cout << "[Server] Press Ctrl+C to shut down.\n";

    while (g_serverRunning.load()) {
        server.Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "\n[Server] Shutting down...\n";
    server.StopServer();
    std::cout << "[Server] Goodbye!\n";
}

} // namespace Hotones