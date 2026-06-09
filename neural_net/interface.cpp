#include "interface.h"
#include "parser.h"
#include "net.h"

#include <algorithm>
#include <random>
#include <ctime>

using namespace std;

string NetRun() {
    Network net("net_weights.txt");
    ForwardPassData fwdData;
    string fullResult = "";

    auto samples = ParseImage("temp.png");
    if (samples.empty()) {
        cout << "(no characters detected)" << endl;
        return "";
    }

    for (auto& sample : samples) {
        if (sample.Label == 26) {
            cout << ' ';
            continue;
        }
        ForwardPass(sample.Image, net, fwdData);
        size_t result = Argmax(fwdData.Probabilities);
        if (result < 26) {
            fullResult = (char)('A' + result);
        } else fullResult += ' ';
    }
    return fullResult;
}

void NetTrain() {
    const string dataFolder  = "dataset";
    const string weightsFile = "net_weights.txt";
    const int    EPOCHS      = 20;

    #ifdef LOAD
        Network net(weightsFile);
    #else
        mt19937 rng(time(NULL));
        uniform_real_distribution<float> dist(-0.01f, 0.01f);
        Network net(rng, dist);
    #endif

    auto dataset = LoadDataset(dataFolder);
    if (dataset.empty()) {
        cerr << "No samples found in " << dataFolder << endl;
        return;
    }
    cout << "Loaded " << dataset.size() << " samples." << endl;

    mt19937 rng(time(NULL));
    ForwardPassData fwdData;

    for (int epoch = 0; epoch < EPOCHS; epoch++) {
        shuffle(dataset.begin(), dataset.end(), rng);

        float totalLoss = 0.0f;
        int correct = 0;
        char expected, got;

        for (auto& sample : dataset) {
            got = ForwardPass(sample.Image, net, fwdData);
            totalLoss += Loss(fwdData.Probabilities, sample.Label);
            if (Argmax(fwdData.Probabilities) == sample.Label) correct++;
            Backprop(net, fwdData, sample.Label);
            expected = (sample.Label < 26) ? ('A' + sample.Label) : '_';
        }

        cout << "Expected: " << expected << "  Got: " << got << endl;

        cout << "Epoch " << epoch + 1 << "/" << EPOCHS
             << "  Loss: " << totalLoss / dataset.size()
             << "  Accuracy: " << (100.0f * correct / dataset.size()) << "%" << endl;
    }

    #ifdef SAVE
        net.SaveToFile(weightsFile);
        cout << "Weights saved to " << weightsFile << endl;
    #endif
}

void NetInterface() {
    #ifdef TRAIN
        NetTrain();
    #else
        NetRun();
    #endif
}
