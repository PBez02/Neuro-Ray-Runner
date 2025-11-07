#include <SDL2/SDL.h>
#include <cmath>
#include "Cave.hpp"
using namespace std;

// Function to draw an arrow at (x, y) with given angle and size
static void drawArrow(SDL_Renderer* renderer, float x, float y, float angle, float size) {
    float half = size * 0.5f; // Half size for calculations

    SDL_FPoint p1 = {x + size, y}; // Tip of the arrow
    SDL_FPoint p2 = {x - half, y - half}; // Bottom left
    SDL_FPoint p3 = {x - half, y + half}; // Bottom right
    
    auto rot = [&](SDL_FPoint& p) { // Rotate point around (x, y) using standard 2d rotation formula
        float s = sin(angle); // Sine of angle
        float c = cos(angle); // Cosine of angle
        float px = p.x - x; // Translate point to origin
        float py = p.y - y; // Translate point to origin
        SDL_FPoint out {px*c - py*s + x, px*s + py*c + y}; // Rotate and translate back
        return out; // Return rotated point
    };
    p1 = rot(p1); // Rotate all points
    p2 = rot(p2); 
    p3 = rot(p3); 

    SDL_RenderDrawLineF(renderer, p1.x, p1.y, p2.x, p2.y); // Draw the arrow
    SDL_RenderDrawLineF(renderer, p2.x, p2.y, p3.x, p3.y); 
    SDL_RenderDrawLineF(renderer, p3.x, p3.y, p1.x, p1.y);
}

int main() {
    SDL_Init(SDL_INIT_VIDEO); // Initialize SDL2
    // Create an application window with the following settings:
    SDL_Window* window = SDL_CreateWindow("Neuro Ray Runner", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);

    // Create a renderer for the window
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Create cave instance
    Cave cave;

    // Variables for frame timing
    Uint64 prev = SDL_GetPerformanceCounter(); // Previous performance counter
    const double freq = (double)SDL_GetPerformanceFrequency(); // Performance frequency

    // Player variables
    float x = 120.f; 
    float y = 300.f; // Positions
    float vy = 0.f; // Vertical velocity
    float vx = 220.f; // Horizontal velocity

    const float ACC_UP = 800.f; // Upward acceleration
    const float ACC_DOWN = -600.f; // Downward acceleration
    const float MAXV = 900.f; // Maximum vertical velocity
    const float ARROW_SIZE = 20.f; // Size of the arrow

    // Create a boolean variable to control the loop
    bool running = true;

    // Main loop
    while (running) {
        SDL_Event event; // Event variable
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                // Exit the loop if the window is closed or ESC is pressed
                running = false;
            }
        } 

        // Calculate delta time
        Uint64 now = SDL_GetPerformanceCounter(); // Get the current performance counter
        float deltaTime = (now - prev) / freq; // Calculate delta time
        if (deltaTime > 0.033f) {
            deltaTime = 0.033f; // Cap delta time to avoid large jumps
        }
        prev = now; // Update previous counter

        // Handle keyboard input
        const Uint8* keystate = SDL_GetKeyboardState(nullptr); // Get the current state of the keyboard
        bool space = keystate[SDL_SCANCODE_SPACE]; // Check if spacebar is pressed

        // Update player physics
        float ay;
        if (space) {
            ay = ACC_UP; // Upward acceleration when space is pressed
        } else {
            ay = ACC_DOWN; // Downward acceleration otherwise
        }

        vy += ay * deltaTime; // Update vertical velocity
        if (vy > MAXV) {
            vy = MAXV; // Cap maximum downward velocity
        }
        if (vy < -MAXV) {
            vy = -MAXV; // Cap maximum upward velocity
        }
        y -= vy * deltaTime; // Update vertical position
        x += vx * deltaTime; // Update horizontal position

        cave.update(vx, deltaTime); // Update cave scroll

        // Bounds checking
        int W;
        int H;
        SDL_GetWindowSize(window, &W, &H); // Get window size

        SDL_SetRenderDrawColor(renderer, 17, 17, 17, 255); // Black Background
        SDL_RenderClear(renderer); // Clear the screen

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green cave walls
        for (int xPix = 0; xPix < W; xPix++) { // Draw cave walls
            float topY; 
            float botY; 
            cave.sample(W, H, xPix, topY, botY); // Sample cave at this x position
            SDL_RenderDrawLineF(renderer, xPix, 0.f, xPix, topY); // Top wall
            SDL_RenderDrawLineF(renderer, xPix, botY, xPix, (float)H); // Bottom wall
        }
        if (y < ARROW_SIZE) {
            y = ARROW_SIZE; // Top boundary
            vy = 0.f; // Stop upward movement
        }
        if (y > H - ARROW_SIZE) {
            y = H - ARROW_SIZE; // Bottom boundary
            vy = 0.f; // Stop downward movement
        }
        if (x > W + ARROW_SIZE) {
            x = -ARROW_SIZE; // Wrap around horizontally
        }

        float angle = atan2(-vy, vx); // Calculate angle based on velocity

        SDL_SetRenderDrawColor(renderer, 255,255,255,255); // White
        drawArrow(renderer, x, y, angle, ARROW_SIZE); // Draw the player arrow
        SDL_RenderPresent(renderer); // Update the screen
        SDL_Delay(16); // Roughly 60 FPS  

    }
    // Clean up and quit SDL2
    SDL_DestroyRenderer(renderer); // Destroy the renderer
    SDL_DestroyWindow(window); // Destroy the window
    SDL_Quit(); // Quit SDL2
    return 0; // Exit the program
}