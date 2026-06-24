#ifndef INTERFACE_H
#define INTERFACE_H

#include "net.h"
#include <string>

// #define TRAIN
#define LOAD
//#define FULL_EPOCH
//#define SAVE
//#define VERBOSE

std::string NetRun(const std::string& imagePath, const std::string& weightsPath);
void NetTrain();
void NetInterface(const std::string& imagePath, const std::string& weightsPath);

#endif // INTERFACE_H
