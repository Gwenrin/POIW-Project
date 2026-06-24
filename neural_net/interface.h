#ifndef INTERFACE_H
#define INTERFACE_H

#include "net.h"
#include <string>

//#define TRAIN
#define LOAD
//#define FULL_EPOCH
//#define SAVE
//#define VERBOSE

std::string NetRun();
void NetTrain();
void NetInterface();

#endif // INTERFACE_H
