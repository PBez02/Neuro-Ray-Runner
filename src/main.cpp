#include <SDL2/SDL.h>

int main() {
    SDL_Init(SDL_INIT_VIDEO); // Initialize SDL2
    // Create an application window with the following settings:
    SDL_Window* window = SDL_CreateWindow("Neuro Ray Runner", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);

    // Create a boolean variable to control the loop
    // and an SDL_Event variable to handle events
    bool running = true;
    SDL_Event event;

    // Main loop
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                // Exit the loop if the window is closed or ESC is pressed
                running = false;
            }
        } 
        SDL_Delay(16); // Roughly 60 FPS  
    }
    // Clean up and quit SDL2
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}