/*  setleds for Mac
    http://github.com/damieng/setledsmac
    Copyright Damien Guard. GPL 2 licence 
 */

#include "main.h"

Boolean verbose = false;

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
    
    int setLEDs = 0;
    int unsetLEDs = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0)
            verbose = true;
        else if (strcmp(argv[i], "+num") == 0)
            setLEDs |= NUM;
        else if (strcmp(argv[i], "-num") == 0)
            unsetLEDs |= NUM;
        else if (strcmp(argv[i], "+caps") == 0)
            setLEDs |= CAPS;
        else if (strcmp(argv[i], "-caps") == 0)
            unsetLEDs |= CAPS;
        else if (strcmp(argv[i], "+scroll") == 0)
            setLEDs |= SCROLL;
        else if (strcmp(argv[i], "-scroll") == 0)
            unsetLEDs |= SCROLL;
        else {
            fprintf(stderr, "Unknown option %s\n\n", argv[i]);
            explainUsage();
            exit(1);
        }
    }
    
    setAllKeyboards(setLEDs, unsetLEDs);
}

void explainUsage() {
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

void setKeyboard(struct __IOHIDDevice *device, CFDictionaryRef keyboardDictionary, int setLEDs, int clearLEDs)
{
    CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, keyboardDictionary, kIOHIDOptionsTypeNone);
    if (!elements)
        return;
    
    CFStringRef deviceName = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));

    CFIndex elementCount = CFArrayGetCount(elements);
    for (CFIndex elementIndex = 0; elementIndex < elementCount; elementIndex++) {
        IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, elementIndex);
        if (element) {
            uint32_t usage = IOHIDElementGetUsage(element);
            uint32_t usagePage = IOHIDElementGetUsagePage(element);
            if (kHIDPage_LEDs == usagePage) {
                // Get current keyboard led status
                IOHIDValueRef value = 0;
                IOHIDDeviceGetValue(device, element, &value);
                long oldLEDs = IOHIDValueGetIntegerValue(value);

                // Set and clear necessary leds
                long newLEDs = (oldLEDs & ~clearLEDs) | setLEDs;
                Boolean change = (newLEDs != oldLEDs);
                
                if (verbose || change)
                    printf("%s [%i] was %lu",
                           CFStringGetCStringPtr(deviceName, kCFStringEncodingUTF8),
                           usage,
                           oldLEDs);
                
                if (change) {
                    value = IOHIDValueCreateWithIntegerValue(kCFAllocatorDefault, element, 0, newLEDs);
                    if (value) {
                        printf(" now set to %lu", newLEDs);
                        if (kIOReturnSuccess != IOHIDDeviceSetValue(device, element, value))
                            printf(" #FAILED");
                        CFRelease(value);
                    }
                }
                
                if (verbose || change)
                    printf("\n");
            }
        }
    }
    
    CFRelease(deviceName);
    CFRelease(elements);
}

void setAllKeyboards(int setLEDs, int clearLEDs)
{
    IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager) {
        fprintf(stderr, "IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone) failed.");
        return;
    }
    
    IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone);
    CFDictionaryRef keyboard = CreateDictionaryUsagePageUsage(CFSTR(kIOHIDDeviceUsageKey), kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard);
    if (keyboard) {
        IOHIDManagerSetDeviceMatching(manager, keyboard);
        CFRelease(keyboard);
        
        CFSetRef devices = IOHIDManagerCopyDevices(manager);
        if (devices) {
            CFIndex deviceCount = CFSetGetCount(devices);
            IOHIDDeviceRef *deviceRefs = malloc(sizeof(IOHIDDeviceRef) * deviceCount);
            if (deviceRefs) {
                CFSetGetValues(devices, (const void **) deviceRefs);
                CFDictionaryRef ledsDictionary = CreateDictionaryUsagePageUsage(CFSTR(kIOHIDElementUsageKey), kHIDPage_LEDs, 0);
                if (ledsDictionary) {
                    for (CFIndex deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
                        if (isKeyboardDevice(deviceRefs[deviceIndex]))
                            setKeyboard(deviceRefs[deviceIndex], keyboard, setLEDs, clearLEDs);
                    CFRelease(ledsDictionary);
                }
                free(deviceRefs);
            }
            CFRelease(devices);
        }
    }
    
    IOHIDManagerClose(manager, kIOHIDOptionsTypeNone);
    CFRelease(manager);
}