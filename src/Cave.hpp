#pragma once
#include <SDL2/SDL.h>
#include <cmath>
#include <algorithm>
#include <random>

// Cave structure to represent cave parameters and update logic
struct Cave {

    // Cave path parameters
    float baseGap = 180.f; // Gap size between top and bottom
    float gapJitter = 60.f; // Amplitude of gap size variation
    float gapFreq = 0.0025f; // Frequency of gap size variation

    float pathAmp = 120.f; // Amplitude of cave walls
    float pathFreq; // Frequency of cave walls
    
    float pathAmp2 = 60.f; // Secondary amplitude
    float pathFreq2 = 0.005f; // Secondary frequency
    float pathPhase = 1.3f; // Phase shift for secondary wave

    // Cave boundaries
    float margin = 30.f; // Margin from top and bottom
    float scroll = 0.f; // Horizontal scroll position

    // 
    float startPhase = 0.f; // Starting phase for cave generation

    // Constructor to initialize cave parameters
    Cave() {
        // Randomly initialize path frequency
        Uint64 seed = SDL_GetPerformanceCounter(); // Use performance counter as seed
        std::mt19937 rng(static_cast<unsigned int>(seed));

        // Random distributions for cave parameters
        std::uniform_real_distribution<float> dist(-0.0005f, 0.0005f);
        pathFreq = dist(rng);

        // Randomly initialize start phase
        std::uniform_real_distribution<float> distPhase(10000.f, 30000.f);
        startPhase = distPhase(rng);
    }
    Cave(float g, float a, float f) {
        baseGap = g;
        pathAmp = a;
        pathFreq = f;
        // Randomly initialize path frequency
        Uint64 seed = SDL_GetPerformanceCounter(); // Use performance counter as seed
        std::mt19937 rng(static_cast<unsigned int>(seed));
        
    }

    // Update cave scroll based on horizontal velocity and delta time
    void update(float vx, float deltaTime) {
        scroll += vx * deltaTime; // Update scroll based on horizontal velocity
    }

    // Sample the cave at a given x position to get top and bottom Y coordinates
    void sample(int W, int H, float x, float& topY, float& botY) const {
        const float xWorld = scroll + x + startPhase; // World X position
    
        float localFreq = pathFreq + 0.0005f * sin(0.0003f * xWorld);
        // Compute sine waves for cave path
        float s1 = sin(localFreq * xWorld);
        float s2 = sin(pathFreq2 * xWorld + pathPhase);

        // Compute center Y position of the cave
        const float center = H * 0.5f + (s1 * pathAmp + s2 * pathAmp2);
    
        // Compute the gap size with jitter
        float gap = baseGap + gapJitter * std::sin(gapFreq * xWorld + 2.0f);
        if (gap < 80.f) {
            gap = 80.f; // safety floor
        }
    
        // Compute top and bottom Y positions
        float top = center - gap * 0.5f;
        float bot = center + gap * 0.5f;
    
        // Apply margins to top and bottom 
        top = std::max(margin, top);
        bot = std::min((float)H - margin, bot);
    
        // Output the sampled top and bottom Y positions
        topY = top; 
        botY = bot;
    }
};