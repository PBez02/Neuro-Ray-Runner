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

// == Constants ==
static constexpr int   NUM_RAYS = 7; // number of rays cast for sensing
static constexpr float RAY_FOV  = 1.2f; // total FOV in radians
static constexpr float RAY_MAX  = 700.f; // max ray distance
static constexpr float RAY_STEP = 4.f; // ray marching step size

static const int   POP_SIZE    = 50; // population size
static const int   ELITE_COUNT = 5; // number of elite agents preserved each generation
static const float MUT_PROB    = 0.15f; // mutation probability
static const float MUT_SIGMA   = 0.30f; // mutation strength

static const float VX = 220.f;  // horizontal scroll speed (px/s)
static const float VY = 200.f;  // max vertical speed (px/s)

static const float PIXELS_PER_POINT = 50.f; // scoring resolution
static const float ARROW_SIZE       = 20.f; // size of the agent arrow

// == Drawing helpers ==
static void drawText(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y) {
    SDL_Color color = {255, 255, 255, 255}; // white color
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color); // create surface
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface); // create texture
    SDL_Rect dest = {x, y, surface->w, surface->h}; // destination rectangle
    SDL_FreeSurface(surface); // free the surface
    SDL_RenderCopy(renderer, texture, nullptr, &dest); // render the texture
    SDL_DestroyTexture(texture); // destroy the texture
}

static void drawArrow(SDL_Renderer* renderer, float x, float y, float angle, float size) {
    float half = size * 0.5f; // half size

    SDL_FPoint p1 = {x + size, y}; // tip point
    SDL_FPoint p2 = {x - half, y - half}; // bottom rear point
    SDL_FPoint p3 = {x - half, y + half}; // top rear point

    auto rot = [&](SDL_FPoint p) {
        float s = sinf(angle), c = cosf(angle); // sine and cosine of angle
        float px = p.x - x, py = p.y - y; // translate to origin
        return SDL_FPoint{px * c - py * s + x, px * s + py * c + y}; // rotate and translate back
    };
    p1 = rot(p1); p2 = rot(p2); p3 = rot(p3); // rotate all points

    Uint8 r,g,b,a; SDL_GetRenderDrawColor(renderer,&r,&g,&b,&a); // get current color
    SDL_Color col = {r,g,b,a}; // create SDL_Color

    SDL_Vertex vtx[3]; // define vertices
    vtx[0].position = p1; vtx[0].color = col; // tip
    vtx[1].position = p2; vtx[1].color = col; // bottom rear
    vtx[2].position = p3; vtx[2].color = col; // top rear

    SDL_RenderGeometry(renderer, nullptr, vtx, 3, nullptr, 0); // render triangle, fill with color instead of hollow triangle
}

// == Ray casting function ==
static float castRay(const Cave& cave, int W, int H, float ox, float oy,
                     float angle, float maxDist = RAY_MAX, float step = RAY_STEP) {
    float dx = cosf(angle) * step; // delta x per step
    float dy = sinf(angle) * step; // delta y per step
    float x = ox, y = oy, dist = 0.f; // initialize position and distance
    while (dist < maxDist) { // while within max distance
        float topY, botY; // cave boundaries
        cave.sample(W, H, x, topY, botY); // sample cave at current x
        if (y < topY || y > botY) break; // collision detected
        x += dx; y += dy; dist += step; // advance ray
    }
    return dist; // return distance traveled
}

//  == Runner structure ==
struct Runner {
    float y = 0.f; // vertical position
    float vy = 0.f; // vertical velocity
    bool  alive = true; // alive status
    float fitness_acc = 0.f; // accumulated fitness
};

// == Main function ==
int main() {
    // -- SDL & TTF init ---
    SDL_Init(SDL_INIT_VIDEO); // initialize SDL
    TTF_Init(); // initialize TTF
    TTF_Font* font = TTF_OpenFont("/Library/Fonts/Arial Unicode.ttf", 22); // load font
    if (!font) { SDL_Log("TTF_OpenFont failed: %s", TTF_GetError()); return 1; } // error check

    SDL_Window* window = SDL_CreateWindow("Neuro Ray Runner", // window title
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN); // create window
    SDL_Renderer* renderer = SDL_CreateRenderer( // create renderer
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); // with vsync

    // -- Timing --
    Uint64 prev = SDL_GetPerformanceCounter(); // previous timestamp
    const double freq = (double)SDL_GetPerformanceFrequency(); // performance frequency

    // -- World & UI state --
    int   W = 800, H = 600; // window size
    float x = 240.f; // fixed X for all agents
    int   score = 0, highScore = 0; // current and high score
    float pxAcc = 0.f; // pixel accumulator for scoring

    // -- Population & agents --
    std::mt19937 rng(std::random_device{}()); // random number generator
    vector<Agent> agents; agents.reserve(POP_SIZE); // agent population
    for (int i = 0; i < POP_SIZE; ++i) { // for each agent
        agents.emplace_back(NUM_RAYS + 2); // create agents
    }
    vector<Runner> runners(agents.size()); // corresponding runners

    // -- Cave per generation --
    int   generation = 1; // generation counter
    float bestFitnessEver = 0.f; // best fitness ever
    Cave  cave, baseCave; // cave instances

    // -- Helper lambdas --
    auto center_on_corridor = [&](float& yOut){ // center y on cave corridor
        float t,b; cave.sample(W,H,x,t,b); // sample cave
        yOut = 0.5f * (t + b); // set y to center
    };

    // -- Reset the entire generation --
    auto reset_generation = [&](){
        // If you can seed Cave deterministically, do it here for reproducibility
        baseCave = Cave(); // create new base cave
        cave = baseCave; // assign to current cave

        for (size_t i = 0; i < runners.size(); ++i) { // for each runner
            runners[i] = Runner{}; // reset runner
            runners[i].alive = true; // set alive
            runners[i].vy = 0.f; // reset vertical velocity
            float t,b; cave.sample(W,H,x,t,b); // sample cave
            runners[i].y = 0.5f*(t+b); // center on corridor
            agents[i].fitness = 0.f; // reset fitness
        }

        score = 0; pxAcc = 0.f; // reset score
    };

    reset_generation(); // initial generation reset

    // -- Main loop --
    bool running = true; // running flag
    while (running) { // main loop
        // Events
        SDL_Event e; // event variable
        while (SDL_PollEvent(&e)) { // poll events
            if (e.type == SDL_QUIT || // quit event
               (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) { // escape key
                running = false; // stop running
            }
        }

        // Window size (handle resizes)
        SDL_GetWindowSize(window, &W, &H); // get window size

        // Delta time (cap)
        Uint64 now = SDL_GetPerformanceCounter(); // current timestamp
        float dt = float(now - prev) / float(freq); // delta time in seconds
        if (dt > 0.033f) dt = 0.033f; // cap dt to ~30 FPS
        prev = now; // update previous timestamp

        // Keep x’s corridor valid (on the very first frames / resizes)
        float topY0, botY0; // cave boundaries at agent x
        cave.sample(W, H, x, topY0, botY0); // sample cave

        // Update cave scroll once per frame
        cave.update(VX, dt); // update cave scroll

        // Background
        SDL_SetRenderDrawColor(renderer, 17, 17, 17, 255); // dark background
        SDL_RenderClear(renderer); // clear screen

        // Draw cave walls
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // green walls
        for (int xp = 0; xp < W; ++xp) { // for each x pixel
            float t,b; cave.sample(W,H,(float)xp,t,b); // sample cave
            SDL_RenderDrawLineF(renderer, (float)xp, 0.f, (float)xp, t); // top wall
            SDL_RenderDrawLineF(renderer, (float)xp, b, (float)xp, (float)H); // bottom wall
        }

        // Manual override (SPACE makes you go up at max speed)
        const Uint8* ks = SDL_GetKeyboardState(nullptr); // keyboard state
        bool manual = ks[SDL_SCANCODE_SPACE]; // space key for manual control

        // Simulate all agents
        bool anyAlive = false; // any alive flag
        int  bestAliveIdx = -1; float bestAliveFit = -1e9f; // best alive tracking

        for (size_t i = 0; i < agents.size(); ++i) { // for each agent
            Runner& R = runners[i]; // corresponding runner
            if (!R.alive) continue; // skip if dead
            anyAlive = true; // at least one alive

            // -- Sensing (forward-locked FOV for stability) --
            float rayAngles[NUM_RAYS], rayDists[NUM_RAYS]; // ray data
            const float angleForward = 0.0f; // straight ahead (world +X)
            for (int r = 0; r < NUM_RAYS; ++r) { // for each ray
                float t = (NUM_RAYS == 1) ? 0.5f : (float)r / (float)(NUM_RAYS - 1); // normalized [0,1]
                float a = angleForward - (RAY_FOV * 0.5f) + t * RAY_FOV; // ray angle
                rayAngles[r] = a; // store angle
                rayDists[r]  = castRay(cave, W, H, x, R.y, a, RAY_MAX, RAY_STEP); // cast ray
            }

            // -- Neural Network Decision --
            float topSense, botSense; // cave boundaries for sensing
            cave.sample(W, H, x + 30.f, topSense, botSense); // sample slightly ahead
            float center = 0.5f*(topSense + botSense); // cave center
            float halfGap = 0.5f*(botSense - topSense); // half gap size
            float offSetNorm = (halfGap > 1.f) ? (R.y - center)/halfGap : 0.f; // normalized offset
            float velNorm = std::max(-1.f, std::min(1.f, R.vy / VY)); // normalized velocity

            vector<float> in; in.reserve(NUM_RAYS + 2); // NN input vector
            for (int r = 0; r < NUM_RAYS; ++r){ // for each ray
                in.push_back(std::min(1.f, rayDists[r]/RAY_MAX)); // normalized distance
            }
            in.push_back(offSetNorm); // normalized offset
            in.push_back(velNorm); // normalized velocity

            float a = agents[i].brain.forward(in); // tanh ∈ [-1,1]

            // -- Control (proportional velocity) --
            if (manual) {
                R.vy = VY; // max upward speed
            }
            else { 
                R.vy = a * VY; // set vertical speed
            }
            R.y -= R.vy * dt; // update vertical position
            if (R.y < ARROW_SIZE) {
                R.y = ARROW_SIZE; // top boundary
            }
            if (R.y > H - ARROW_SIZE) {
                R.y = H - ARROW_SIZE; // bottom boundary
            }

            // -- Fitness shaping (gentle) --
            R.fitness_acc += dt * (1.0f - 0.1f * fabsf(offSetNorm) - 0.001f * a * a); // reward center and smoothness

            // -- Collision --
            float t0, b0; cave.sample(W, H, x, t0, b0); // sample cave at agent x
            if (R.y < t0 || R.y > b0) { // collision check
                R.alive = false; // mark as dead
                float survivalBonus = cave.scroll; // survival bonus based on distance
                agents[i].fitness = R.fitness_acc + survivalBonus + (float)score * 50.f; // total fitness
            }

            // Track best alive (for highlight)
            if (R.alive && agents[i].fitness > bestAliveFit) { // better than current best
                bestAliveFit = agents[i].fitness; // update best fitness
                bestAliveIdx = (int)i; // update best index
            }

            // Render this agent (dim)
            SDL_SetRenderDrawColor(renderer, 173, 26, 255, R.alive ? 120 : 40); // purple, dim if dead
            float visAngle = atan2f(-R.vy, VX); // visual angle based on velocity
            drawArrow(renderer, x, R.y, visAngle, ARROW_SIZE * 0.9f); // draw agent

        }

        // Highlight current best-alive (if any)
        if (bestAliveIdx >= 0) {
            Runner& R = runners[bestAliveIdx]; // best runner
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // white color
            float visAngle = atan2f(-R.vy, VX); // visual angle
            drawArrow(renderer, x, R.y, visAngle, ARROW_SIZE); // draw highlighted agent
        }

        // Score (distance-based, shared)
        if (anyAlive) { // if at least one alive
            pxAcc += VX * dt; // accumulate pixels
            while (pxAcc >= PIXELS_PER_POINT) { score += 1; pxAcc -= PIXELS_PER_POINT; } // update score
        }

        // If everyone died -> evolve & new generation
        if (!anyAlive) {
            if (score > highScore) highScore = score; // update high score

            // Evolve population
            evolve(agents, rng, ELITE_COUNT, MUT_SIGMA, MUT_PROB, generation, bestFitnessEver); // evolve agents

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
