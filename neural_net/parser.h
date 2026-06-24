#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>

using namespace std;

// Label 0-95 = characters per LABELS in labels.h
// Label 96 = blank/space
// Label 97 = unknown (inference only, identity determined by net)
struct Sample {
    vector<vector<int>> Image;
    size_t Label;
};

// Load all labeled samples from a dataset folder
// Filenames must match the prefix scheme in generate_dataset.py
vector<Sample> LoadDataset(const string& folderPath);

// Parse an image into a reading-order sequence of samples
// Label 96 = space, label 97 = character (net decides which)
vector<Sample> ParseImage(const string& filePath);

#endif // PARSER_H
