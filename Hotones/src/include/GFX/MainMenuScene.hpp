#pragma once

#include <GFX/Scene.hpp>
#include <raylib.h>
#include <string>
#include <cstdint>

namespace Hotones {

class MainMenuScene : public Scene {
public:
    enum class Action { None, Host, Join, Quit };

    MainMenuScene() = default;
    ~MainMenuScene() override = default;

    void Init()   override;
    void Update() override;
    void Draw()   override;
    void Unload() override;

    // Read by main.cpp after IsFinished()
    Action      GetAction()      const { return m_action; }
    std::string GetPlayerName()  const { return m_nameBuffer; }
    std::string GetConnectHost() const { return m_ipBuffer; }
    uint16_t    GetConnectPort() const { return m_port; }

private:
    enum class State { Main, Join, Host };
    State  m_state        = State::Main;
    Action m_action       = Action::None;
    int    m_activeField  = -1;  // index of focused text field, -1 = none

    char     m_ipBuffer[64]  = "127.0.0.1";
    char     m_nameBuffer[16] = "Player";
    char     m_portBuffer[8]  = "27015";
    uint16_t m_port           = 27015;

    // Sub-screen draws (called from Draw())
    void DrawMain();
    void DrawJoin();
    void DrawHost();

    // Returns true on click
    static bool Button(const char* text, Rectangle rect);
    // Draws a labelled text field; clicking it sets focus
    static void TextField(const char* label, char* buf, int maxLen,
                          Rectangle rect, bool active);
};

} // namespace Hotones
