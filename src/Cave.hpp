#pragma once
#include <SDL2/SDL.h>
#include <cmath>
using namespace std;

// Cave structure to represent cave parameters and update logic
struct Cave {
    float gap; // Gap size between top and bottom
    float amp; // Amplitude of cave walls
    float freq; // Frequency of cave walls
    float scroll; // Horizontal scroll position

    // Constructor to initialize cave parameters
    Cave(float g=180.f, float a=120.f, float f=0.0025f) : gap(g), amp(a), freq(f), scroll(0.f) {}

    // Update cave scroll based on horizontal velocity and delta time
    void update(float vx, float deltaTime) {
        scroll += vx * deltaTime; // Update scroll based on horizontal velocity
    }

    void sample(int W, int H, float x, float& topY, float& botY) const {
        float center = H * 0.5f + amp * sin(freq * (scroll + x)); // Calculate center Y position
        topY = center - gap * 0.5f; // Top Y position
        botY = center + gap * 0.5f; // Bottom Y position
    }
};