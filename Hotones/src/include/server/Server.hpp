#pragma once
#include <cstdint>
#include <string>

namespace Hotones {

// Run a headless (no-window) dedicated game server.
// Blocks until SIGINT / SIGTERM is received.
//
// port    – UDP port to listen on (default 27015)
// pakPath – path to a .cup archive or an extracted directory; if non-empty
//           the pack's Lua :Update() is called every server tick.
void RunHeadlessServer(uint16_t           port    = 27015,
                       const std::string& pakPath = {});

} // namespace Hotones
