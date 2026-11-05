// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Media.h"
#if JUCE_MAC
#include <IOKit/hid/IOHIDManager.h>
#endif
namespace fader {
juce::var inspectDevices() {
    auto *root = new juce::DynamicObject();
    juce::var result(root);
    juce::OwnedArray<juce::AudioIODeviceType> types;
    juce::AudioDeviceManager manager;
    manager.createAudioDeviceTypes(types);
    juce::Array<juce::var> audio;
    for (auto *type : types) {
        type->scanForDevices();
        for (const auto &name : type->getDeviceNames(false)) {
            auto *obj = new juce::DynamicObject();
            obj->setProperty("backend", type->getTypeName());
            obj->setProperty("name", name);
            audio.add(juce::var(obj));
        }
    }
    root->setProperty("audioOutputs", audio);
    juce::Array<juce::var> controllers;
#if JUCE_MAC
    const auto hid = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (hid) {
        const int vendor = 0x17cc, product = 0x1720;
        const auto vid = CFNumberCreate(nullptr, kCFNumberIntType, &vendor),
                   pid = CFNumberCreate(nullptr, kCFNumberIntType, &product);
        const void *keys[]{CFSTR(kIOHIDVendorIDKey), CFSTR(kIOHIDProductIDKey)};
        const void *values[]{vid, pid};
        auto match = CFDictionaryCreate(nullptr, keys, values, 2, &kCFTypeDictionaryKeyCallBacks,
                                        &kCFTypeDictionaryValueCallBacks);
        IOHIDManagerSetDeviceMatching(hid, match);
        if (auto devices = IOHIDManagerCopyDevices(hid)) {
            std::vector<const void *> list(static_cast<std::size_t>(CFSetGetCount(devices)));
            CFSetGetValues(devices, list.data());
            for (auto entry : list) {
                const auto device = static_cast<IOHIDDeviceRef>(const_cast<void *>(entry));
                auto *o = new juce::DynamicObject();
                o->setProperty("model", "Traktor Kontrol S4 MK3");
                o->setProperty("vendorId", vendor);
                o->setProperty("productId", product);
                for (const auto &key : {CFSTR(kIOHIDPrimaryUsagePageKey), CFSTR(kIOHIDPrimaryUsageKey),
                                        CFSTR(kIOHIDVersionNumberKey)}) {
                    const auto value = IOHIDDeviceGetProperty(device, key);
                    int integer = 0;
                    if (value && CFGetTypeID(value) == CFNumberGetTypeID() &&
                        CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberIntType, &integer))
                        o->setProperty(juce::String::fromCFString(key), integer);
                }
                controllers.add(juce::var(o));
            }
            CFRelease(devices);
        }
        CFRelease(match);
        CFRelease(vid);
        CFRelease(pid);
        CFRelease(hid);
    }
#endif
    root->setProperty("s4Interfaces", controllers);
    root->setProperty("controllerMapping", "not yet qualified; inventory only");
    return result;
}
} // namespace fader
