Neuro Ray Runner

Demo: https://www.youtube.com/watch?v=vH2sczdZDN8

Overview:
Neuro Ray Runner is a real-time simulation where a small population of agents are controlled by neural networks.
They learn to navigate an infinite scrolling cave using ray-based perception, continuous control, and evolutionary optimisation.

Each agent receives information about its environment by casting forward-facing rays. Based on this sensory input, the neural network outputs a continuous control signal that determines whether the agent moves up or down.

At the end of each generation, the top-performing agents are selected, replicated, and mutated with small perturbations to their network parameters, allowing the population to improve over time through elite selection.

Key Features:
Ray-Based Perception:
- Each agent casts 7 forward rays within a fixed field of view
- Rays detect distances from the agent to the caves boundaries
- Distance across all rays is then used to make the decision wether to move up or down

Neural Network:
- Fully connected feedforward network
- Usage of activation function (tanh)
- Continuous output controls vertical velocity

Network inputs:
- 7 normalised ray distances
- Normalised vertical offset from cave center
- Normalised vertical velocity

Evolutionary Learning:
- Population size: 50 agents
- Optimisation through neuroevolution
- Elitism: Top 5 agents are preserved and unchanged
- Gaussian mutation applied to offspring weights
- Fitness components: 
    - Survival duration
    - Distance travelled
    - Stability and smoothness of movement
    - Corridor-centering behavior

Procedural Environment:
- Infinite horizontally scrolling cave
- Dynamically varying cave curvutare and width
- Cave regenerated per generation
- Deterministic simulation step with capped delta-time

Real-Time Visulation:
- Built using SDL2 and SDL_ttf
- Agents rendered as arrows
- Best alive agent highlighted in real time (white color)
- Live HUD display:
    - Generation number
    - Current score
    - High score
    - Population and elite count

Controls:
- ESC: Quit simulation
- Space: Manual override (used for debugging)

Build and Run (macOS):
- Dependencies:
Install via Homebrew:

brew install sdl2 sdl2_ttf

- Compile:

From the src directory:

clang++ *.cpp \
  -std=c++17 \
  -I/opt/homebrew/include \
  -L/opt/homebrew/lib \
  -lSDL2 -lSDL2_ttf \
  -O2 \
  -o neuro_ray_runner

- Run:
./neuro_ray_runner

Build & Run (Windows):
- Dependencies

Install MSYS2 (provides g++ + SDL2):

https://www.msys2.org/

Open MSYS2 MinGW64 terminal and install dependencies:

pacman -S mingw-w64-x86_64-toolchain \
          mingw-w64-x86_64-SDL2 \
          mingw-w64-x86_64-SDL2_ttf

- Compile:

From the src directory (inside MSYS2 MinGW64):

g++ *.cpp \
  -std=c++17 \
  -O2 \
  -lSDL2 -lSDL2_ttf \
  -o neuro_ray_runner.exe

- Run:
./neuro_ray_runner.exe

Project Structure:
src/
├── main.cpp              # Simulation loop & rendering
├── Agent.hpp             # Agent definition & evolution logic
├── NeuralNet.hpp         # Neural network implementation
├── Cave.hpp              # Cave structure


This project demonstrates:
- Low-level C++ proficiency (memory layout, performance, real-time loops)
- Understanding of learning systems beyond black-box libraries
- Ability to design evaluation-stable simulations
- Strong intuition for control systems and optimisation dynamics

The techniques used here parallel those found in:
- Algorithmic trading simulations
- Reinforcement learning research environments
- Autonomous navigation systems
- Game AI and agent-based modelling

