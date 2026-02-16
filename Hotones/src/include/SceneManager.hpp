
#pragma once

#include "Scene.hpp"
#include "../GFX/TransitionScene.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>

namespace Hotones {

// SceneManager now stores factories for scenes and supports an optional scene stack.
class SceneManager {
public:
    using SceneFactory = std::function<std::unique_ptr<Scene>()>;

    // Register a scene factory under a name
    void Add(const std::string &name, SceneFactory factory) {
        factories[name] = std::move(factory);
    }

    // Switch clears the stack and replaces with a fresh instance of the named scene
    void SwitchTo(const std::string &name) {
        auto it = factories.find(name);
        if (it == factories.end()) return;
        // Unload and clear any stacked scenes
        if (!stack.empty()) {
            stack.back()->Unload();
            stack.clear();
        }
        // create and push new
        auto inst = it->second();
        inst->Init();
        currentName = name;
        stack.push_back(std::move(inst));
    }

    // Switch with transition: capture outgoing snapshot and push a TransitionScene that will
    // render the animated transition and instantiate the incoming scene mid-way.
    void SwitchWithTransition(const std::string &name, float duration = 1.0f) {
        auto it = factories.find(name);
        if (it == factories.end()) return;

        // Capture current top scene into a render texture
        RenderTexture2D outTex = {0};
        if (!stack.empty()) {
            outTex = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
            BeginTextureMode(outTex);
            ClearBackground(BLACK);
            stack.back()->Draw();
            EndTextureMode();
        } else {
            // create a blank texture
            outTex = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
            BeginTextureMode(outTex);
            ClearBackground(BLACK);
            EndTextureMode();
        }

        // Clear existing stack but keep outTex for transition to draw
        if (!stack.empty()) {
            stack.back()->Unload();
            stack.clear();
        }

            // create incoming scene now so it can initialize / generate while the transition runs
            auto incoming = it->second();
            if (incoming) {
                incoming->Init();
            }

            // create transition scene and push it, passing ownership of the incoming instance
            auto transition = std::make_unique<TransitionScene>(outTex, std::move(incoming), duration);
            transition->Init();
            stack.push_back(std::move(transition));
        // set pending target so we can assign the name when the incoming scene is installed
        pendingTargetName = name;
        // mark currentName as transition
        currentName = "<transition>";
    }

    // Push a new instance of a named scene on top of the stack (optional stack behavior)
    void Push(const std::string &name) {
        auto it = factories.find(name);
        if (it == factories.end()) return;
        auto inst = it->second();
        inst->Init();
        stack.push_back(std::move(inst));
        currentName = name;
    }

    // Pop the top scene; if any remaining, it resumes as current
    void Pop() {
        if (stack.empty()) return;
        stack.back()->Unload();
        stack.pop_back();
        if (!stack.empty()) {
            // currentName becomes the factory key of the resumed scene if known
            // We don't track reverse mapping here; leave currentName unchanged or empty
            // Caller can query GetCurrentName if they track it externally
        } else {
            currentName.clear();
        }
    }

    // Update top scene
    void Update() {
        if (!stack.empty()) {
            stack.back()->Update();

            // If the top scene finished, handle it here
            if (stack.back()->IsFinished()) {
                // If it is a TransitionScene, extract the incoming instance and push it
                TransitionScene *ts = dynamic_cast<TransitionScene*>(stack.back().get());
                if (ts) {
                    // take ownership of incoming
                    std::unique_ptr<Scene> incoming = ts->ReleaseIncoming();
                    // unload and pop transition
                    stack.back()->Unload();
                    stack.pop_back();

                    if (incoming) {
                        // push the incoming scene as the active scene
                        stack.push_back(std::move(incoming));
                        // set currentName to the pending target if available
                        if (!pendingTargetName.empty()) {
                            currentName = pendingTargetName;
                            pendingTargetName.clear();
                        }
                    }
                } else {
                    // Non-transition finished scene: leave it on the stack
                    // so caller can decide what to do (e.g., trigger a transition).
                }
            }
        }
    }

    // Draw top scene
    void Draw() {
        if (!stack.empty()) stack.back()->Draw();
    }

    Scene *GetCurrent() { return stack.empty() ? nullptr : stack.back().get(); }
    const std::string &GetCurrentName() const { return currentName; }

private:
    std::unordered_map<std::string, SceneFactory> factories;
    std::vector<std::unique_ptr<Scene>> stack;
    std::string currentName;
    // Used when a SwitchWithTransition is initiated to remember the incoming target name
    std::string pendingTargetName;
};

} // namespace Hotones
