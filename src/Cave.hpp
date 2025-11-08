#pragma once
#include <SDL2/SDL.h>
#include <cmath>
#include <algorithm>

// Cave structure to represent cave parameters and update logic
struct Cave {
    float baseGap = 180.f; // Gap size between top and bottom
    float pathAmp = 120.f; // Amplitude of cave walls
    float pathFreq = 0.0025f; // Frequency of cave walls
    float margin = 30.f; // Margin from top and bottom
    float scroll = 0.f; // Horizontal scroll position
    float gapJitter = 60.f; // Amplitude of gap size variation
    float gapFreq = 0.0025f; // Frequency of gap size variation

    // Constructor to initialize cave parameters
    Cave() = default;
    Cave(float g, float a, float f) {
        baseGap = g;
        pathAmp = a;
        pathFreq = f;
    }

    // Update cave scroll based on horizontal velocity and delta time
    void update(float vx, float deltaTime) {
        scroll += vx * deltaTime; // Update scroll based on horizontal velocity
    }

    void sample(int W, int H, float x, float& topY, float& botY) const {
        const float xWorld = scroll + x;
    
        // Center path (new)
        const float center = H * 0.5f + pathAmp * std::sin(pathFreq * xWorld);
    
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
        topY = top; botY = bot;
    }
};