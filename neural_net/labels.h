#ifndef LABELS_H
#define LABELS_H

#include <string>
#include <vector>

// 97 classes: 0-25 A-Z, 26-51 a-z, 52-60 ĄĆĘŁŃÓŚŹŻ, 61-69 ąćęłńóśźż,
// 70-95 punctuation, 96 blank
// All strings are UTF-8 encoded

static const std::vector<std::string> LABELS = {
    // 0-25: uppercase Latin
    "A","B","C","D","E","F","G","H","I","J","K","L","M",
    "N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
    // 26-51: lowercase Latin
    "a","b","c","d","e","f","g","h","i","j","k","l","m",
    "n","o","p","q","r","s","t","u","v","w","x","y","z",
    // 52-60: uppercase Polish
    "\xC4\x84","\xC4\x86","\xC4\x98","\xC5\x81","\xC5\x83",
    "\xC3\x93","\xC5\x9A","\xC5\xB9","\xC5\xBB",
    // 61-69: lowercase Polish
    "\xC4\x85","\xC4\x87","\xC4\x99","\xC5\x82","\xC5\x84",
    "\xC3\xB3","\xC5\x9B","\xC5\xBA","\xC5\xBC",
    // 70-95: punctuation
    ".","," ,"?","!","\"","'",";",":","-","_",
    "(",")",  "[","]","/","\\","@","#","$","%",
    "&","*",  "+","=","<",">"
};

static const size_t BLANK_LABEL   = 96;
static const size_t NUM_CLASSES   = 97; // including blank
static const size_t OUTPUT_NEURONS = 97;

inline std::string LabelToString(size_t label) {
    if (label < LABELS.size()) return LABELS[label];
    return " "; // blank
}

// Returns index of character in LABELS, or BLANK_LABEL if not found
inline size_t StringToLabel(const std::string& s) {
    for (size_t i = 0; i < LABELS.size(); i++)
        if (LABELS[i] == s) return i;
    return BLANK_LABEL;
}

#endif // LABELS_H
