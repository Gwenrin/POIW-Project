#pragma once

#include <vector>
#include <string>

using namespace std;

// Label 0-25 = A-Z, 26 = blank/space, 27 = unknown (inference only)
struct Sample {
    vector<vector<int>> Image;
    size_t Label;
};

// Load all labeled samples from a folder of PNGs named X_YYY.png
// Positive samples labeled from filename, blanks get label 26
vector<Sample> LoadDataset(const string& folderPath);

// Parse an image into a sequence of samples in reading order
// Label 26 = space, label 27 = character (identity determined by net)
vector<Sample> ParseImage(const string& filePath);
