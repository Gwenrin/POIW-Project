#include "interface.h"
#include "parser.h"
#include "labels.h"

#include <algorithm>
#include <random>
#include <ctime>
#include <sstream>
#include <iomanip>

using namespace std;

// Escape a UTF-8 string for JSON output
static string JsonEscape(const string& s) {
    string out;
    for (unsigned char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else                out += c;
    }
    return out;
}

string NetRun(const std::string& imagePath, const std::string& weightsPath) {
    Network net(weightsPath);
    ForwardPassData fwdData;

    auto samples = ParseImage(imagePath);
    if (samples.empty()) {
        cout << "{\"text\":empty\"\",\"confidence\":0.0}" << endl;
        return "";
    }

    string result;
    float totalProb = 0.0f;
    int charCount = 0;

    for (auto& sample : samples) {
        if (sample.Label == BLANK_LABEL || sample.Label == SPACE_LABEL) {
            result += ' ';
            continue;
        }
        if (sample.Label == NEWLINE_LABEL) {
            result += '\n';
            continue;
        }

        ForwardPass(sample.Image, net, fwdData);
        size_t pred = Argmax(fwdData.Probabilities);
        float prob  = fwdData.Probabilities[pred];

        if (pred < BLANK_LABEL) {
            result += LabelToString(pred);
            totalProb += prob;
            charCount++;
        }
    }

    float certainty = (charCount > 0) ? (totalProb / charCount) : 0.0f;

    // Output JSON to stdout for the Java backend
    while (!result.empty() && (result.back() == ' ' || result.back() == '\n')) //clears trailing spaces and newlines
        result.pop_back();
    ostringstream json;
    json << fixed << setprecision(4);
    json << "{\"text\":\"" << JsonEscape(result)
         << "\",\"confidence\":" << certainty << "}";
    cout << json.str() << endl;

    return result;
}

void NetTrain() {
    const string dataFolder  = "dataset";
    const string weightsFile = "net_weights.txt";
    const int    EPOCHS      = 5;

    mt19937 rng(time(NULL));
    #ifdef LOAD
        Network net(weightsFile);
    #else
        uniform_real_distribution<float> dist(-0.01f, 0.01f);
        Network net(rng, dist);
    #endif

    auto dataset = LoadDataset(dataFolder);
    if (dataset.empty()) {
        cerr << "No samples found in " << dataFolder << endl;
        return;
    }
    cout << "Loaded " << dataset.size() << " samples." << endl;

    ForwardPassData fwdData;

    for (int epoch = 0; epoch < EPOCHS; epoch++) {
        shuffle(dataset.begin(), dataset.end(), rng);

        float totalLoss = 0.0f;
        int correct = 0;

        int sampleIdx = 0;
        for (auto& sample : dataset) {
            ForwardPass(sample.Image, net, fwdData);
            totalLoss += Loss(fwdData.Probabilities, sample.Label);

            size_t pred = Argmax(fwdData.Probabilities);
            if (pred == sample.Label) correct++;

            if (++sampleIdx % 10000 == 0) cout << "  Sample " << sampleIdx << "/" << dataset.size() << endl;
            #ifdef VERBOSE
                string expected = (sample.Label < BLANK_LABEL) ? LabelToString(sample.Label) : "_";
                string got      = (pred < BLANK_LABEL)         ? LabelToString(pred)         : "_";
                cout << "Expected: " << expected << "  Got: " << got << endl;
            #endif

            Backprop(net, fwdData, sample.Label);
        }

        cout << "Epoch " << epoch + 1 << "/" << EPOCHS
             << "  Loss: " << totalLoss / dataset.size()
             << "  Accuracy: " << (100.0f * correct / dataset.size()) << "%" << endl;

        #ifdef SAVE
            net.SaveToFile(weightsFile);
            cout << "Weights saved to " << weightsFile << endl;
        #endif
    }


}

void NetInterface(const std::string& imagePath, const std::string& weightsPath) {
    #ifdef TRAIN
        NetTrain();
    #else
        NetRun(imagePath, weightsPath);
    #endif
}
