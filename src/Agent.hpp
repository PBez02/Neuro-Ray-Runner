#pragma once
#include <vector>
#include <algorithm>
#include <random>
#include "NeuralNet.hpp"
#include "Cave.hpp"

using std::vector;

// Agent structure representing an AI agent with a neural network brain and fitness score
struct Agent {
    Net brain;
    float fitness;
    explicit Agent(int inputCount) 
        : brain(vector<int>{inputCount, 16, 8, 1}), // Deeper network
          fitness(0.f) {}
    Agent() : brain(vector<int>{9, 16, 8, 1}), fitness(0.f) {}
};

// Reset the game state for the agent
inline void reset_run(float &y, float ARROW_SIZE, int H, float& pxAcc, int& score, Cave& cave, float& vy) {
    score = 0; // Reset score
    pxAcc = 0.0f; // Reset distance
    y = H * 0.5f; // Reset player position
    cave.scroll = 0.f; // Reset cave scroll
    cave = Cave(); // Create new cave
    vy = 0.f; // Reset vertical velocity
}

inline void evolve(vector<Agent>& agents, std::mt19937& rng, int eliteCount, 
        float mutationSigma, float mutationProb, int& generation, 
        float& bestFitness) {

    // Sort by fitness (best first)
    sort(agents.begin(), agents.end(), [](const Agent& a, const Agent& b) {
        return a.fitness > b.fitness;
    });

    // Update best fitness
    if (agents[0].fitness > bestFitness) {
        bestFitness = agents[0].fitness;
    }

    // Calculate statistics
    float avgFitness = 0.0f;
    for (const auto& a : agents) {
    avgFitness += a.fitness;
    }
    avgFitness /= agents.size();

    SDL_Log("Gen %d: Best=%.1f, Avg=%.1f, Worst=%.1f", 
    generation, agents[0].fitness, avgFitness, agents.back().fitness);

    vector<Agent> newAgents;
    newAgents.reserve(agents.size());

    // Elitism: preserve top agents
    for (int i = 0; i < eliteCount && i < (int)agents.size(); ++i) {
        Agent elite(agents[i].brain.layers[0].neurons[0].weights.size() - 1);
        elite.brain.copyWeightsFrom(agents[i].brain);
        elite.fitness = 0.f;
        newAgents.push_back(elite);
    }

    // FITNESS-PROPORTIONATE SELECTION (Roulette Wheel)
    // Calculate fitness sum for top 50% (ignore worst performers)
    int topHalf = agents.size() / 2;
    float fitnessSum = 0.0f;
    for (int i = 0; i < topHalf; ++i) {
        fitnessSum += std::max(0.0f, agents[i].fitness); // Avoid negative fitness
    }

    std::uniform_real_distribution<float> dist(0.0f, fitnessSum);

    // Fill rest with mutated offspring
    while (newAgents.size() < agents.size()) {
        // Select parent based on fitness
        int parentIdx = 0;
        if (fitnessSum > 0.0f) {
            float pick = dist(rng);
            float cumulative = 0.0f;

            for (int i = 0; i < topHalf; ++i) {
            cumulative += std::max(0.0f, agents[i].fitness);
                if (cumulative >= pick) {
                   parentIdx = i;
                    break;
                }
            }
        } else {
            // Fallback if all fitness is 0
            parentIdx = rng() % eliteCount;
        }

        Agent child(agents[parentIdx].brain.layers[0].neurons[0].weights.size() - 1);
        child.brain.copyWeightsFrom(agents[parentIdx].brain);
        child.brain.mutate(rng, mutationSigma, mutationProb);
        child.fitness = 0.f;
        newAgents.push_back(child);
    }

    agents = std::move(newAgents);
    generation++;
}