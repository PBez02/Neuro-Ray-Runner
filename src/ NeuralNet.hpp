#pragma once
#include <vector>
#include <random>
#include <algorithm>
using namespace std;

// Simple feedforward neural network with step activation function
struct Neuron { // Single neuron
    vector<float> weights; // Flattened weight matrix

    int predict (const vector<float>& inputs) const{ // Forward pass to get prediction
        float s = 0.f; // Weighted sum
        const int n = (int)inputs.size(); // Number of inputs
        for (int i = 0; i < n; i++) { // Calculate weighted sum
            s += weights[i] * inputs[i]; // Weight * Input
        }
        s += weights.back(); // Bias term
        int out = 0; // Output after activation (step function)
        if (s > 0.f) { // Step activation
            out = 1; // Activated
        }
        else {
            out = 0; // Not activated
        }
        return out; // Return output
    }
};

struct Layer {
    vector<Neuron> neurons; // Neurons in this layer

    // Forward pass for float input
    vector<int> predictF(const vector<float>& inputF) const { // Forward pass for the layer
        vector<int> outputs; // Outputs of the layer
        outputs.reserve(neurons.size()); // Set's output size
        for (size_t i = 0; i < neurons.size(); i++) { // For each neuron
            outputs.push_back(neurons[i].predict(inputF)); // Get prediction 
        }
        return outputs; // Return all outputs
    }

    // Overloaded function to accept binary input
    vector<int> predict(const vector<int>& inputBin) const { // Forward pass for the layer
        vector<float> inputF(inputBin.size()); // Convert binary to float
        vector<int> outputs; // Outputs of the layer
        outputs.reserve(neurons.size()); // Set's output size
        for (size_t i = 0; i < neurons.size(); i++) { // For each neuron
            outputs.push_back(neurons[i].predict(inputF)); // Get prediction
        }
        return outputs; // Return all outputs
    }
};

struct Net {
    vector<Layer> layers; // Layers in the network

    explicit Net(const vector<int>& layerSizes, unsigned seed=1234) {
        mt19937 rng(seed); // Random number generator
        uniform_real_distribution<float> dist(-1.f, 1.f); // Weight initialization range
        layers.resize(layerSizes.size() - 1); // Number of layers
        for (size_t li = 0; li < layers.size(); li++) { // For each layer
            int inSize = layerSizes[li]; // Input size
            int outSize = layerSizes[li + 1]; // Output size
            layers[li].neurons.resize(outSize); // Resize neurons
            for (int j = 0; j < outSize; j++) { // For each neuron
                vector<float>& weights = layers[li].neurons[j].weights; // Reference to weights
                weights.resize(inSize + 1); // +1 for bias
                for (int k = 0; k < inSize + 1; k++) { // Initialize weights
                    weights[k] = dist(rng); // Random weight
                }
            }
        }
    }

    bool predict(vector<float> input) const { // Forward pass through the network
        vector<int> act = layers[0].predictF(input); // First layer prediction
        for (size_t li = 1; li < layers.size(); li++) { // For each layer
            act = layers[li].predict(act); // Get next layer's activations
        }

        if (act.empty()) { // Safety check
            return false; // No output
        }
        if (act[0] > 0) { // Final output check
            return true; // Positive class
        }
        return false; // Negative class
    }
};