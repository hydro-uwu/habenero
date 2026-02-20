#include <GFX/MainMenuScene.hpp>
#include <raylib.h>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <filesystem>

namespace Hotones {

// ─── Palette ──────────────────────────────────────────────────────────────────
static constexpr Color BG_DARK     = {  15,  12,  20, 255 };
static constexpr Color BG_PANEL    = {  25,  22,  35, 230 };
static constexpr Color BTN_NORMAL  = {  55,  35,  85, 255 };
static constexpr Color BTN_HOVER   = {  85,  55, 125, 255 };
static constexpr Color BTN_PRESS   = {  35,  15,  55, 255 };
static constexpr Color ACCENT      = { 220,  75, 110, 255 };
static constexpr Color TEXT_DIM    = { 155, 145, 175, 255 };
static constexpr Color TEXT_BRIGHT = { 220, 210, 235, 255 };
static constexpr Color SEL_BG      = {  60,  40, 100, 255 };
static constexpr Color ROW_ALT     = {  22,  19,  32, 255 };

// ─── Constructor ──────────────────────────────────────────────────────────────
MainMenuScene::MainMenuScene(Net::NetworkManager* net)
    : m_net(net)
    , m_nameField("Player")
    , m_ipField("127.0.0.1")
    , m_portField("27015")
    , m_addIpField("127.0.0.1")
    , m_addPortField("27015")
{}

// ─── Scene lifecycle ──────────────────────────────────────────────────────────
void MainMenuScene::Init() {
    EnableCursor();
    m_state          = State::Main;
    m_action         = Action::None;
    m_activeField    = -1;
    m_addActiveField = -1;
    m_showAddServer  = false;

    RefreshPacks();

    if (m_net) {
        m_net->OnServerInfo = [this](const std::string& host, uint16_t port,
                                     uint8_t players, uint8_t maxPlayers,
                                     const char* pakName, const char* gameVersion,
                                     const char* pakVersion) {
            for (auto& s : m_servers) {
                if (s.host == host && s.port == port) {
                    s.playerCount = players;
                    s.maxPlayers  = maxPlayers;
                    std::memcpy(s.pakName, pakName, 32);
                    std::memcpy(s.gameVersion, gameVersion, 16);
                    std::memcpy(s.pakVersion, pakVersion, 16);
                    s.responded   = true;
                    s.pinging     = false;
                    return;
                }
            }
        };
    }
}

void MainMenuScene::Unload() {
    if (m_net) m_net->OnServerInfo = nullptr;
}

// ─── Pack helpers ─────────────────────────────────────────────────────────────
void MainMenuScene::RefreshPacks() {
    m_packs           = Assets::ScanPacksDir("./paks");
    m_selectedPack    = -1;
    m_selectedPakPath = "";
    m_packScroll      = 0;
}

bool MainMenuScene::MatchLocalPak(const char* pakName) {
    if (!pakName || !pakName[0]) return false;
    auto toLo = [](std::string s) {
        for (auto& c : s) c = (char)tolower((unsigned char)c);
        return s;
    };
    const std::string needle = toLo(pakName);
    for (int i = 0; i < (int)m_packs.size(); ++i) {
        std::filesystem::path fp(m_packs[i].displayName);
        if (toLo(fp.stem().string()) == needle ||
            toLo(m_packs[i].displayName) == needle) {
            m_selectedPakPath = m_packs[i].fullPath;
            return true;
        }
    }
    return false;
}

// ─── Server helpers ───────────────────────────────────────────────────────────
void MainMenuScene::PingAllServers() {
    if (!m_net) return;
    for (auto& s : m_servers) {
        s.responded = false;
        s.pinging   = true;
        m_net->PingServer(s.host, s.port);
    }
}

void MainMenuScene::AddServer(const char* host, uint16_t port) {
    for (auto& s : m_servers)
        if (s.host == host && s.port == port) return;
    ServerEntry e;
    e.host    = host;
    e.port    = port;
    e.pinging = (m_net != nullptr);
    m_servers.push_back(e);
    if (m_net) m_net->PingServer(host, port);
}

void MainMenuScene::RemoveSelectedServer() {
    if (m_selectedServer < 0 || m_selectedServer >= (int)m_servers.size()) return;
    m_servers.erase(m_servers.begin() + m_selectedServer);
    m_selectedServer = -1;
}

// ─── Update ───────────────────────────────────────────────────────────────────
void MainMenuScene::Update() {
    if (m_net) m_net->Update(); // drain ping results → OnServerInfo

    if (IsKeyPressed(KEY_ESCAPE) && m_state != State::Main) {
        m_state          = State::Main;
        m_activeField    = -1;
        m_addActiveField = -1;
        m_showAddServer  = false;
    }

    switch (m_state) {
    case State::Host:
        if      (m_activeField == 0) m_nameField.UpdateInput();
        else if (m_activeField == 1) m_portField.UpdateInput();
        break;
    case State::ServerBrowser:
        if (m_showAddServer) {
            if      (m_addActiveField == 0) m_addIpField.UpdateInput();
            else if (m_addActiveField == 1) m_addPortField.UpdateInput();
        }
        break;
    default: break;
    }
}

// ─── Draw ─────────────────────────────────────────────────────────────────────
void MainMenuScene::Draw() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    ClearBackground(BG_DARK);
    for (int x = 0; x < sw; x += 60) DrawLine(x, 0, x, sh, {28, 24, 40, 255});
    for (int y = 0; y < sh; y += 60) DrawLine(0, y, sw, y, {28, 24, 40, 255});

    const char* title = "Habenero";
    int tfs = 64;
    DrawText(title, (sw - MeasureText(title, tfs)) / 2, 50, tfs, ACCENT);
    DrawText("alpha v0.1",
             (sw - MeasureText("alpha v0.1", 16)) / 2, 50 + tfs + 4,
             16, TEXT_DIM);

    switch (m_state) {
    case State::Main:          DrawMain();          break;
    case State::ServerBrowser: DrawServerBrowser(); break;
    case State::Host:          DrawHost();          break;
    }
}

// ─── Main sub-screen ──────────────────────────────────────────────────────────
void MainMenuScene::DrawMain() {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float bw = 300, bh = 52;
    float cx = (sw - bw) * 0.5f;
    float cy = (float)sh * 0.5f - 40.f;

    if (Button("SERVER BROWSER", {cx, cy,       bw, bh})) {
        m_state = State::ServerBrowser;
        PingAllServers();
    }
    if (Button("HOST GAME",      {cx, cy + 68,  bw, bh})) m_state = State::Host;
    if (Button("QUIT",           {cx, cy + 136, bw, bh})) {
        m_action = Action::Quit;
        MarkFinished();
    }

    const char* hint = "Tip: place .cup packs in ./paks/   |   --server flag for headless mode";
    DrawText(hint, (sw - MeasureText(hint, 14)) / 2, sh - 28, 14, TEXT_DIM);
}

// ─── Server Browser sub-screen ────────────────────────────────────────────────
void MainMenuScene::DrawServerBrowser() {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float pw = 700, ph = 440;
    Rectangle panel = {(sw - pw) * 0.5f, (sh - ph) * 0.5f + 20.f, pw, ph};
    DrawRectangleRec(panel, BG_PANEL);
    DrawRectangleLinesEx(panel, 2.f, ACCENT);

    DrawText("SERVER BROWSER", (int)(panel.x + 16), (int)(panel.y + 12), 22, WHITE);

    int hy = (int)(panel.y + 46);
    DrawLine((int)panel.x, hy + 20, (int)(panel.x + pw), hy + 20, {55, 45, 75, 255});
    Label("HOST : PORT", (int)(panel.x + 10),  hy);
    Label("PLAYERS",     (int)(panel.x + 300), hy);
    Label("PACK",        (int)(panel.x + 400), hy);

    static constexpr int ROW_H   = 34;
    static constexpr int MAX_VIS = 7;
    int rowY = hy + 26;

    for (int i = m_serverScroll;
         i < (int)m_servers.size() && i < m_serverScroll + MAX_VIS; ++i) {
        const auto& s   = m_servers[i];
        bool        sel = (i == m_selectedServer);
        Rectangle rr = {panel.x + 4, (float)rowY, pw - 8, (float)(ROW_H - 2)};
        DrawRectangleRec(rr, sel ? SEL_BG : (i % 2 == 0 ? ROW_ALT : BG_PANEL));

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(GetMousePosition(), rr))
            m_selectedServer = i;

        char addr[80];
        snprintf(addr, sizeof(addr), "%s : %d", s.host.c_str(), (int)s.port);
        DrawText(addr, (int)(panel.x + 10), rowY + 8, 15, sel ? WHITE : TEXT_BRIGHT);

        if (s.pinging && !s.responded) {
            DrawText("...", (int)(panel.x + 300), rowY + 8, 15, TEXT_DIM);
        } else if (s.responded) {
            char pl[16];
            snprintf(pl, sizeof(pl), "%d / %d", s.playerCount, s.maxPlayers);
            DrawText(pl, (int)(panel.x + 300), rowY + 8, 15,
                     sel ? WHITE : TEXT_BRIGHT);
            DrawText(s.pakName[0] ? s.pakName : "\xe2\x80\x94",
                     (int)(panel.x + 400), rowY + 8, 15, ACCENT);
        } else {
            DrawText("?", (int)(panel.x + 300), rowY + 8, 15, TEXT_DIM);
        }
        rowY += ROW_H;
    }

    if (m_serverScroll > 0)
        DrawText("^", (int)(panel.x + pw - 18), (int)(panel.y + 50), 14, TEXT_DIM);
    if ((int)m_servers.size() > m_serverScroll + MAX_VIS)
        DrawText("v", (int)(panel.x + pw - 18), (int)(panel.y + ph - 110), 14, TEXT_DIM);

    float wheel = GetMouseWheelMove();
    if (wheel != 0.f && CheckCollisionPointRec(GetMousePosition(), panel)) {
        m_serverScroll -= (int)wheel;
        int maxScroll = (int)m_servers.size() - MAX_VIS;
        m_serverScroll = std::max(0, m_serverScroll);
        if (maxScroll > 0) m_serverScroll = std::min(m_serverScroll, maxScroll);
    }

    float bw = 110, bh = 36;
    float by  = panel.y + ph - bh - 10;

    if (Button("BACK",    {panel.x + 8,   by, bw, bh})) {
        m_state = State::Main; m_selectedServer = -1;
    }
    if (Button("REFRESH", {panel.x + 126, by, bw, bh})) PingAllServers();
    if (Button("ADD",     {panel.x + 244, by, bw, bh})) {
        m_showAddServer  = !m_showAddServer;
        m_addActiveField = -1;
    }

    if (m_selectedServer >= 0 && m_selectedServer < (int)m_servers.size()) {
        if (Button("REMOVE",  {panel.x + 362,      by, bw, bh}))
            RemoveSelectedServer();
        if (Button("CONNECT", {panel.x + pw - 128, by, 120, bh},
                   {80, 40, 120, 255}, WHITE)) {
            const auto& sv  = m_servers[m_selectedServer];
            // Game version check
            if (sv.gameVersion[0] != '\0' &&
                strncmp(sv.gameVersion, Net::GAME_VERSION, sizeof(sv.gameVersion)) != 0) {
                m_statusMessage = "Error: server game version mismatch.";
                m_statusTimer = 3.0f;
            } else if (sv.pakName[0] != '\0' && !MatchLocalPak(sv.pakName)) {
                // Pak advertised but missing locally
                m_statusMessage = "Error: required pack not installed.";
                m_statusTimer = 3.0f;
            } else {
                m_serverPakName = sv.pakName;
                m_selectedPakPath.clear();
                MatchLocalPak(sv.pakName);
                m_port = sv.port;
                m_ipField.setText(sv.host.c_str());
                m_action = Action::Join;
                MarkFinished();
            }
        }
    }

    // Inline "add server" form
    if (m_showAddServer) {
        Rectangle ap = {panel.x + 8, by - 58, pw - 16, 52};
        DrawRectangleRec(ap, {30, 20, 50, 240});
        DrawRectangleLinesEx(ap, 1.f, ACCENT);

        Rectangle ipR  = {ap.x + 8,   ap.y + 8, 260, 36};
        Rectangle prR  = {ap.x + 276, ap.y + 8,  90, 36};
        Rectangle addR = {ap.x + 374, ap.y + 8,  80, 36};

        Label("IP",   (int)ipR.x, (int)ap.y - 16);
        Label("PORT", (int)prR.x, (int)ap.y - 16);

        if (m_addIpField.Draw(ipR,  m_addActiveField == 0)) m_addActiveField = 0;
        if (m_addPortField.Draw(prR, m_addActiveField == 1)) m_addActiveField = 1;

        if (Button("ADD", addR)) {
            uint16_t p = (uint16_t)std::atoi(m_addPortField.text());
            if (p == 0) p = Net::DEFAULT_PORT;
            AddServer(m_addIpField.text(), p);
            m_showAddServer  = false;
            m_addActiveField = -1;
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            !CheckCollisionPointRec(GetMousePosition(), ap) &&
            !CheckCollisionPointRec(GetMousePosition(),
                                    {panel.x + 244, by, bw, bh}))
            m_showAddServer = false;
    }

    // Status message (temporary)
    if (!m_statusMessage.empty()) {
        DrawText(m_statusMessage.c_str(), (int)(panel.x + 8), (int)(panel.y + ph - 60), 16, ACCENT);
    }
}

// ─── Host sub-screen ──────────────────────────────────────────────────────────
void MainMenuScene::DrawHost() {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float pw = 540, ph = 470;
    Rectangle panel = {(sw - pw) * 0.5f, (sh - ph) * 0.5f + 20.f, pw, ph};
    DrawRectangleRec(panel, BG_PANEL);
    DrawRectangleLinesEx(panel, 2.f, ACCENT);
    DrawText("HOST GAME", (int)(panel.x + 16), (int)(panel.y + 12), 22, WHITE);

    float fx = panel.x + 24, fw = pw - 48, fh = 38;
    float fy = panel.y + 54;

    Label("Your Name", (int)fx,               (int)fy - 18);
    Label("Port",      (int)(fx + fw * 0.62f),(int)fy - 18);

    if (m_nameField.Draw({fx, fy, fw * 0.57f, fh}, m_activeField == 0))
        m_activeField = 0;
    if (m_portField.Draw({fx + fw * 0.62f, fy, fw * 0.36f, fh}, m_activeField == 1))
        m_activeField = 1;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Rectangle nr = {fx, fy, fw * 0.57f, fh};
        Rectangle pr = {fx + fw * 0.62f, fy, fw * 0.36f, fh};
        if (!CheckCollisionPointRec(GetMousePosition(), nr) &&
            !CheckCollisionPointRec(GetMousePosition(), pr))
            m_activeField = -1;
    }

    fy += fh + 26;
    Label("Select Pack  (./paks/)", (int)fx, (int)fy - 18);

    static constexpr int PACK_ROW_H = 32;
    static constexpr int PACK_MAX_V = 8;
    float listH = (float)(PACK_ROW_H * PACK_MAX_V);
    Rectangle listBg = {fx, fy, fw, listH};
    DrawRectangleRec(listBg, {18, 15, 28, 255});
    DrawRectangleLinesEx(listBg, 1.f, {55, 45, 80, 255});

    if (m_packs.empty()) {
        DrawText("No packs found  \xe2\x80\x94  place .cup files or pack folders in  ./paks/",
                 (int)(fx + 8), (int)(fy + listH * 0.5f - 8), 12, TEXT_DIM);
    } else {
        int rowY = (int)fy;
        for (int i = m_packScroll;
             i < (int)m_packs.size() && i < m_packScroll + PACK_MAX_V; ++i) {
            bool sel = (i == m_selectedPack);
            Rectangle rr = {fx + 2, (float)rowY, fw - 4, (float)(PACK_ROW_H - 2)};
            DrawRectangleRec(rr, sel ? SEL_BG : (i % 2 == 0 ? ROW_ALT : BG_PANEL));
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                CheckCollisionPointRec(GetMousePosition(), rr)) {
                m_selectedPack    = i;
                m_selectedPakPath = m_packs[i].fullPath;
            }
            const char* icon = (m_packs[i].type == Assets::PackEntry::Type::ZipFile)
                                ? "[zip] " : "[dir] ";
            char buf[96];
            snprintf(buf, sizeof(buf), "%s%s", icon, m_packs[i].displayName.c_str());
            DrawText(buf, (int)(fx + 8), rowY + 7, 15, sel ? WHITE : TEXT_BRIGHT);
            rowY += PACK_ROW_H;
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0.f && CheckCollisionPointRec(GetMousePosition(), listBg)) {
            m_packScroll -= (int)wheel;
            int maxScroll = (int)m_packs.size() - PACK_MAX_V;
            m_packScroll = std::max(0, m_packScroll);
            if (maxScroll > 0) m_packScroll = std::min(m_packScroll, maxScroll);
        }
    }

    if (Button("[refresh]", {fx + fw - 80, fy + listH + 4, 80, 24},
               {35, 25, 55, 255}, TEXT_DIM))
        RefreshPacks();

    float bw = 110, bh = 42;
    float by = panel.y + ph - bh - 10;

    if (Button("BACK", {panel.x + 12, by, bw, bh})) {
        m_state       = State::Main;
        m_activeField = -1;
    }
    if (Button("START", {panel.x + pw - bw - 12, by, bw + 20, bh},
               {80, 40, 120, 255}, WHITE)) {
        m_port = (uint16_t)std::atoi(m_portField.text());
        if (m_port == 0) m_port = 27015;
        m_action = Action::Host;
        MarkFinished();
    }
}

// ─── Shared helpers ───────────────────────────────────────────────────────────
bool MainMenuScene::Button(const char* text, Rectangle rect, Color bg, Color fg) {
    Vector2 mp  = GetMousePosition();
    bool over   = CheckCollisionPointRec(mp, rect);
    bool press  = over && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool click  = over && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    Color col   = press ? BTN_PRESS : (over ? BTN_HOVER : bg);
    Color bord  = over  ? ACCENT    : Color{75, 55, 105, 255};
    DrawRectangleRec(rect, col);
    DrawRectangleLinesEx(rect, 2.f, bord);
    int fs = 18, tw = MeasureText(text, fs);
    DrawText(text,
             (int)(rect.x + (rect.width  - tw) * 0.5f),
             (int)(rect.y + (rect.height - fs) * 0.5f),
             fs, over ? WHITE : fg);
    return click;
}

void MainMenuScene::Label(const char* text, int x, int y, int fs, Color col) {
    DrawText(text, x, y, fs, col);
}

} // namespace Hotones
