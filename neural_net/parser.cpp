#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "parser.h"
#include "labels.h"
#include "parser_label.h"
#include <stdexcept>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <numeric>

using namespace std;
namespace fs = std::filesystem;

// ── internal helpers ────────────────────────────────────────────────────────

static vector<vector<int>> ToGrayscale(const unsigned char* data, int w, int h, int channels) {
    vector<vector<int>> gray(h, vector<int>(w));
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * channels;
            int r = data[idx];
            int g = (channels >= 2) ? data[idx + 1] : r;
            int b = (channels >= 3) ? data[idx + 2] : r;
            gray[y][x] = (int)(0.299f * r + 0.587f * g + 0.114f * b);
        }
    return gray;
}

static int OtsuThreshold(const vector<vector<int>>& gray) {
    int hist[256] = {};
    int total = 0;
    for (auto& row : gray)
        for (int v : row) { hist[v]++; total++; }

    float sum = 0;
    for (int i = 0; i < 256; i++) sum += i * hist[i];

    float sumB = 0, wB = 0, wF = 0, maxVar = 0;
    int threshold = 128;
    for (int t = 0; t < 256; t++) {
        wB += hist[t];
        if (wB == 0) continue;
        wF = total - wB;
        if (wF == 0) break;
        sumB += t * hist[t];
        float mB = sumB / wB;
        float mF = (sum - sumB) / wF;
        float var = wB * wF * (mB - mF) * (mB - mF);
        if (var > maxVar) { maxVar = var; threshold = t; }
    }
    return threshold;
}

// 1 = ink (dark), 0 = background (light)
static vector<vector<int>> Binarize(const vector<vector<int>>& gray, int threshold) {
    int h = gray.size(), w = gray[0].size();
    vector<vector<int>> bin(h, vector<int>(w, 0));
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            bin[y][x] = (gray[y][x] < threshold) ? 1 : 0;
    return bin;
}

// Sum ink pixels per row
static vector<int> HorizontalProfile(const vector<vector<int>>& bin) {
    int h = bin.size(), w = bin[0].size();
    vector<int> profile(h, 0);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            profile[y] += bin[y][x];
    return profile;
}

// Sum ink pixels per column within a row band [y0, y1)
static vector<int> VerticalProfile(const vector<vector<int>>& bin, int y0, int y1) {
    int w = bin[0].size();
    vector<int> profile(w, 0);
    for (int y = y0; y < y1; y++)
        for (int x = 0; x < w; x++)
            profile[x] += bin[y][x];
    return profile;
}

// Find contiguous runs where profile > threshold, return [start, end) pairs
static vector<pair<int,int>> FindRuns(const vector<int>& profile, int inkThreshold, int minGap) {
    vector<pair<int,int>> runs;
    int n = profile.size();
    int start = -1;
    int lastEnd = -1;

    for (int i = 0; i < n; i++) {
        bool hasInk = profile[i] > inkThreshold;
        if (hasInk && start == -1) {
            // Merge with previous run if gap is too small
            if (lastEnd != -1 && i - lastEnd < minGap) {
                start = runs.back().first;
                runs.pop_back();
            } else {
                start = i;
            }
        } else if (!hasInk && start != -1) {
            runs.push_back({start, i});
            lastEnd = i;
            start = -1;
        }
    }
    if (start != -1) runs.push_back({start, n});
    return runs;
}

// Scale a crop to fit within 26x26, center it in a 28x28 grid
static vector<vector<int>> ResizeAndPad(const vector<vector<int>>& bin, int x0, int y0, int srcW, int srcH) {
    const int TARGET = 26;
    float scale = min((float)TARGET / srcH, (float)TARGET / srcW);
    int dstH = max(1, (int)(srcH * scale));
    int dstW = max(1, (int)(srcW * scale));

    vector<vector<int>> resized(dstH, vector<int>(dstW, 0));
    for (int y = 0; y < dstH; y++)
        for (int x = 0; x < dstW; x++) {
            int sy = min(y0 + (int)(y / scale), y0 + srcH - 1);
            int sx = min(x0 + (int)(x / scale), x0 + srcW - 1);
            resized[y][x] = bin[sy][sx];
        }

    int padTop  = (28 - dstH) / 2;
    int padLeft = (28 - dstW) / 2;
    vector<vector<int>> out(28, vector<int>(28, 0));
    for (int y = 0; y < dstH; y++)
        for (int x = 0; x < dstW; x++)
            out[padTop + y][padLeft + x] = resized[y][x];

    return out;
}

// Extract one window per character run, plus blank markers for gaps
// Returns: vector of (image, isBlank) pairs in reading order
static vector<pair<vector<vector<int>>, bool>> SlideLine(
    const vector<vector<int>>& bin,
    int lineY0, int lineY1,
    const vector<pair<int,int>>& charRuns)
{
    int lineH = lineY1 - lineY0;
    int imgW  = bin[0].size();
    // Minimum gap in columns to emit a space between characters
    const int SPACE_GAP = lineH / 3;

    vector<pair<vector<vector<int>>, bool>> results;

    int prevEnd = -1;
    for (auto& [cx0, cx1] : charRuns) {
        int runW = cx1 - cx0;
        if (runW < 2) continue; // skip noise

        // Emit a space if the gap since last character is large enough
        if (prevEnd != -1 && cx0 - prevEnd > SPACE_GAP)
            results.push_back({vector<vector<int>>(28, vector<int>(28, 0)), true}); // blank marker

        // Crop the character bounding box and resize to 28x28
        auto patch = ResizeAndPad(bin, cx0, lineY0, runW, lineH);
        results.push_back({patch, false});
        prevEnd = cx1;
    }

    return results;
}

// ── public API ──────────────────────────────────────────────────────────────

vector<Sample> ParseImage(const string& filePath) {
    int w, h, channels;
    unsigned char* data = stbi_load(filePath.c_str(), &w, &h, &channels, 0);
    if (!data)
        throw runtime_error("Failed to load image: " + filePath);

    // Shortcut: small images are a single character, skip segmentation
    if (w <= 28 && h <= 28) {
        auto gray  = ToGrayscale(data, w, h, channels);
        stbi_image_free(data);
        int thresh = OtsuThreshold(gray);
        if (thresh < 50) thresh = 128;
        auto bin   = Binarize(gray, thresh);
        return {{ ResizeAndPad(bin, 0, 0, w, h), 27 }};
    }

    auto gray = ToGrayscale(data, w, h, channels);
    stbi_image_free(data);

    int thresh = OtsuThreshold(gray);
    if (thresh < 50) thresh = 128;
    auto bin   = Binarize(gray, thresh);

    auto hProfile = HorizontalProfile(bin);
    auto lineRuns = FindRuns(hProfile, 0, 3);

    vector<Sample> results;
    bool lastWasBlank = true;

    for (auto& [ly0, ly1] : lineRuns) {
        int lineH = ly1 - ly0;
        if (lineH < 6) {continue; }

        auto vProfile = VerticalProfile(bin, ly0, ly1);
        auto charRuns = FindRuns(vProfile, 0, 4);

        auto windows = SlideLine(bin, ly0, ly1, charRuns);

        for (auto& [img, isBlank] : windows) {
            if (isBlank) {
                if (!lastWasBlank)
                    results.push_back({img, 26});
                lastWasBlank = true;
            } else {
                results.push_back({img, 27});
                lastWasBlank = false;
            }
        }

        // Newline
        if (!lastWasBlank)
            results.push_back({vector<vector<int>>(28, vector<int>(28, 0)), 28}); // 28 = newline
        lastWasBlank = true;
    }

    return results;
}

// Generate blank (no-letter) samples from gap regions between character runs
static vector<vector<vector<int>>> GenerateBlanks(
    const vector<vector<int>>& bin,
    int lineY0, int lineY1,
    const vector<pair<int,int>>& charRuns,
    int maxBlanks)
{
    int imgW = bin[0].size();
    const int WIN = 28;
    vector<vector<vector<int>>> blanks;

    // Collect gap x-ranges between character runs
    vector<pair<int,int>> gaps;
    int prev = 0;
    for (auto& [cx0, cx1] : charRuns) {
        if (cx0 - prev >= WIN)
            gaps.push_back({prev, cx0});
        prev = cx1;
    }
    if (imgW - prev >= WIN)
        gaps.push_back({prev, imgW});

    for (auto& [gx0, gx1] : gaps) {
        for (int wx = gx0; wx + WIN <= gx1 && (int)blanks.size() < maxBlanks; wx += WIN)
            blanks.push_back(ResizeAndPad(bin, wx, lineY0, WIN, lineY1 - lineY0));
    }

    return blanks;
}

vector<Sample> LoadDataset(const string& folderPath) {
    vector<Sample> dataset;

    for (auto& entry : fs::directory_iterator(folderPath)) {
        if (!entry.is_regular_file()) continue;
        string path = entry.path().string();
        string name = entry.path().filename().string();

        string ext = entry.path().extension().string();
        if (ext != ".png" && ext != ".PNG") continue;

        size_t label = PrefixToLabel(name);
        if (label == BLANK_LABEL) {
            cerr << "Warning: unrecognised prefix in " << name << ", skipping\n";
            continue;
        }

        try {
            int w, h, channels;
            unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 0);
            if (!data) { cerr << "Warning: couldn't load " << name << "\n"; continue; }

            auto gray = ToGrayscale(data, w, h, channels);
            stbi_image_free(data);
            int thresh = OtsuThreshold(gray);
            auto bin   = Binarize(gray, thresh);

            auto hProfile = HorizontalProfile(bin);
            auto lineRuns = FindRuns(hProfile, 0, 3);

            for (auto& [ly0, ly1] : lineRuns) {
                if (ly1 - ly0 < 6) continue;

                auto vProfile = VerticalProfile(bin, ly0, ly1);
                auto charRuns = FindRuns(vProfile, 0, 2);

                // Positive samples: windows that overlap a character
                auto windows = SlideLine(bin, ly0, ly1, charRuns);
                for (auto& [img, isBlank] : windows)
                    if (!isBlank)
                        dataset.push_back({img, label});

                // Negative samples: windows sitting in gaps, label = 26 (blank)
                auto blanks = GenerateBlanks(bin, ly0, ly1, charRuns, 5);
                for (auto& img : blanks)
                    dataset.push_back({img, 26});
            }

        } catch (const exception& e) {
            cerr << "Warning: skipping " << name << " — " << e.what() << "\n";
        }
    }

    return dataset;
}
