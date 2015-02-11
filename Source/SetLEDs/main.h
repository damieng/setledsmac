#ifndef SetLEDs_main_h
#define SetLEDs_main_h

#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
#include <IOKit/hid/IOHIDLib.h>

// Key bit flags
const int CAPS = 0x01;
const int NUM = 0x02;
const int SCROLL = 0x04;

void parseOptions(int argc, const char * argv[]);
void explainUsage();
void setAllKeyboards(int setLEDs, int clearLEDs);

#endif
