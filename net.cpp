#include <iostream>
#include <vector>
#include <random>
#include <cmath>

using namespace std;

class Neuron {
public:
    vector<float> Weights;
    float bias;

    Neuron(const int& n,  mt19937& rng, uniform_real_distribution<float>& dist) {
        Weights.resize(n);
        SetRandomInitialWeights(rng, dist);
        bias = dist(rng);
    }

    float Compute(const vector<float>& inputs) const;

private:
    void SetRandomInitialWeights(mt19937& rng, uniform_real_distribution<float>& dist);
};

void Neuron::SetRandomInitialWeights(mt19937& rng, uniform_real_distribution<float>& dist) {
    for (size_t i = 0; i < Weights.size(); i++) {
        Weights[i] = dist(rng);
    }
}

float Neuron::Compute(const vector<float>& inputs) const {
    if (inputs.size() != Weights.size()) {
        throw std::invalid_argument("Incorrect number of inputs.");
    }

    float sum = 0;
    for (size_t i = 0; i < inputs.size(); i++) {
        sum += inputs[i] * Weights[i];
    }
    sum += bias;

    return sum;
}

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
            DenseNeurons.push_back(Neuron(800, rng, dist));
        }
        for (size_t i = 0; i < 26; i++) {
            DenseNeurons2.push_back(Neuron(128, rng, dist));
        }
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

vector<vector<vector<float>>> ImageToMap(const vector<vector<int>>& Image) {
    size_t imageDimension = Image.size();
    vector<vector<vector<float>>> Input(imageDimension, vector<vector<float>>(imageDimension, vector<float>(1)));
    for (size_t i = 0; i < imageDimension; i++) {
        for (size_t j = 0; j < imageDimension; j++) {
            Input[i][j][0] = (float)Image[i][j];
        }
    }
    return Input;
}

vector<vector<vector<float>>> ConvolutionLayer(const vector<vector<vector<float>>>& input, const vector<Neuron>& Filters) {
    size_t depth = input[0][0].size();
    int height = input.size() - 2;
    int width = input[0].size() - 2;

    vector<vector<vector<float>>> FeatureMaps(height, vector<vector<float>>(width, vector<float>(Filters.size())));

    for (size_t k = 0; k < Filters.size(); k++) {
        for (size_t i = 0; i + 2 < input.size(); i++) {
            for (size_t j = 0; j + 2 < input[i].size(); j++) {
                vector<float> patch;
                for (size_t d = 0; d < depth; d++) {
                    patch.push_back(input[i][j][d]);
                    patch.push_back(input[i][j+1][d]);
                    patch.push_back(input[i][j+2][d]);
                    patch.push_back(input[i+1][j][d]);
                    patch.push_back(input[i+1][j+1][d]);
                    patch.push_back(input[i+1][j+2][d]);
                    patch.push_back(input[i+2][j][d]);
                    patch.push_back(input[i+2][j+1][d]);
                    patch.push_back(input[i+2][j+2][d]);
                }
                FeatureMaps[i][j][k] = max(0.0f, Filters[k].Compute(patch));
            }
        }
    }

    return FeatureMaps;
}

vector<vector<vector<float>>> PoolLayer(const vector<vector<vector<float>>>& featureMaps, vector<vector<vector<pair<size_t,size_t>>>>& maxPositions) {
    size_t height = featureMaps.size();
    size_t width = featureMaps[0].size();
    size_t depth = featureMaps[0][0].size();
    vector<vector<vector<float>>> Pooled(height / 2, vector<vector<float>>(width / 2, vector<float>(depth)));

    for (size_t k = 0; k < depth; k++) {
        for (size_t i = 0; i + 1 < height; i += 2) {
            for (size_t j = 0; j + 1 < width; j += 2) {
                pair<size_t,size_t> maxPos = {i, j};
                float maxVal = featureMaps[i][j][k];
                if (featureMaps[i+1][j][k] > maxVal) { maxVal = featureMaps[i+1][j][k]; maxPos = {i+1, j}; }
                if (featureMaps[i][j+1][k] > maxVal) { maxVal = featureMaps[i][j+1][k]; maxPos = {i, j+1}; }
                if (featureMaps[i+1][j+1][k] > maxVal) { maxVal = featureMaps[i+1][j+1][k]; maxPos = {i+1, j+1}; }
                Pooled[i/2][j/2][k] = maxVal;
                maxPositions[i/2][j/2][k] = maxPos;
            }
        }
    }
    return Pooled;
}

vector<float> Flattening(const vector<vector<vector<float>>>& pooled) {
    vector<float> Flattened;
    for (size_t k = 0; k < pooled[0][0].size(); k++) {
        for (size_t i = 0; i < pooled.size(); i++) {
            for (size_t j = 0; j < pooled[0].size(); j++) {
                Flattened.push_back(pooled[i][j][k]);
            }
        }
    }
    return Flattened;
}

vector<float> DenseLayer(const vector<float>& input, const vector<Neuron>& DenseNeurons, vector<float>& RawDenseOutput, const bool& doReLU) {
    vector<float> DenseOutput;
    for (size_t i = 0; i < DenseNeurons.size(); i++) {
        float output = DenseNeurons[i].Compute(input);
        RawDenseOutput.push_back(output);
        if (doReLU) {
            DenseOutput.push_back(max(0.0f, output));
        } else DenseOutput.push_back(output);
    }
    return DenseOutput;
}

vector<float> Softmax(const vector<float>& logits) {
    vector<float> result;
    float sum = 0;
    for (size_t i = 0; i < logits.size(); i++) {
        sum += exp(logits[i]);
    }
    for (size_t i = 0; i < logits.size(); i++) {
        result.push_back(exp(logits[i]) / sum);
    }
    return result;
}

size_t Argmax(const vector<float>& probabilities) {
    float maximum = 0;
    size_t maxIndex = 0;
    for (size_t i = 0; i < probabilities.size(); i++) {
        if (probabilities[i] > maximum) {
            maximum = probabilities[i];
            maxIndex = i;
        }
    }
    return maxIndex;
}

void ForwardPass(const vector<vector<int>>& Image, Network& net, ForwardPassData& fwdPass) {
    fwdPass.RawDense.clear();
    fwdPass.RawDense2.clear();
    fwdPass.Flattened.clear();
    fwdPass.Input = ImageToMap(Image);

    fwdPass.FeatureMaps = ConvolutionLayer(fwdPass.Input, net.Filters);
    fwdPass.MaxPositions.clear();
    fwdPass.MaxPositions.resize(fwdPass.FeatureMaps.size() / 2, vector<vector<pair<size_t,size_t>>>(fwdPass.FeatureMaps[0].size() / 2, vector<pair<size_t,size_t>>(fwdPass.FeatureMaps[0][0].size())));
    fwdPass.Pooled = PoolLayer(fwdPass.FeatureMaps, fwdPass.MaxPositions);

    fwdPass.FeatureMaps2 = ConvolutionLayer(fwdPass.Pooled, net.Filters2);
    fwdPass.MaxPositions2.clear();
    fwdPass.MaxPositions2.resize(fwdPass.FeatureMaps2.size() / 2, vector<vector<pair<size_t,size_t>>>(fwdPass.FeatureMaps2[0].size() / 2, vector<pair<size_t,size_t>>(fwdPass.FeatureMaps2[0][0].size())));
    fwdPass.Pooled2 = PoolLayer(fwdPass.FeatureMaps2, fwdPass.MaxPositions2);


    fwdPass.Flattened = Flattening(fwdPass.Pooled2);
    fwdPass.Dense = DenseLayer(fwdPass.Flattened, net.DenseNeurons, fwdPass.RawDense, true);
    fwdPass.Dense2 = DenseLayer(fwdPass.Dense, net.DenseNeurons2, fwdPass.RawDense2, false);

    fwdPass.Probabilities = Softmax(fwdPass.Dense2);

    auto resultIndex = Argmax(fwdPass.Probabilities);

    /*
    switch(resultIndex) {
        case 0: cout << 'A'; break;
        case 1: cout << 'B'; break;
        case 2: cout << 'C'; break;
        case 3: cout << 'D'; break;
        case 4: cout << 'E'; break;
        case 5: cout << 'F'; break;
        case 6: cout << 'G'; break;
        case 7: cout << 'H'; break;
        case 8: cout << 'I'; break;
        case 9: cout << 'J'; break;
        case 10: cout << 'K'; break;
        case 11: cout << 'L'; break;
        case 12: cout << 'M'; break;
        case 13: cout << 'N'; break;
        case 14: cout << 'O'; break;
        case 15: cout << 'P'; break;
        case 16: cout << 'Q'; break;
        case 17: cout << 'R'; break;
        case 18: cout << 'S'; break;
        case 19: cout << 'T'; break;
        case 20: cout << 'U'; break;
        case 21: cout << 'V'; break;
        case 22: cout << 'W'; break;
        case 23: cout << 'X'; break;
        case 24: cout << 'Y'; break;
        case 25: cout << 'Z'; break;
    }
    */
    cout << (char)('A' + resultIndex) << endl;
}

float Loss(const vector<float>& probabilities, size_t correctIndex) {
    return -log(probabilities[correctIndex]);
}

vector<float> LossGradient(const vector<float>& probabilities, size_t correctIndex) {
    vector<float> delta = probabilities;
    delta[correctIndex] -= 1.0f;
    return delta;
}

vector<float> ReLUMask(vector<float>& delta, const vector<float>& output) {
    if (delta.size() != output.size()) {
        throw std::invalid_argument("Delta size different from ReLU mask size");
    }

    for (size_t i = 0; i < output.size(); i++) {
        if (output[i] <= 0) {
            delta[i] = 0;
        }
    }
    return delta;
}

vector<vector<vector<float>>> ReLUMask(vector<vector<vector<float>>>& delta, const vector<vector<vector<float>>>& output) {
    if (delta.size() != output.size()) {
        throw std::invalid_argument("Delta size different from ReLU mask size");
    }

    for (size_t i = 0; i < output.size(); i++) {
        for (size_t j = 0; j < output[0].size(); j++) {
            for (size_t k = 0; k < output[0][0].size(); k++) {
                if (output[i][j][k] <= 0) {
                    delta[i][j][k] = 0;
                }
            }
        }
    }
    return delta;
}

vector<float> BackpropDense(const vector<float>& delta, const vector<float>& input, vector<Neuron>& neurons, const float& learningRate) {
    vector<float> deltaForPrevious(input.size(), 0.0f);
    for (size_t i = 0; i < neurons.size(); i++) {
        neurons[i].bias -= learningRate * delta[i];
        for (size_t j = 0; j < neurons[0].Weights.size(); j++) {
            deltaForPrevious[j] += delta[i] * neurons[i].Weights[j];
            neurons[i].Weights[j] -= learningRate * delta[i] * input[j];
        }
    }
    return deltaForPrevious;
}

vector<vector<vector<float>>> Unflattening(const vector<float>& delta, const size_t& height, const size_t& width, const size_t& depth) {
    vector<vector<vector<float>>> Unflattened(height, vector<vector<float>>(width, vector<float>(depth, 0.0f)));
    size_t di = 0;
    for (size_t k = 0; k < depth; k++) {
        for (size_t i = 0; i < height; i++) {
            for (size_t j = 0; j < width; j++) {
                Unflattened[i][j][k] = delta[di];
                di++;
            }
        }
    }
    return Unflattened;
}

vector<vector<vector<float>>> BackpropPool(const vector<vector<vector<float>>>& delta, const vector<vector<vector<pair<size_t,size_t>>>>& maxPositions, size_t origHeight, size_t origWidth) {
    size_t height = delta.size();
    size_t width = delta[0].size();
    size_t depth = delta[0][0].size();
    vector<vector<vector<float>>> convDelta(origHeight, vector<vector<float>>(origWidth, vector<float>(depth, 0.0f)));

    for (size_t k = 0; k < depth; k++) {
        for (size_t i = 0; i < height; i++) {
            for (size_t j = 0; j < width; j++) {
                auto [row, col] = maxPositions[i][j][k];
                convDelta[row][col][k] = delta[i][j][k];
            }
        }
    }
    return convDelta;
}

vector<vector<vector<float>>> BackpropConv(const vector<vector<vector<float>>>& delta, const vector<vector<vector<float>>>& input, vector<Neuron>& filter, const float& learningRate) {
    size_t inHeight = input.size();
    size_t inWidth = input[0].size();
    size_t inDepth = input[0][0].size();
    vector<vector<vector<float>>> previousDelta(inHeight, vector<vector<float>>(inWidth, vector<float>(inDepth, 0.0f)));

    for (size_t k = 0; k < filter.size(); k++) {
        for (size_t i = 0; i < delta.size(); i++) {
            for (size_t j = 0; j < delta[0].size(); j++) {
                // 1. Extract patch
                vector<float> patch;
                for (size_t d = 0; d < inDepth; d++) {
                    patch.push_back(input[i][j][d]);
                    patch.push_back(input[i][j+1][d]);
                    patch.push_back(input[i][j+2][d]);
                    patch.push_back(input[i+1][j][d]);
                    patch.push_back(input[i+1][j+1][d]);
                    patch.push_back(input[i+1][j+2][d]);
                    patch.push_back(input[i+2][j][d]);
                    patch.push_back(input[i+2][j+1][d]);
                    patch.push_back(input[i+2][j+2][d]);
                }

                // 2. Spread delta back
                for (size_t di = 0; di < 3; di++) {
                    for (size_t dj = 0; dj < 3; dj++) {
                        for (size_t d = 0; d < inDepth; d++) {
                            size_t w = d * 9 + di * 3 + dj;
                            previousDelta[i+di][j+dj][d] += delta[i][j][k] * filter[k].Weights[w];
                        }
                    }
                }

                // 3. Update weights
                for (size_t w = 0; w < filter[k].Weights.size(); w++) {
                    filter[k].Weights[w] -= learningRate * delta[i][j][k] * patch[w];
                }
            }
        }
    }
    return previousDelta;
}

void Backprop(Network& net, ForwardPassData& fwdPass, const size_t& correctIndex) {
    float learningRate = 0.001;

    auto deltaDense2 = BackpropDense(LossGradient(fwdPass.Probabilities, correctIndex), fwdPass.Dense, net.DenseNeurons2, learningRate);
    auto deltaDense = BackpropDense(ReLUMask(deltaDense2, fwdPass.RawDense), fwdPass.Flattened, net.DenseNeurons, learningRate);
    auto unFlattenedDelta = Unflattening(deltaDense, fwdPass.Pooled2.size(), fwdPass.Pooled2[0].size(), fwdPass.Pooled2[0][0].size());
    auto deltaPool2 = BackpropPool(unFlattenedDelta, fwdPass.MaxPositions2, fwdPass.FeatureMaps2.size(), fwdPass.FeatureMaps2[0].size());
    auto deltaConvulted2 = BackpropConv(ReLUMask(deltaPool2, fwdPass.FeatureMaps2), fwdPass.Pooled, net.Filters2, learningRate);
    auto deltaPool = BackpropPool(deltaConvulted2, fwdPass.MaxPositions, fwdPass.FeatureMaps.size(), fwdPass.FeatureMaps[0].size());
    auto deltaConvulted = BackpropConv(ReLUMask(deltaPool, fwdPass.FeatureMaps), fwdPass.Input, net.Filters, learningRate);
}

int main() {
    mt19937 rng(time(NULL));
    uniform_real_distribution<float> dist(-0.01f, 0.01f);

    Network net(rng, dist);
    ForwardPassData fwdData;

    vector<vector<int>> Image = {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0},
        {0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0},
        {0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0},
        {0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0},
        {0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0},
        {0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
        {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1}
    };
    size_t correctIndex = 7;

    for (size_t epoch = 0; epoch < 100; epoch++) {
        ForwardPass(Image, net, fwdData);
        Backprop(net, fwdData, correctIndex);
        /*if (epoch % 100 == 0)*/ cout << "Epoch " << epoch << " Loss: " << Loss(fwdData.Probabilities, correctIndex) << endl;
    }

    return 0;
}
