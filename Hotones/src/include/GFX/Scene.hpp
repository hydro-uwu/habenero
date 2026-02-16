#pragma once

#include <raylib.h>
#include <memory>

namespace Hotones {

class Scene {
public:
    virtual ~Scene() = default;
    virtual void Init() {}
    virtual void Update() = 0; // update logic (called before Draw)
    virtual void Draw() = 0;   // draw commands (called between BeginDrawing/EndDrawing)
    virtual void Unload() {}

    bool IsFinished() const { return finished; }
protected:
    void MarkFinished() { finished = true; }

private:
    bool finished = false;
};

} // namespace Hotones
