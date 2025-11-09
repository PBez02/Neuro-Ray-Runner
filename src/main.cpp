#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cmath>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "Cave.hpp"
#include "NeuralNet.hpp"
#include "Agent.hpp"

using namespace std;

// =================== Tunables ===================
static constexpr int   NUM_RAYS = 7;
static constexpr float RAY_FOV  = 1.2f;
static constexpr float RAY_MAX  = 700.f;
static constexpr float RAY_STEP = 4.f;

static const int   POP_SIZE    = 50;
static const int   ELITE_COUNT = 5;
static const float MUT_PROB    = 0.15f;
static const float MUT_SIGMA   = 0.30f;

static const float VX = 220.f;  // horizontal scroll speed (px/s)
static const float VY = 200.f;  // max vertical speed (px/s)

static const float PIXELS_PER_POINT = 50.f; // scoring resolution
static const float ARROW_SIZE       = 20.f;

// =================== Utils (drawing) ===================
static void drawText(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y) {
    SDL_Color color = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dest = {x, y, surface->w, surface->h};
    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, nullptr, &dest);
    SDL_DestroyTexture(texture);
}

static void drawArrow(SDL_Renderer* renderer, float x, float y, float angle, float size) {
    float half = size * 0.5f;

    SDL_FPoint p1 = {x + size, y};
    SDL_FPoint p2 = {x - half, y - half};
    SDL_FPoint p3 = {x - half, y + half};

    auto rot = [&](SDL_FPoint p) {
        float s = sinf(angle), c = cosf(angle);
        float px = p.x - x, py = p.y - y;
        return SDL_FPoint{px * c - py * s + x, px * s + py * c + y};
    };
    p1 = rot(p1); p2 = rot(p2); p3 = rot(p3);

    Uint8 r,g,b,a; SDL_GetRenderDrawColor(renderer,&r,&g,&b,&a);
    SDL_Color col = {r,g,b,a};

    SDL_Vertex vtx[3];
    vtx[0].position = p1; vtx[0].color = col;
    vtx[1].position = p2; vtx[1].color = col;
    vtx[2].position = p3; vtx[2].color = col;

    SDL_RenderGeometry(renderer, nullptr, vtx, 3, nullptr, 0);
}

static float castRay(const Cave& cave, int W, int H, float ox, float oy,
                     float angle, float maxDist = RAY_MAX, float step = RAY_STEP) {
    float dx = cosf(angle) * step;
    float dy = sinf(angle) * step;
    float x = ox, y = oy, dist = 0.f;
    while (dist < maxDist) {
        float topY, botY;
        cave.sample(W, H, x, topY, botY);
        if (y < topY || y > botY) break;
        x += dx; y += dy; dist += step;
    }
    return dist;
}

// =================== Per-agent runtime state ===================
struct Runner {
    float y = 0.f;
    float vy = 0.f;
    bool  alive = true;
    float fitness_acc = 0.f;
};

// =================== Main ===================
int main() {
    // --- SDL core ---
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    TTF_Font* font = TTF_OpenFont("/Library/Fonts/Arial Unicode.ttf", 22);
    if (!font) { SDL_Log("TTF_OpenFont failed: %s", TTF_GetError()); return 1; }

    SDL_Window* window = SDL_CreateWindow("Neuro Ray Runner",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // --- Timing ---
    Uint64 prev = SDL_GetPerformanceCounter();
    const double freq = (double)SDL_GetPerformanceFrequency();

    // --- World / UI state ---
    int   W = 800, H = 600;
    float x = 240.f;         // fixed X for all agents
    int   score = 0, highScore = 0;
    float pxAcc = 0.f;

    // --- Population & runners ---
    std::mt19937 rng(std::random_device{}());
    vector<Agent> agents; agents.reserve(POP_SIZE);
    for (int i = 0; i < POP_SIZE; ++i) agents.emplace_back(NUM_RAYS + 2);
    vector<Runner> runners(agents.size());

    // --- Cave per generation (fair evaluation) ---
    int   generation = 1;
    float bestFitnessEver = 0.f;
    Cave  cave, baseCave;

    // --- Helper lambdas ---
    auto center_on_corridor = [&](float& yOut){
        float t,b; cave.sample(W,H,x,t,b);
        yOut = 0.5f * (t + b);
    };

    auto reset_generation = [&](){
        // If you can seed Cave deterministically, do it here for reproducibility
        baseCave = Cave();
        cave = baseCave;

        for (size_t i = 0; i < runners.size(); ++i) {
            runners[i] = Runner{};
            runners[i].alive = true;
            runners[i].vy = 0.f;
            float t,b; cave.sample(W,H,x,t,b);
            runners[i].y = 0.5f*(t+b);
            agents[i].fitness = 0.f;
        }

        score = 0; pxAcc = 0.f;
    };

    reset_generation();

    // --- Main loop ---
    bool running = true;
    while (running) {
        // Events
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT ||
               (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            }
        }

        // Window size (handle resizes)
        SDL_GetWindowSize(window, &W, &H);

        // Delta time (cap)
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = float(now - prev) / float(freq);
        if (dt > 0.033f) dt = 0.033f;
        prev = now;

        // Keep x’s corridor valid (on the very first frames / resizes)
        float topY0, botY0;
        cave.sample(W, H, x, topY0, botY0);

        // Update cave scroll once per frame
        cave.update(VX, dt);

        // Background
        SDL_SetRenderDrawColor(renderer, 17, 17, 17, 255);
        SDL_RenderClear(renderer);

        // Draw cave walls
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        for (int xp = 0; xp < W; ++xp) {
            float t,b; cave.sample(W,H,(float)xp,t,b);
            SDL_RenderDrawLineF(renderer, (float)xp, 0.f, (float)xp, t);
            SDL_RenderDrawLineF(renderer, (float)xp, b, (float)xp, (float)H);
        }

        // Manual override (SPACE makes you go up at max speed)
        const Uint8* ks = SDL_GetKeyboardState(nullptr);
        bool manual = ks[SDL_SCANCODE_SPACE];

        // Simulate all agents
        bool anyAlive = false;
        int  bestAliveIdx = -1; float bestAliveFit = -1e9f;

        for (size_t i = 0; i < agents.size(); ++i) {
            Runner& R = runners[i];
            if (!R.alive) continue;
            anyAlive = true;

            // ---- Sensing (forward-locked FOV for stability) ----
            float rayAngles[NUM_RAYS], rayDists[NUM_RAYS];
            const float angleForward = 0.0f; // straight ahead (world +X)
            for (int r = 0; r < NUM_RAYS; ++r) {
                float t = (NUM_RAYS == 1) ? 0.5f : (float)r / (float)(NUM_RAYS - 1);
                float a = angleForward - (RAY_FOV * 0.5f) + t * RAY_FOV;
                rayAngles[r] = a;
                rayDists[r]  = castRay(cave, W, H, x, R.y, a, RAY_MAX, RAY_STEP);
            }

            float topSense, botSense;
            cave.sample(W, H, x + 30.f, topSense, botSense);
            float center = 0.5f*(topSense + botSense);
            float halfGap = 0.5f*(botSense - topSense);
            float offSetNorm = (halfGap > 1.f) ? (R.y - center)/halfGap : 0.f;
            float velNorm = std::max(-1.f, std::min(1.f, R.vy / VY));

            vector<float> in; in.reserve(NUM_RAYS + 2);
            for (int r = 0; r < NUM_RAYS; ++r)
                in.push_back(std::min(1.f, rayDists[r]/RAY_MAX));
            in.push_back(offSetNorm);
            in.push_back(velNorm);

            float a = agents[i].brain.forward(in); // tanh ∈ [-1,1]

            // ---- Control (proportional velocity) ----
            if (manual) R.vy = VY;
            else        R.vy = a * VY;

            R.y -= R.vy * dt;
            if (R.y < ARROW_SIZE)     R.y = ARROW_SIZE;
            if (R.y > H - ARROW_SIZE) R.y = H - ARROW_SIZE;

            // ---- Fitness shaping (gentle) ----
            R.fitness_acc += dt * (1.0f - 0.1f * fabsf(offSetNorm) - 0.001f * a * a);

            // ---- Collision ----
            float t0, b0; cave.sample(W, H, x, t0, b0);
            if (R.y < t0 || R.y > b0) {
                R.alive = false;
                float survivalBonus = cave.scroll;
                agents[i].fitness = R.fitness_acc + survivalBonus + (float)score * 50.f;
            }

            // Track best alive (for highlight)
            if (R.alive && agents[i].fitness > bestAliveFit) {
                bestAliveFit = agents[i].fitness;
                bestAliveIdx = (int)i;
            }

            // Render this agent (dim)
            SDL_SetRenderDrawColor(renderer, 173, 26, 255, R.alive ? 120 : 40);
            float visAngle = atan2f(-R.vy, VX);
            drawArrow(renderer, x, R.y, visAngle, ARROW_SIZE * 0.9f);

        }

        // Highlight current best-alive (if any)
        if (bestAliveIdx >= 0) {
            Runner& R = runners[bestAliveIdx];
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            float visAngle = atan2f(-R.vy, VX);
            drawArrow(renderer, x, R.y, visAngle, ARROW_SIZE);
        }

        // Score (distance-based, shared)
        if (anyAlive) {
            pxAcc += VX * dt;
            while (pxAcc >= PIXELS_PER_POINT) { score += 1; pxAcc -= PIXELS_PER_POINT; }
        }

        // If everyone died -> evolve & new generation
        if (!anyAlive) {
            if (score > highScore) highScore = score;

            // Evolve population
            evolve(agents, rng, ELITE_COUNT, MUT_SIGMA, MUT_PROB, generation, bestFitnessEver);

            // Reset world & runners for the new generation
            reset_generation();
        }

        // HUD
        char line1[128], line2[128], line3[128];
        snprintf(line1, sizeof(line1), "Gen: %d", generation);
        snprintf(line2, sizeof(line2), "Score: %d   High: %d", score, highScore);
        snprintf(line3, sizeof(line3), "Pop: %zu  Elite: %d", agents.size(), ELITE_COUNT);

        drawText(renderer, font, line1, 16, 16);
        drawText(renderer, font, line2, 16, 42);
        drawText(renderer, font, line3, 16, 68);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
