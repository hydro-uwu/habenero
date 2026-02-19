#pragma once

struct lua_State;

namespace Hotones::Scripting::LuaLoader {

// Register the `input` table and key/mouse constants into the Lua state.
//
// -- Keyboard
// input.isKeyDown(key)      -> bool   -- held this frame
// input.isKeyPressed(key)   -> bool   -- first frame down
// input.isKeyReleased(key)  -> bool   -- first frame up
//
// -- Mouse
// input.isMouseDown(btn)    -> bool   -- held (0=left, 1=right, 2=middle)
// input.isMousePressed(btn) -> bool   -- first frame down
// input.getMousePos()       -> x, y   -- screen position
// input.getMouseDelta()     -> dx, dy -- delta since last frame
// input.getMouseWheel()     -> float  -- wheel move this frame
//
// -- Key constants (integers, pass to isKeyDown etc.)
// input.KEY_A .. input.KEY_Z
// input.KEY_0 .. input.KEY_9
// input.KEY_F1 .. input.KEY_F12
// input.KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT
// input.KEY_SPACE, KEY_ENTER, KEY_ESCAPE, KEY_TAB
// input.KEY_LSHIFT, KEY_RSHIFT, KEY_LCTRL, KEY_RCTRL
// input.KEY_LALT, KEY_RALT, KEY_BACKSPACE, KEY_DELETE
//
// -- Mouse button constants
// input.MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE
void registerInput(lua_State* L);

} // namespace Hotones::Scripting::LuaLoader
