/*  setleds for Mac
    http://github.com/damieng/setledsmac
    Copyright 2015 Damien Guard. GPL 2 licenced.
 */

#include "main.h"

Boolean verbose = false;
Boolean debug = false;
const int maxLEDs = kHIDUsage_LED_ScrollLock + 1;

int main(int argc, const char * argv[])
{
    parseOptions(argc, argv);
    return 0;
}

void parseOptions(int argc, const char * argv[])
{
    if (argc == 1) {
        explainUsage();
        exit(1);
    }
    
    int changeLEDs[maxLEDs] = { 0 };
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0)
            verbose = true;
        else if (strcmp(argv[i], "-debug") == 0)
            debug = true;
        else if (strcmp(argv[i], "+num") == 0)
            changeLEDs[kHIDUsage_LED_NumLock] = 1;
        else if (strcmp(argv[i], "-num") == 0)
            changeLEDs[kHIDUsage_LED_NumLock] = -1;
        else if (strcmp(argv[i], "+caps") == 0)
            changeLEDs[kHIDUsage_LED_CapsLock] = 1;
        else if (strcmp(argv[i], "-caps") == 0)
            changeLEDs[kHIDUsage_LED_CapsLock] = -1;
        else if (strcmp(argv[i], "+scroll") == 0)
            changeLEDs[kHIDUsage_LED_ScrollLock] = 1;
        else if (strcmp(argv[i], "-scroll") == 0)
            changeLEDs[kHIDUsage_LED_ScrollLock] = -1;
        else {
            fprintf(stderr, "Unknown option %s\n\n", argv[i]);
            explainUsage();
            exit(1);
        }
    }
    
    setAllKeyboards(changeLEDs);
}

void explainUsage()
{
    printf("Usage:\tsetleds [-v] [[+|-][ num | caps | scroll]]\n"
           "Thus,\tsetleds +caps -num\n"
           "will set CapsLock, clear NumLock and leave ScrollLock unchanged.\n"
           "The settings before and after the change (if any) are reported\n"
           "when the -v option is given or when no change is requested.\n");
}

CFMutableDictionaryRef CreateDictionaryUsagePageUsage(CFStringRef key, UInt32 inUsagePage, UInt32 inUsage)
{
    CFMutableDictionaryRef result = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    if (result) {
        if (inUsagePage) {
            CFNumberRef page = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &inUsagePage);
            if (page) {
                CFDictionarySetValue(result, key, page);
                CFRelease(page);
                
                if (inUsage) {
                    CFNumberRef usage = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &inUsage);
                    
                    if (usage) {
                        CFDictionarySetValue(result, key, usage);
                        CFRelease(usage);
                    } else {
                        fprintf(stderr, "CFNumberCreate(usage) failed.");
                    }
                }
            } else {
                fprintf(stderr, "CFNumberCreate(usage page) failed.");
            }
        }
    } else {
        fprintf(stderr, "CFDictionaryCreateMutable failed.");
    }
    
    return result;
}

Boolean isKeyboardDevice(struct __IOHIDDevice *device)
{
    return IOHIDDeviceConformsTo(device, kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard);
}

void setKeyboard(struct __IOHIDDevice *device, CFDictionaryRef keyboardDictionary, int changeLEDs[])
{
    CFStringRef deviceName = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));
    if (deviceName) {
        printf("Found %s\n", CFStringGetCStringPtr(deviceName, kCFStringEncodingUTF8));
        CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, keyboardDictionary, kIOHIDOptionsTypeNone);
        if (elements) {
            int before[maxLEDs] = { 0 };
            int after[maxLEDs] = { 0 };
            CFIndex elementCount = CFArrayGetCount(elements);
            for (CFIndex elementIndex = 0; elementIndex < elementCount; elementIndex++) {
                IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, elementIndex);
                if (element) {
                    if (kHIDPage_LEDs == IOHIDElementGetUsagePage(element)) {
                        uint32_t led = IOHIDElementGetUsage(element);
                        if (debug) printf(" Examining usage %d\n", led);
                        
                        // Get current keyboard led status
                        IOHIDValueRef oldValue = 0;
                        IOHIDDeviceGetValue(device, element, &oldValue);
                        long oldLEDStatus = IOHIDValueGetIntegerValue(oldValue);
                        CFRelease(oldValue);
                        if (debug) printf("  Before was %ld\n", oldLEDStatus);

                        int newLEDStatus = changeLEDs[led] == 1 ? 1 : 0;
                       
                        if (led < maxLEDs) {
                            if (oldLEDStatus > 0)
                                before[led] = 1;

                            if (oldLEDStatus != newLEDStatus) {
                                if (debug) printf("  Should change to %d\n", newLEDStatus);
                                IOHIDValueRef newValue = IOHIDValueCreateWithIntegerValue(kCFAllocatorDefault, element, 0, newLEDStatus);
                                if (newValue) {
                                    if (debug) printf("  Changing to %d\n", newLEDStatus);
                                    after[led] = newLEDStatus;
                                    IOReturn change = IOHIDDeviceSetValue(device, element, newValue);
                                    if (kIOReturnSuccess != change)
                                        printf("  Failed to change, error %ul\n", change);
                                    CFRelease(newValue);
                                }
                            }
                        }
                    }
                }
            }
            CFRelease(elements);
        }
        CFRelease(deviceName);
    }
}

void setAllKeyboards(int changeLEDs[])
{
    IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager) {
        fprintf(stderr, "IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone) failed.");
        return;
    }
    
    CFDictionaryRef keyboard = CreateDictionaryUsagePageUsage(CFSTR(kIOHIDDeviceUsageKey), kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard);
    if (keyboard) {
        if (debug) printf("Obtained keyboard dictionary\n");
        CFDictionaryRef ledsDictionary = CreateDictionaryUsagePageUsage(CFSTR(kIOHIDElementUsageKey), kHIDPage_LEDs, 0);
        if (ledsDictionary) {
            if (debug) printf("Obtained LEDs dictionary\n");
            IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone);
            IOHIDManagerSetDeviceMatching(manager, keyboard);
            if (debug) printf("Manager opened and set to keyboard\n");
            
            CFSetRef devices = IOHIDManagerCopyDevices(manager);
            if (devices) {
                if (debug) printf("Manager devices copied\n");
                CFIndex deviceCount = CFSetGetCount(devices);
                IOHIDDeviceRef *deviceRefs = malloc(sizeof(IOHIDDeviceRef) * deviceCount);
                if (deviceRefs) {
                    if (debug) printf("Device references obtained\n");
                    CFSetGetValues(devices, (const void **) deviceRefs);
                    for (CFIndex deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
                        if (isKeyboardDevice(deviceRefs[deviceIndex]))
                            setKeyboard(deviceRefs[deviceIndex], keyboard, changeLEDs);
                    free(deviceRefs);
                }
                CFRelease(devices);
            }
            CFRelease(ledsDictionary);
        }
        CFRelease(keyboard);
    }
}