// Dino Runner - Chrome T-Rex style game in C++ with OpenGL (legacy pipeline) + GLFW.
//
// MENU      : UP/DOWN choose a row, LEFT/RIGHT change it, ENTER or SPACE start, ESC quit
//             rows: LEVEL (MEDIUM / HARD), TIME (MORNING / NOON / EVENING / NIGHT),
//                   DINO (T-REX / RAPTOR / TRICERATOPS / STEGOSAURUS)
// IN GAME   : SPACE or UP = jump, DOWN = duck / fast fall, ESC = back to menu
// GAME OVER : SPACE or ENTER = retry, M = menu
// ANYTIME   : R = toggle rain (rain also starts and stops by itself)
//             Q = quit
//
// Build: see README.md (CMake works the same on Windows, macOS and Linux;
//        Docker can build the Linux and Windows versions in one command).

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

// ------------------------------------------------------------------ constants
static const float W = 800.f, H = 300.f;   // logical world size
static const float GROUND_Y = 40.f;
static const float GRAVITY = 2400.f;
static const float JUMP_V = 800.f;
static const float DINO_X = 60.f;
static const float PI = 3.14159265f;

struct Rect  { float x, y, w, h; };
struct Color { float r, g, b; };

static bool overlap(const Rect& a, const Rect& b) {
    return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}
static Color mix(const Color& a, const Color& b, float t) {
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t};
}
static void setColor(const Color& c, float a = 1.f) { glColor4f(c.r, c.g, c.b, a); }

static void rect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x, y); glVertex2f(x + w, y);
    glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
}
static void circle(float cx, float cy, float r) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 32; ++i) {
        float a = 2.f * PI * i / 32.f;
        glVertex2f(cx + r * std::cos(a), cy + r * std::sin(a));
    }
    glEnd();
}

// ------------------------------------------------------------------ tiny 5x7 font
struct Glyph { char c; const char* rows; };   // 7 rows x 5 columns = 35 chars
static const Glyph FONT[] = {
    {'A', "01110" "10001" "10001" "11111" "10001" "10001" "10001"},
    {'B', "11110" "10001" "10001" "11110" "10001" "10001" "11110"},
    {'C', "01110" "10001" "10000" "10000" "10000" "10001" "01110"},
    {'D', "11110" "10001" "10001" "10001" "10001" "10001" "11110"},
    {'E', "11111" "10000" "10000" "11110" "10000" "10000" "11111"},
    {'F', "11111" "10000" "10000" "11110" "10000" "10000" "10000"},
    {'G', "01110" "10001" "10000" "10111" "10001" "10001" "01111"},
    {'H', "10001" "10001" "10001" "11111" "10001" "10001" "10001"},
    {'I', "01110" "00100" "00100" "00100" "00100" "00100" "01110"},
    {'J', "00111" "00010" "00010" "00010" "00010" "10010" "01100"},
    {'K', "10001" "10010" "10100" "11000" "10100" "10010" "10001"},
    {'L', "10000" "10000" "10000" "10000" "10000" "10000" "11111"},
    {'M', "10001" "11011" "10101" "10101" "10001" "10001" "10001"},
    {'N', "10001" "11001" "10101" "10011" "10001" "10001" "10001"},
    {'O', "01110" "10001" "10001" "10001" "10001" "10001" "01110"},
    {'P', "11110" "10001" "10001" "11110" "10000" "10000" "10000"},
    {'Q', "01110" "10001" "10001" "10001" "10101" "10010" "01101"},
    {'R', "11110" "10001" "10001" "11110" "10100" "10010" "10001"},
    {'S', "01111" "10000" "10000" "01110" "00001" "00001" "11110"},
    {'T', "11111" "00100" "00100" "00100" "00100" "00100" "00100"},
    {'U', "10001" "10001" "10001" "10001" "10001" "10001" "01110"},
    {'V', "10001" "10001" "10001" "10001" "10001" "01010" "00100"},
    {'W', "10001" "10001" "10001" "10101" "10101" "11011" "10001"},
    {'X', "10001" "10001" "01010" "00100" "01010" "10001" "10001"},
    {'Y', "10001" "10001" "01010" "00100" "00100" "00100" "00100"},
    {'Z', "11111" "00001" "00010" "00100" "01000" "10000" "11111"},
    {'0', "01110" "10001" "10011" "10101" "11001" "10001" "01110"},
    {'1', "00100" "01100" "00100" "00100" "00100" "00100" "01110"},
    {'2', "01110" "10001" "00001" "00010" "00100" "01000" "11111"},
    {'3', "11110" "00001" "00001" "01110" "00001" "00001" "11110"},
    {'4', "00010" "00110" "01010" "10010" "11111" "00010" "00010"},
    {'5', "11111" "10000" "11110" "00001" "00001" "10001" "01110"},
    {'6', "00110" "01000" "10000" "11110" "10001" "10001" "01110"},
    {'7', "11111" "00001" "00010" "00100" "01000" "01000" "01000"},
    {'8', "01110" "10001" "10001" "01110" "10001" "10001" "01110"},
    {'9', "01110" "10001" "10001" "01111" "00001" "00010" "01100"},
    {'-', "00000" "00000" "00000" "11111" "00000" "00000" "00000"},
    {'<', "00010" "00100" "01000" "10000" "01000" "00100" "00010"},
    {'>', "01000" "00100" "00010" "00001" "00010" "00100" "01000"},
    {':', "00000" "00100" "00000" "00000" "00100" "00000" "00000"},
};

static const char* findGlyph(char c) {
    for (const Glyph& g : FONT) if (g.c == c) return g.rows;
    return nullptr;
}
static float textWidth(const std::string& s, float sc) { return s.size() * 6.f * sc - sc; }

static void drawText(const std::string& s, float x, float y, float sc) {
    glBegin(GL_QUADS);
    for (char ch : s) {
        const char* g = findGlyph(ch);
        if (g) {
            for (int r = 0; r < 7; ++r)
                for (int c = 0; c < 5; ++c)
                    if (g[r * 5 + c] == '1') {
                        float px = x + c * sc, py = y + (6 - r) * sc;
                        glVertex2f(px, py); glVertex2f(px + sc, py);
                        glVertex2f(px + sc, py + sc); glVertex2f(px, py + sc);
                    }
        }
        x += 6.f * sc;
    }
    glEnd();
}
static void drawTextCentered(const std::string& s, float cx, float y, float sc) {
    drawText(s, cx - textWidth(s, sc) / 2.f, y, sc);
}

// 7-segment score digits
static void drawDigit(int d, float x, float y, float w, float h, float t) {
    static const int mask[10] = {63, 6, 91, 79, 102, 109, 125, 7, 127, 111};
    int m = mask[d];
    if (m & 1)  rect(x + t, y + h - t, w - 2 * t, t);
    if (m & 2)  rect(x + w - t, y + h / 2, t, h / 2);
    if (m & 4)  rect(x + w - t, y, t, h / 2);
    if (m & 8)  rect(x + t, y, w - 2 * t, t);
    if (m & 16) rect(x, y, t, h / 2);
    if (m & 32) rect(x, y + h / 2, t, h / 2);
    if (m & 64) rect(x + t, y + h / 2 - t / 2, w - 2 * t, t);
}
static void drawNumber(int n, float rightX, float y) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%05d", n);
    const float dw = 12, dh = 20, gap = 5;
    float x = rightX - 5 * (dw + gap);
    for (int i = 0; buf[i]; ++i, x += dw + gap) drawDigit(buf[i] - '0', x, y, dw, dh, 3);
}

// ------------------------------------------------------------------ choices
struct Level {
    const char* name;
    float startSpeed, accel, maxSpeed;
    float gapMin, gapMax;     // gap between obstacles, in seconds of travel
    int   birdScore;          // score after which birds can appear
    float birdChance;
    int   maxCluster;         // max cacti in one group (long obstacle)
};
static const Level LEVELS[2] = {
    {"MEDIUM", 320.f,  9.f,  820.f, 1.10f, 1.90f, 300, 0.30f, 1},
    {"HARD",   480.f, 14.f, 1000.f, 0.70f, 1.20f, 150, 0.40f, 3},
};

struct Theme {
    const char* name;
    Color skyTop, skyBot, ground, groundLine, obstacle, cloud, orb;
    bool isSun;
    float orbX, orbY;
    bool stars;
};
static const Theme THEMES[4] = {
    {"MORNING", {0.45f, 0.75f, 0.95f}, {1.00f, 0.88f, 0.70f}, {0.80f, 0.70f, 0.55f},
     {0.40f, 0.33f, 0.25f}, {0.20f, 0.45f, 0.25f}, {1.00f, 1.00f, 1.00f}, {1.00f, 0.80f, 0.30f},
     true, 130.f, 90.f, false},
    {"NOON",    {0.25f, 0.60f, 0.95f}, {0.70f, 0.88f, 1.00f}, {0.85f, 0.78f, 0.55f},
     {0.45f, 0.38f, 0.25f}, {0.15f, 0.42f, 0.20f}, {1.00f, 1.00f, 1.00f}, {1.00f, 0.95f, 0.40f},
     true, 640.f, 255.f, false},
    {"EVENING", {0.22f, 0.18f, 0.42f}, {1.00f, 0.50f, 0.25f}, {0.45f, 0.30f, 0.25f},
     {0.25f, 0.15f, 0.15f}, {0.12f, 0.17f, 0.22f}, {0.95f, 0.60f, 0.55f}, {1.00f, 0.35f, 0.15f},
     true, 670.f, 80.f, false},
    {"NIGHT",   {0.02f, 0.03f, 0.12f}, {0.12f, 0.14f, 0.30f}, {0.18f, 0.20f, 0.28f},
     {0.55f, 0.60f, 0.70f}, {0.40f, 0.70f, 0.45f}, {0.25f, 0.28f, 0.42f}, {0.95f, 0.95f, 0.85f},
     false, 650.f, 245.f, true},
};

struct Species {
    const char* name;
    Color col;
    Rect stand, duck;   // hitboxes relative to the sprite origin
};
static const Species SPECIES[4] = {
    {"T-REX",       {0.30f, 0.72f, 0.35f}, {6, 2, 30, 40}, {4, 2, 44, 22}},
    {"RAPTOR",      {0.95f, 0.55f, 0.15f}, {6, 2, 26, 38}, {4, 2, 42, 20}},
    {"TRICERATOPS", {0.45f, 0.60f, 0.90f}, {2, 2, 52, 32}, {2, 2, 52, 22}},
    {"STEGOSAURUS", {0.75f, 0.70f, 0.25f}, {0, 2, 54, 32}, {0, 2, 54, 20}},
};

// ------------------------------------------------------------------ game state
enum State { MENU, PLAYING, OVER };

struct Obstacle {
    enum Type { CACTUS, BIRD } type;
    float x, y, w, h;
    Rect hitbox() const { return {x + 3, y + 3, w - 6, h - 6}; }
};
struct Drop { float x, y; };

static int wrapIdx(int v, int n) { return ((v % n) + n) % n; }

struct Game {
    State state = MENU;
    int levelIdx = 0, timeIdx = 1, speciesIdx = 0, menuRow = 0;

    float dinoY = GROUND_Y, dinoVY = 0;
    bool ducking = false, onGround = true;

    std::vector<Obstacle> obstacles;
    float speed = 320.f, distance = 0.f, nextGap = 400.f;
    float groundScroll = 0.f, animTime = 0.f, scrollSpeed = 120.f;
    int hiScore[2] = {0, 0};

    float cloudX[4], cloudY[4];
    float starX[70], starY[70], starPhase[70];

    bool raining = false;
    float rainTimer = 8.f, rainAlpha = 0.f, flash = 0.f, lightningTimer = 4.f;
    std::vector<Drop> drops;

    std::mt19937 rng;

    float rnd(float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); }

    Game() : rng(std::random_device{}()) {
        for (int i = 0; i < 4; ++i) { cloudX[i] = 100.f + i * 200.f; cloudY[i] = rnd(170, 262); }
        for (int i = 0; i < 70; ++i) {
            starX[i] = rnd(0, W); starY[i] = rnd(110, H); starPhase[i] = rnd(0, 6.28f);
        }
        drops.resize(180);
        for (auto& d : drops) { d.x = rnd(0, W + 150); d.y = rnd(GROUND_Y, H + 40); }
        rainTimer = rnd(6, 14);
    }

    const Level& level() const { return LEVELS[levelIdx]; }
    int score() const { return (int)(distance / 10.f); }

    void startGame() {
        dinoY = GROUND_Y; dinoVY = 0; ducking = false; onGround = true;
        obstacles.clear();
        speed = level().startSpeed; distance = 0.f; nextGap = 450.f;
        state = PLAYING;
    }

    void jump() {
        if (state == PLAYING && onGround) { dinoVY = JUMP_V; onGround = false; }
    }

    Rect dinoHitbox() const {
        const Species& s = SPECIES[speciesIdx];
        const Rect& r = (ducking && onGround) ? s.duck : s.stand;
        return {DINO_X + r.x, dinoY + r.y, r.w, r.h};
    }

    void spawn() {
        const Level& L = level();
        bool bird = score() > L.birdScore && rnd(0, 1) < L.birdChance;
        if (bird) {
            Obstacle o;
            o.type = Obstacle::BIRD;
            o.w = 42; o.h = 28; o.x = W + 10;
            o.y = GROUND_Y + (rnd(0, 1) < 0.5f ? 30.f : 6.f);  // high: duck, low: jump
            obstacles.push_back(o);
        } else {
            int n = 1 + (L.maxCluster > 1 ? (int)rnd(0, (float)L.maxCluster) : 0);
            n = std::min(n, L.maxCluster);
            float x = W + 10;
            for (int i = 0; i < n; ++i) {
                Obstacle o;
                o.type = Obstacle::CACTUS;
                o.w = rnd(0, 1) < 0.5f ? 22.f : 40.f;
                o.h = rnd(34.f, 58.f);
                o.x = x; o.y = GROUND_Y;
                obstacles.push_back(o);
                x += o.w + rnd(8.f, 16.f);
            }
        }
        nextGap = speed * rnd(L.gapMin, L.gapMax);
    }

    // clouds, ground scroll, weather: runs in every state
    void updateAmbient(float dt) {
        animTime += dt;
        scrollSpeed = (state == PLAYING) ? speed : (state == MENU ? 120.f : 0.f);
        groundScroll = std::fmod(groundScroll + scrollSpeed * dt, 40.f);

        for (int i = 0; i < 4; ++i) {
            cloudX[i] -= (20.f + scrollSpeed * 0.1f) * dt;
            if (cloudX[i] < -70) { cloudX[i] = W + rnd(0, 150); cloudY[i] = rnd(170, 262); }
        }

        // random rain on / off
        rainTimer -= dt;
        if (rainTimer <= 0) {
            raining = !raining;
            rainTimer = raining ? rnd(8, 18) : rnd(12, 28);
        }
        float target = raining ? 1.f : 0.f;
        float step = 0.4f * dt;
        rainAlpha += std::max(-step, std::min(step, target - rainAlpha));

        for (auto& d : drops) {
            d.y -= 720.f * dt;
            d.x -= 130.f * dt;
            if (d.y < GROUND_Y || d.x < -20) { d.x = rnd(0, W + 150); d.y = H + rnd(0, 40); }
        }

        // lightning
        if (rainAlpha > 0.7f) {
            lightningTimer -= dt;
            if (lightningTimer <= 0) { flash = 1.f; lightningTimer = rnd(4, 10); }
        }
        flash = std::max(0.f, flash - 3.5f * dt);
    }

    void updateGame(float dt) {
        const Level& L = level();
        speed = std::min(L.maxSpeed, speed + L.accel * dt);
        distance += speed * dt;

        float g = GRAVITY * ((ducking && !onGround) ? 2.5f : 1.f);
        dinoVY -= g * dt;
        dinoY += dinoVY * dt;
        if (dinoY <= GROUND_Y) { dinoY = GROUND_Y; dinoVY = 0; onGround = true; }

        for (auto& o : obstacles) o.x -= speed * dt;
        obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(),
                        [](const Obstacle& o) { return o.x + o.w < -10; }), obstacles.end());

        if (obstacles.empty() || obstacles.back().x + obstacles.back().w < W - nextGap) spawn();

        Rect d = dinoHitbox();
        for (const auto& o : obstacles)
            if (overlap(d, o.hitbox())) {
                state = OVER;
                hiScore[levelIdx] = std::max(hiScore[levelIdx], score());
                break;
            }
    }
};

// ------------------------------------------------------------------ dinosaurs
static void eye(float x, float y, bool dead) {
    if (dead) { glColor3f(0.9f, 0.1f, 0.1f); rect(x, y, 4, 4); }
    else { glColor3f(1, 1, 1); rect(x, y, 4, 4); glColor3f(0, 0, 0); rect(x + 2, y + 1, 2, 2); }
}

// (x, y) = bottom-left of the sprite. step toggles the running legs.
static void drawDino(int sp, float x, float y, bool duck, bool step, bool run, bool dead) {
    const Color c = SPECIES[sp].col;
    const Color dark = mix(c, Color{0, 0, 0}, 0.30f);
    float la = (!run || step) ? 1.f : 0.55f;    // leg length factors
    float lb = (!run || !step) ? 1.f : 0.55f;

    switch (sp) {
    case 0:  // ---------------------------------------------------- T-REX
        setColor(c);
        if (duck) {
            rect(x + 12, y, 6, 8 * la); rect(x + 28, y, 6, 8 * lb);
            rect(x, y + 8, 40, 12);
            rect(x + 34, y + 14, 16, 12);
            eye(x + 42, y + 19, dead);
        } else {
            rect(x + 10, y, 6, 14 * la); rect(x + 22, y, 6, 14 * lb);
            rect(x + 4, y + 14, 28, 18);
            rect(x - 4, y + 20, 10, 6);
            rect(x + 22, y + 30, 20, 14);
            setColor(dark); rect(x + 30, y + 18, 6, 3);
            eye(x + 33, y + 36, dead);
            glColor3f(1, 1, 1); rect(x + 36, y + 30, 5, 2);
        }
        break;
    case 1:  // ---------------------------------------------------- RAPTOR
        setColor(c);
        if (duck) {
            rect(x + 12, y, 5, 8 * la); rect(x + 26, y, 5, 8 * lb);
            rect(x + 4, y + 8, 34, 10);
            rect(x - 6, y + 12, 12, 4);
            rect(x + 32, y + 12, 14, 10);
            setColor(dark); rect(x + 12, y + 10, 3, 6); rect(x + 19, y + 10, 3, 6);
            eye(x + 38, y + 16, dead);
        } else {
            rect(x + 10, y, 5, 14 * la); rect(x + 21, y, 5, 14 * lb);
            rect(x + 6, y + 14, 24, 14);
            rect(x - 4, y + 20, 12, 5); rect(x - 8, y + 22, 6, 3);
            rect(x + 24, y + 26, 16, 12);
            setColor(dark); rect(x + 12, y + 16, 3, 10); rect(x + 19, y + 16, 3, 10);
            glColor3f(0.9f, 0.9f, 0.9f); rect(x + 30, y + 14, 5, 2);
            eye(x + 32, y + 32, dead);
        }
        break;
    case 2:  // ---------------------------------------------------- TRICERATOPS
        if (duck) {
            setColor(c);
            rect(x + 4, y, 8, 6 * la); rect(x + 28, y, 8, 6 * lb);
            rect(x, y + 6, 38, 16);
            glColor3f(0.85f, 0.45f, 0.55f); rect(x + 30, y + 14, 8, 10);
            setColor(c); rect(x + 36, y + 8, 14, 12);
            glColor3f(0.95f, 0.95f, 0.85f); rect(x + 44, y + 20, 4, 6); rect(x + 50, y + 12, 4, 4);
            eye(x + 41, y + 14, dead);
        } else {
            setColor(c);
            rect(x + 4, y, 8, 12 * la); rect(x + 28, y, 8, 12 * lb);
            rect(x, y + 12, 38, 20);
            setColor(dark); rect(x, y + 12, 38, 4);
            glColor3f(0.85f, 0.45f, 0.55f); rect(x + 30, y + 22, 8, 14);
            setColor(c); rect(x + 36, y + 14, 14, 14);
            glColor3f(0.95f, 0.95f, 0.85f); rect(x + 44, y + 28, 4, 8); rect(x + 50, y + 20, 4, 4);
            eye(x + 41, y + 22, dead);
        }
        break;
    default: // ---------------------------------------------------- STEGOSAURUS
        {
            static const float ph[4] = {5, 8, 8, 6};
            if (duck) {
                setColor(c);
                rect(x + 6, y, 8, 6 * la); rect(x + 32, y, 8, 6 * lb);
                rect(x + 2, y + 6, 42, 12);
                rect(x - 8, y + 8, 12, 6);
                rect(x + 44, y + 8, 10, 8);
                glColor3f(0.9f, 0.45f, 0.15f);
                for (int i = 0; i < 4; ++i) rect(x + 6 + i * 9, y + 18, 6, ph[i] * 0.5f);
                eye(x + 49, y + 10, dead);
            } else {
                setColor(c);
                rect(x + 6, y, 8, 10 * la); rect(x + 32, y, 8, 10 * lb);
                rect(x + 2, y + 10, 42, 18);
                rect(x - 8, y + 12, 12, 8);
                rect(x + 42, y + 16, 6, 10);
                rect(x + 46, y + 14, 8, 8);
                glColor3f(0.9f, 0.9f, 0.85f); rect(x - 10, y + 18, 3, 5); rect(x - 4, y + 20, 3, 5);
                glColor3f(0.9f, 0.45f, 0.15f);
                for (int i = 0; i < 4; ++i) rect(x + 6 + i * 9, y + 28, 6, ph[i]);
                eye(x + 49, y + 17, dead);
            }
        }
        break;
    }
}

static void drawObstacle(const Obstacle& o, float t, const Color& col) {
    setColor(col);
    if (o.type == Obstacle::CACTUS) {
        float tw = o.w * (o.w > 30 ? 0.3f : 0.6f);
        rect(o.x + (o.w - tw) / 2, o.y, tw, o.h);
        if (o.w > 30) {
            rect(o.x, o.y + o.h * 0.35f, 8, 5);
            rect(o.x, o.y + o.h * 0.35f, 5, o.h * 0.35f);
            rect(o.x + o.w - 8, o.y + o.h * 0.45f, 8, 5);
            rect(o.x + o.w - 5, o.y + o.h * 0.45f, 5, o.h * 0.3f);
        }
    } else {
        bool up = ((int)(t * 8)) % 2 == 0;
        rect(o.x + 8, o.y + 8, 28, 10);
        rect(o.x, o.y + 12, 10, 6);
        rect(o.x + 14, up ? o.y + 18 : o.y, 14, 10);
    }
}

// ------------------------------------------------------------------ rendering
static void renderMenu(const Game& g, bool step) {
    glColor4f(0, 0, 0, 0.50f);
    rect(40, 52, 430, 188);

    glColor4f(0, 0, 0, 0.6f);
    drawTextCentered("DINO RUNNER", W / 2 + 3, 247, 4);
    glColor4f(1, 1, 1, 1);
    drawTextCentered("DINO RUNNER", W / 2, 250, 4);

    const char* labels[3] = {"LEVEL", "TIME", "DINO"};
    std::string vals[3] = {LEVELS[g.levelIdx].name, THEMES[g.timeIdx].name, SPECIES[g.speciesIdx].name};
    for (int r = 0; r < 3; ++r) {
        float y = 195.f - 37.f * r;
        bool sel = (r == g.menuRow);
        if (sel) glColor4f(1.f, 0.85f, 0.2f, 1.f); else glColor4f(0.9f, 0.9f, 0.9f, 1.f);
        drawText(labels[r], 60, y, 2);
        drawText(vals[r], 232, y, 2);
        if (sel) {
            drawText("<", 208, y, 2);
            drawText(">", 232 + textWidth(vals[r], 2) + 12, y, 2);
        }
    }
    glColor4f(0.8f, 0.8f, 0.8f, 1.f);
    drawText("ARROWS: CHOOSE", 60, 88, 2);
    drawText("ENTER: START  R: RAIN", 60, 66, 2);

    // big preview of the selected dino
    glPushMatrix();
    glTranslatef(590.f, GROUND_Y, 0.f);
    glScalef(2.f, 2.f, 1.f);
    drawDino(g.speciesIdx, 0, 0, false, step, true, false);
    glPopMatrix();
}

static void render(const Game& g) {
    const Theme& T = THEMES[g.timeIdx];
    bool step = ((int)(g.animTime * 10)) % 2 == 0;

    // sky gradient
    glBegin(GL_QUADS);
    setColor(T.skyBot); glVertex2f(0, 0); glVertex2f(W, 0);
    setColor(T.skyTop); glVertex2f(W, H); glVertex2f(0, H);
    glEnd();

    // stars
    if (T.stars) {
        for (int i = 0; i < 70; ++i) {
            float tw = 0.5f + 0.5f * std::sin(g.animTime * 2.f + g.starPhase[i]);
            glColor4f(1.f, 1.f, 0.9f, (0.3f + 0.7f * tw) * (1.f - g.rainAlpha));
            rect(g.starX[i], g.starY[i], 2, 2);
        }
    }

    // sun or moon
    if (T.isSun) {
        setColor(T.orb, 0.25f); circle(T.orbX, T.orbY, 40);
        setColor(T.orb);        circle(T.orbX, T.orbY, 24);
    } else {
        setColor(T.orb);        circle(T.orbX, T.orbY, 22);
        setColor(mix(T.skyBot, T.skyTop, T.orbY / H)); circle(T.orbX + 10, T.orbY + 5, 19);
    }

    // clouds (greyer in the rain)
    setColor(mix(T.cloud, Color{0.40f, 0.42f, 0.48f}, g.rainAlpha));
    for (int i = 0; i < 4; ++i) {
        rect(g.cloudX[i], g.cloudY[i], 46, 10);
        rect(g.cloudX[i] + 10, g.cloudY[i] + 10, 24, 8);
        rect(g.cloudX[i] + 30, g.cloudY[i] + 8, 18, 6);
    }

    // storm darkening
    glColor4f(0.05f, 0.08f, 0.15f, 0.32f * g.rainAlpha);
    rect(0, 0, W, H);

    // ground (darker when wet)
    setColor(mix(T.ground, Color{0.10f, 0.12f, 0.18f}, 0.3f * g.rainAlpha));
    rect(0, 0, W, GROUND_Y - 2);
    setColor(T.groundLine);
    rect(0, GROUND_Y - 2, W, 2);
    setColor(T.groundLine, 0.6f);
    for (float x = -g.groundScroll; x < W; x += 40.f) {
        rect(x + 8, GROUND_Y - 10, 8, 2);
        rect(x + 28, GROUND_Y - 18, 4, 2);
    }

    if (g.state == MENU) {
        renderMenu(g, step);
    } else {
        for (const auto& o : g.obstacles) drawObstacle(o, g.animTime, T.obstacle);
        bool dead = (g.state == OVER);
        drawDino(g.speciesIdx, DINO_X, g.dinoY, g.ducking && g.onGround, step,
                 g.onGround && !dead, dead);

        // HUD
        Color tc = g.timeIdx <= 1 ? Color{0.12f, 0.14f, 0.20f} : Color{0.95f, 0.95f, 0.95f};
        tc = mix(tc, Color{0.95f, 0.95f, 0.95f}, g.rainAlpha * 0.6f);
        setColor(tc);
        drawNumber(g.score(), W - 20, H - 36);
        setColor(tc, 0.7f);
        drawNumber(g.hiScore[g.levelIdx], W - 125, H - 36);
        drawText("HI", W - 246, H - 34, 2);
        setColor(tc);
        std::string label = std::string(SPECIES[g.speciesIdx].name) + "  " + LEVELS[g.levelIdx].name +
                            "  " + T.name + (g.raining ? "  RAIN" : "");
        drawText(label, 14, H - 30, 2);

        if (dead) {
            glColor4f(0, 0, 0, 0.55f);
            rect(W / 2 - 190, 95, 380, 110);
            glColor4f(1, 1, 1, 1);
            drawTextCentered("GAME OVER", W / 2, 165, 4);
            drawTextCentered("PRESS SPACE TO RETRY", W / 2, 135, 2);
            drawTextCentered("M: MENU", W / 2, 108, 2);
        }
    }

    // rain streaks in front of everything
    int n = (int)(g.drops.size() * g.rainAlpha);
    if (n > 0) {
        glLineWidth(1.5f);
        glColor4f(0.75f, 0.85f, 1.f, 0.55f);
        glBegin(GL_LINES);
        for (int i = 0; i < n; ++i) {
            glVertex2f(g.drops[i].x, g.drops[i].y);
            glVertex2f(g.drops[i].x - 3.2f, g.drops[i].y - 18.f);
        }
        glEnd();
    }

    // lightning flash
    if (g.flash > 0.f) {
        glColor4f(1, 1, 1, 0.55f * g.flash);
        rect(0, 0, W, H);
    }
}

// ------------------------------------------------------------------ input / main
static Game game;

static void keyCallback(GLFWwindow* win, int key, int, int action, int) {
    if (action == GLFW_RELEASE) return;
    bool press = (action == GLFW_PRESS);

    if (key == GLFW_KEY_Q && press) {               // Q or q: quit from anywhere
        glfwSetWindowShouldClose(win, GLFW_TRUE);
        return;
    }

    if (key == GLFW_KEY_R && press) {               // toggle rain any time
        game.raining = !game.raining;
        game.rainTimer = game.raining ? game.rnd(8, 18) : game.rnd(12, 28);
        return;
    }

    switch (game.state) {
    case MENU:
        if (key == GLFW_KEY_UP)   game.menuRow = wrapIdx(game.menuRow - 1, 3);
        if (key == GLFW_KEY_DOWN) game.menuRow = wrapIdx(game.menuRow + 1, 3);
        if (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT) {
            int d = (key == GLFW_KEY_RIGHT) ? 1 : -1;
            if (game.menuRow == 0) game.levelIdx   = wrapIdx(game.levelIdx + d, 2);
            if (game.menuRow == 1) game.timeIdx    = wrapIdx(game.timeIdx + d, 4);
            if (game.menuRow == 2) game.speciesIdx = wrapIdx(game.speciesIdx + d, 4);
        }
        if (press && (key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE)) game.startGame();
        if (press && key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, GLFW_TRUE);
        break;
    case PLAYING:
        if (press && (key == GLFW_KEY_SPACE || key == GLFW_KEY_UP)) game.jump();
        if (press && key == GLFW_KEY_ESCAPE) game.state = MENU;
        break;
    case OVER:
        if (press && (key == GLFW_KEY_SPACE || key == GLFW_KEY_ENTER)) game.startGame();
        if (press && (key == GLFW_KEY_M || key == GLFW_KEY_ESCAPE)) game.state = MENU;
        break;
    }
}

int main() {
    if (!glfwInit()) { std::fprintf(stderr, "Failed to init GLFW\n"); return 1; }
    // Start at 1.5x the logical size (1200x450). The window can be resized or
    // maximised, but keeps its shape so the game never looks stretched.
    GLFWwindow* win = glfwCreateWindow((int)(W * 1.5f), (int)(H * 1.5f), "Dino Runner", nullptr, nullptr);
    if (!win) { std::fprintf(stderr, "Failed to create window\n"); glfwTerminate(); return 1; }
    glfwSetWindowAspectRatio(win, (int)W, (int)H);
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);
    glfwSetKeyCallback(win, keyCallback);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    double last = glfwGetTime();
    while (!glfwWindowShouldClose(win)) {
        double now = glfwGetTime();
        float dt = std::min(0.05f, (float)(now - last));
        last = now;

        glfwPollEvents();
        game.ducking = (game.state == PLAYING) && glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS;
        game.updateAmbient(dt);
        if (game.state == PLAYING) game.updateGame(dt);

        int fw, fh;
        glfwGetFramebufferSize(win, &fw, &fh);
        glViewport(0, 0, fw, fh);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, W, 0, H, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        render(game);
        glfwSwapBuffers(win);
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
