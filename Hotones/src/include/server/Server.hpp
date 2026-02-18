#pragma once
#include <cstdint>

namespace Hotones {

// Run a headless (no-window) dedicated game server.
// Blocks until SIGINT / SIGTERM is received.
void RunHeadlessServer(uint16_t port = 27015);

} // namespace Hotones
