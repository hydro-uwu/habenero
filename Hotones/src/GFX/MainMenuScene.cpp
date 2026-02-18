#include <GFX/MainMenuScene.hpp>
#include <cstring>
#include <cstdlib>

namespace Hotones {

// ─── Palette ──────────────────────────────────────────────────────────────────
static constexpr Color BG_DARK    = {  15,  12,  20, 255 };
static constexpr Color BG_PANEL   = {  25,  22,  35, 230 };
static constexpr Color BTN_NORMAL = {  55,  35,  85, 255 };
static constexpr Color BTN_HOVER  = {  85,  55, 125, 255 };
static constexpr Color BTN_PRESS  = {  35,  15,  55, 255 };
static constexpr Color ACCENT     = { 220,  75, 110, 255 };
static constexpr Color TEXT_DIM   = { 155, 145, 175, 255 };
static constexpr Color FIELD_BG   = {  20,  18,  30, 255 };

// ─── Shared helpers ───────────────────────────────────────────────────────────

// Draws a styled button; returns true on mouse-button release over it.
bool MainMenuScene::Button(const char* text, Rectangle rect) {
    Vector2 mouse = GetMousePosition();
    bool over  = CheckCollisionPointRec(mouse, rect);
    bool press = over && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool click = over && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

    Color bg     = press ? BTN_PRESS : (over ? BTN_HOVER : BTN_NORMAL);
    Color border = over  ? ACCENT    : Color{ 75, 55, 105, 255 };
    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, 2.f, border);        

    int fs = 22;
    int tw = MeasureText(text, fs);
    Color tc = over ? WHITE : Color{ 220, 210, 235, 255 };
    DrawText(text,
             (int)(rect.x + (rect.width  - tw) * 0.5f),
             (int)(rect.y + (rect.height - fs) * 0.5f),
             fs, tc);
    return click;
}

// Draws a labelled text-input box.
void MainMenuScene::TextField(const char* label, char* buf, int maxLen,
                               Rectangle rect, bool active) {
    Color border = active ? ACCENT : Color{ 75, 65, 100, 255 };
    DrawRectangleRec(rect, FIELD_BG);
    DrawRectangleLinesEx(rect, 2.f, border);

    // label above the box
    DrawText(label, (int)rect.x, (int)(rect.y - 22), 16, TEXT_DIM);

    int fs = 20;
    DrawText(buf,
             (int)(rect.x + 10),
             (int)(rect.y + (rect.height - fs) * 0.5f),
             fs, WHITE);

    // blinking cursor
    if (active && ((int)(GetTime() * 2) % 2 == 0)) {
        int cx = (int)(rect.x + 10) + MeasureText(buf, fs);
        DrawText("|", cx, (int)(rect.y + (rect.height - fs) * 0.5f), fs, ACCENT);
    }

    (void)maxLen; // used by callers
}

// Feeds keyboard input into a text buffer for the focused field.
static void UpdateBuf(char* buf, int maxLen) {
    int key = GetCharPressed();
    int len = (int)strlen(buf);
    while (key > 0) {
        if (key >= 32 && key < 127 && len < maxLen - 1) {
            buf[len++] = (char)key;
            buf[len]   = '\0';
        }
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && len > 0)
        buf[--len] = '\0';
}

// ─── Scene lifecycle ──────────────────────────────────────────────────────────

void MainMenuScene::Init() {
    EnableCursor();
    m_state       = State::Main;
    m_action      = Action::None;
    m_activeField = -1;
}

void MainMenuScene::Unload() {}

void MainMenuScene::Update() {
    // Feed typed characters into whichever field is active
    if (m_activeField < 0) return;

    if (m_state == State::Join) {
        if      (m_activeField == 0) UpdateBuf(m_ipBuffer,   64);
        else if (m_activeField == 1) UpdateBuf(m_nameBuffer, 16);
        else if (m_activeField == 2) UpdateBuf(m_portBuffer,  8);
    } else if (m_state == State::Host) {
        if      (m_activeField == 0) UpdateBuf(m_nameBuffer, 16);
        else if (m_activeField == 1) UpdateBuf(m_portBuffer,  8);
    }

    if (IsKeyPressed(KEY_ESCAPE)) m_state = State::Main;
}

void MainMenuScene::Draw() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // Background + subtle grid
    ClearBackground(BG_DARK);
    for (int x = 0; x < sw; x += 60) DrawLine(x, 0, x, sh, Color{ 28, 24, 40, 255 });
    for (int y = 0; y < sh; y += 60) DrawLine(0, y, sw, y, Color{ 28, 24, 40, 255 });

    // Title
    const char* title  = "HABANERO HOTEL";
    const int   titleFs = 72;
    DrawText(title, (sw - MeasureText(title, titleFs)) / 2, 70, titleFs, ACCENT);

    const char* sub   = "alpha v0.1";
    const int   subFs = 18;
    DrawText(sub, (sw - MeasureText(sub, subFs)) / 2, 70 + titleFs + 6, subFs, TEXT_DIM);

    if      (m_state == State::Main) DrawMain();
    else if (m_state == State::Join) DrawJoin();
    else if (m_state == State::Host) DrawHost();
}

// ─── Main menu sub-screen ─────────────────────────────────────────────────────

void MainMenuScene::DrawMain() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    float bw = 300, bh = 56;
    float cx = (sw - bw) * 0.5f;
    float cy = sh * 0.5f;

    if (Button("HOST GAME", { cx, cy,       bw, bh })) m_state = State::Host;
    if (Button("JOIN GAME", { cx, cy + 72,  bw, bh })) m_state = State::Join;
    if (Button("QUIT",      { cx, cy + 144, bw, bh })) {
        m_action = Action::Quit;
        MarkFinished();
    }

    const char* hint = "Tip: run with --server to start a dedicated headless server";
    DrawText(hint, (sw - MeasureText(hint, 14)) / 2, sh - 30, 14, TEXT_DIM);
}

// ─── Join game sub-screen ─────────────────────────────────────────────────────

void MainMenuScene::DrawJoin() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    float pw = 500, ph = 360;
    Rectangle panel = { (sw - pw) * 0.5f, (sh - ph) * 0.5f, pw, ph };
    DrawRectangleRec(panel, BG_PANEL);
    DrawRectangleLinesEx(panel, 2.f, ACCENT);
    DrawText("JOIN GAME", (int)(panel.x + 22), (int)(panel.y + 16), 28, WHITE);

    float fx = panel.x + 30, fw = pw - 60, fh = 44;

    // Field 0 – Server IP
    Rectangle r0 = { fx, panel.y + 72, fw, fh };
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), r0))
        m_activeField = 0;
    TextField("Server IP", m_ipBuffer, 64, r0, m_activeField == 0);

    // Field 1 – Player Name
    Rectangle r1 = { fx, panel.y + 152, fw, fh };
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), r1))
        m_activeField = 1;
    TextField("Player Name", m_nameBuffer, 16, r1, m_activeField == 1);

    // Field 2 – Port (half-width)
    Rectangle r2 = { fx, panel.y + 232, fw * 0.4f, fh };
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), r2))
        m_activeField = 2;
    TextField("Port", m_portBuffer, 8, r2, m_activeField == 2);

    // Buttons
    float bh2 = 46;
    if (Button("BACK",    { panel.x + 22,        panel.y + ph - bh2 - 18, 100, bh2 }))
        m_state = State::Main;
    if (Button("CONNECT", { panel.x + pw - 140, panel.y + ph - bh2 - 18, 118, bh2 })) {
        m_port = (uint16_t)std::atoi(m_portBuffer);
        if (m_port == 0) m_port = 27015;
        m_action = Action::Join;
        MarkFinished();
    }

    // Click outside panel → deselect field
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !CheckCollisionPointRec(GetMousePosition(), panel))
        m_activeField = -1;
}

// ─── Host game sub-screen ─────────────────────────────────────────────────────

void MainMenuScene::DrawHost() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    float pw = 440, ph = 300;
    Rectangle panel = { (sw - pw) * 0.5f, (sh - ph) * 0.5f, pw, ph };
    DrawRectangleRec(panel, BG_PANEL);
    DrawRectangleLinesEx(panel, 2.f, ACCENT);
    DrawText("HOST GAME", (int)(panel.x + 22), (int)(panel.y + 16), 28, WHITE);

    float fx = panel.x + 30, fw = pw - 60, fh = 44;

    // Field 0 – Player Name
    Rectangle r0 = { fx, panel.y + 72, fw, fh };
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), r0))
        m_activeField = 0;
    TextField("Your Name", m_nameBuffer, 16, r0, m_activeField == 0);

    // Field 1 – Port (half-width)
    Rectangle r1 = { fx, panel.y + 152, fw * 0.4f, fh };
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), r1))
        m_activeField = 1;
    TextField("Port", m_portBuffer, 8, r1, m_activeField == 1);

    float bh2 = 46;
    if (Button("BACK",  { panel.x + 22,        panel.y + ph - bh2 - 18, 100, bh2 }))
        m_state = State::Main;
    if (Button("START", { panel.x + pw - 140, panel.y + ph - bh2 - 18, 118, bh2 })) {
        m_port = (uint16_t)std::atoi(m_portBuffer);
        if (m_port == 0) m_port = 27015;
        m_action = Action::Host;
        MarkFinished();
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !CheckCollisionPointRec(GetMousePosition(), panel))
        m_activeField = -1;
}

} // namespace Hotones
