#ifndef PARSER_LABEL_H
#define PARSER_LABEL_H

#include "labels.h"
#include <string>
#include <map>

inline size_t PrefixToLabel(const std::string& filename) {
    static const std::map<std::string, size_t> PREFIX_TO_LABEL = {
        {"A",0},{"B",1},{"C",2},{"D",3},{"E",4},{"F",5},{"G",6},{"H",7},
        {"I",8},{"J",9},{"K",10},{"L",11},{"M",12},{"N",13},{"O",14},{"P",15},
        {"Q",16},{"R",17},{"S",18},{"T",19},{"U",20},{"V",21},{"W",22},{"X",23},
        {"Y",24},{"Z",25},
        {"la",26},{"lb",27},{"lc",28},{"ld",29},{"le",30},{"lf",31},{"lg",32},
        {"lh",33},{"li",34},{"lj",35},{"lk",36},{"ll",37},{"lm",38},{"ln",39},
        {"lo",40},{"lp",41},{"lq",42},{"lr",43},{"ls",44},{"lt",45},{"lu",46},
        {"lv",47},{"lw",48},{"lx",49},{"ly",50},{"lz",51},
        {"pA",52},{"pC",53},{"pE",54},{"pL",55},{"pN",56},{"pO",57},{"pS",58},
        {"pZ",59},{"pZZ",60},
        {"plA",61},{"plC",62},{"plE",63},{"plL",64},{"plN",65},{"plO",66},
        {"plS",67},{"plZ",68},{"plZZ",69},
        {"PERIOD",70},{"COMMA",71},{"QUESTION",72},{"EXCLAIM",73},{"DQUOTE",74},
        {"SQUOTE",75},{"SEMICOL",76},{"COLON",77},{"DASH",78},{"UNDER",79},
        {"LPAREN",80},{"RPAREN",81},{"LBRACK",82},{"RBRACK",83},{"SLASH",84},
        {"BSLASH",85},{"AT",86},{"HASH",87},{"DOLLAR",88},{"PERCENT",89},
        {"AMP",90},{"STAR",91},{"PLUS",92},{"EQUAL",93},{"LANGLE",94},{"RANGLE",95}
    };

    size_t lastUnderscore = filename.rfind('_');
    if (lastUnderscore == std::string::npos) return BLANK_LABEL;
    std::string prefix = filename.substr(0, lastUnderscore);
    auto it = PREFIX_TO_LABEL.find(prefix);
    if (it == PREFIX_TO_LABEL.end()) return BLANK_LABEL;
    return it->second;
}

#endif // PARSER_LABEL_H
