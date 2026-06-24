#ifndef NET_H
#define NET_H

#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <fstream>

using namespace std;

class Neuron {
public:
    vector<float> Weights;
    float bias;

    Neuron() {}

    Neuron(const int& n,  mt19937& rng, uniform_real_distribution<float>& dist) {
        Weights.resize(n);
        SetRandomInitialWeights(rng, dist);
        bias = dist(rng);
    }

    Neuron(const vector<float>& w, const float& b) : Weights(w), bias(b) {}

    float Compute(const vector<float>& inputs) const;

private:
    void SetRandomInitialWeights(mt19937& rng, uniform_real_distribution<float>& dist);
};

struct Network {
    vector<Neuron> Filters;
    vector<Neuron> Filters2;
    vector<Neuron> DenseNeurons;
    vector<Neuron> DenseNeurons2;

    Network(mt19937& rng, uniform_real_distribution<float>& dist) {
        for (size_t i = 0; i < 32; i++) {
            Filters.push_back(Neuron(9, rng, dist));
        }
        for (size_t i = 0; i < 32; i++) {
            Filters2.push_back(Neuron(9 * 32, rng, dist));
        }
        for (size_t i = 0; i < 128; i++) {
            DenseNeurons.push_back(Neuron(1152, rng, dist));
        }
        for (size_t i = 0; i < 26; i++) {
            DenseNeurons2.push_back(Neuron(128, rng, dist));
        }
    }

    Network(const string& fileName) {
        ifstream file(fileName);
        if (!file.is_open()) throw std::runtime_error("Couldn't open " + fileName);

        vector<float> weights;
        float value, bias;

        for (size_t i = 0; i < 32; i++) {
            for (size_t j = 0; j < 9; j++) {
                file >> value;
                weights.push_back(value);
            }
            file >> bias;
            Filters.push_back(Neuron(weights, bias));
            weights.clear();
        }

        for (size_t i = 0; i < 32; i++) {
            for (size_t j = 0; j < 288; j++) {
                file >> value;
                weights.push_back(value);
            }
            file >> bias;
            Filters2.push_back(Neuron(weights, bias));
            weights.clear();
        }

        for (size_t i = 0; i < 128; i++) {
            for (size_t j = 0; j < 1152; j++) {
                file >> value;
                weights.push_back(value);
            }
            file >> bias;
            DenseNeurons.push_back(Neuron(weights, bias));
            weights.clear();
        }

        for (size_t i = 0; i < 26; i++) {
            for (size_t j = 0; j < 128; j++) {
                file >> value;
                weights.push_back(value);
            }
            file >> bias;
            DenseNeurons2.push_back(Neuron(weights, bias));
            weights.clear();
        }

        file.close();
    }

    bool SaveToFile(const string& fileName) {
        ofstream file(fileName);
        if (!file.is_open()) throw std::runtime_error("Couldn't open " + fileName);

        for (auto& n : Filters) {
            for (auto& w : n.Weights) {
                file << w << "\n";
            }
            file << n.bias << "\n";
        }
        for (auto& n : Filters2) {
            for (auto& w : n.Weights) {
                file << w << "\n";
            }
            file << n.bias << "\n";
        }
        for (auto& n : DenseNeurons) {
            for (auto& w : n.Weights) {
                file << w << "\n";
            }
            file << n.bias << "\n";
        }
        for (auto& n : DenseNeurons2) {
            for (auto& w : n.Weights) {
                file << w << "\n";
            }
            file << n.bias << "\n";
        }
        return true;
    }
};

struct ForwardPassData {
    vector<vector<vector<float>>> Input;
    vector<vector<vector<float>>> FeatureMaps;
    vector<vector<vector<float>>> Pooled;
    vector<vector<vector<pair<size_t,size_t>>>> MaxPositions;
    vector<vector<vector<float>>> FeatureMaps2;
    vector<vector<vector<float>>> Pooled2;
    vector<vector<vector<pair<size_t,size_t>>>> MaxPositions2;
    vector<float> Flattened;
    vector<float> RawDense;
    vector<float> Dense;
    vector<float> RawDense2;
    vector<float> Dense2;
    vector<float> Probabilities;
};

vector<vector<vector<float>>> ImageToMap(const vector<vector<int>>& Image);
vector<vector<vector<float>>> ConvolutionLayer(const vector<vector<vector<float>>>& input, const vector<Neuron>& Filters);
vector<vector<vector<float>>> PoolLayer(const vector<vector<vector<float>>>& featureMaps, vector<vector<vector<pair<size_t,size_t>>>>& maxPositions);
vector<float> Flattening(const vector<vector<vector<float>>>& pooled);
vector<float> DenseLayer(const vector<float>& input, const vector<Neuron>& DenseNeurons, vector<float>& RawDenseOutput, const bool& doReLU);
vector<float> Softmax(const vector<float>& logits);
size_t Argmax(const vector<float>& probabilities);
void ForwardPass(const vector<vector<int>>& Image, Network& net, ForwardPassData& fwdPass);

float Loss(const vector<float>& probabilities, size_t correctIndex);
vector<float> LossGradient(const vector<float>& probabilities, size_t correctIndex);
vector<float> ReLUMask(vector<float>& delta, const vector<float>& output);
vector<vector<vector<float>>> ReLUMask(vector<vector<vector<float>>>& delta, const vector<vector<vector<float>>>& output);
vector<float> BackpropDense(const vector<float>& delta, const vector<float>& input, vector<Neuron>& neurons, const float& learningRate);
vector<vector<vector<float>>> Unflattening(const vector<float>& delta, const size_t& height, const size_t& width, const size_t& depth);
vector<vector<vector<float>>> BackpropPool(const vector<vector<vector<float>>>& delta, const vector<vector<vector<pair<size_t,size_t>>>>& maxPositions, size_t origHeight, size_t origWidth);
vector<vector<vector<float>>> BackpropConv(const vector<vector<vector<float>>>& delta, const vector<vector<vector<float>>>& input, vector<Neuron>& filter, const float& learningRate);
void Backprop(Network& net, ForwardPassData& fwdPass, const size_t& correctIndex);

#endif // NET_H
