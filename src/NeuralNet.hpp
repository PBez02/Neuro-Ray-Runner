#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>

using std::vector;
using std::mt19937;
using std::uniform_real_distribution;
using std::normal_distribution;

// == Nueron ==
struct Neuron { // Single neuron
    vector<float> weights; // Flattened weight matrix

    static inline float act(float x) {
        return tanh(x); // Activation function (tanh)
    }

    float forward (const vector<float>& inputs) const {
        float s = weights.back(); // Bias term
        const int n = (int)inputs.size();
        for (int i = 0; i < n; i++) {
            s += weights[i] * inputs[i]; // Weighted sum
        }
        return act(s); // Apply activation function
    }
};

// == Layer of neurons ==
struct Layer {
    vector<Neuron> neurons; // Neurons in this layer

    vector<float> forward(const vector<float>& inputs) const {
        vector<float> outputs; // Output activations
        outputs.reserve(neurons.size());
        for (const Neuron& neuron: neurons) {
            outputs.push_back(neuron.forward(inputs)); // Forward pass for each neuron
        }
        return outputs;
    }
};

// == Neural Network ==
struct Net {
    vector<Layer> layers; // Layers in the network

    explicit Net(const vector<int>& layerSizes, unsigned seed = std::random_device{}()) {
        mt19937 rng(seed); // Random number generator
        uniform_real_distribution<float> dist(-0.5f, 0.5f); // Weight initialization range

        layers.resize(layerSizes.size() - 1); // Number of layers excluding input layer

        for (size_t l = 0; l < layerSizes.size() - 1; l++) {
            const int inSize = layerSizes[l]; // Number of inputs to this layer
            const int outSize = layerSizes[l + 1]; // Number of neurons in this layer
            Layer L; // Current layer
            L.neurons.resize(outSize); // Resize neurons

            for (int n = 0; n < outSize; n++) { // For each neuron
                auto& neuron = L.neurons[n].weights; // Current neuron
                neuron.resize(inSize + 1); // +1 for bias
                for (int k = 0; k < inSize + 1; k++) { // For each weight
                    neuron[k] = dist(rng); // Initialize weights randomly
                }
            }
            layers[l] = L; // Assign layer
        }
    }

    // -- Forward pass through the network --
    float forward(vector<float> x) const { // Input vector
        for (const Layer& layer : layers) { // For each layer
            x = layer.forward(x); // Forward pass through each layer
        }
        if (x.empty()) { 
            return 0.f; // No output
        }
        return x[0]; // Return first output
    }

    // -- Decision based on output --
    bool upDecision(const vector<float>& x) const {
        return forward(x) > 0.f; // Decision based on output
    }

    // -- Mutation for evolutionary strategies --
    void mutate(std::mt19937& rng, float sigma, float prob) {
        normal_distribution<float> dist(0.f, sigma); // Gaussian distribution for mutation
        uniform_real_distribution<float> probDist(0.0f, 1.0f); // Uniform distribution for mutation probability
        
        for (size_t l = 0; l < layers.size(); l++) { // For each layer
            Layer& L = layers[l];
            for (size_t n = 0; n < L.neurons.size(); n++) { // For each neuron
                Neuron& neuron = L.neurons[n];
                for (size_t k = 0; k < neuron.weights.size(); k++) { // For each weight
                    if (probDist(rng) < prob) { // Mutate with given probability
                        neuron.weights[k] += dist(rng); // Add Gaussian noise
                    }
                }
            }
        }
    }

    // -- Copy weights from another network --
    void copyWeightsFrom(const Net& other) {
        layers.resize(other.layers.size()); // Resize layers
        for (size_t l = 0; l < other.layers.size(); l++) { // For each layer
            const Layer& otherLayer = other.layers[l];
            Layer& layer = layers[l];
            layer.neurons.resize(otherLayer.neurons.size()); // Resize neurons
            for (size_t n = 0; n < otherLayer.neurons.size(); n++) { // For each neuron
                const Neuron& otherNeuron = otherLayer.neurons[n]; // Get neuron from other network
                Neuron& neuron = layer.neurons[n]; // Current neuron
                neuron.weights = otherNeuron.weights; // Copy weights
            }
        }
    }
};