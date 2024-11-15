/*  setleds for Mac
    https://github.com/damieng/setledsmac
    Copyright 2015-2024 Damien Guard. GPL 2 licenced.
 */

#ifndef SetLEDs_main_h
#define SetLEDs_main_h

#include <CoreFoundation/CoreFoundation.h>

const int maxLeds = 3;
const char* ledNames[] = { "num", "caps", "scroll" };
const char* stateSymbol[] = {"-", "+" };
typedef enum { NoChange = -1, Off, On, Toggle } LedState;

void parseOptions(int argc, const char * argv[]);
void explainUsage(void);
void setAllKeyboards(LedState changes[]);
CFMutableDictionaryRef getKeyboardDictionary(void);

#endif
