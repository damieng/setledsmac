#ifndef SetLEDs_main_h
#define SetLEDs_main_h

#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
#include <IOKit/hid/IOHIDLib.h>

void parseOptions(int argc, const char * argv[]);
void explainUsage();
void setAllKeyboards(int changeLEDs[]);

#endif
