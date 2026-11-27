#pragma once

#include "prx/libSceJsc/include/WtfDataLogExports.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstddef>

namespace Jsc::WtfAbi {

struct StackTrace {
    int size;
    int capacity;
    void* skippedFrame1;
    void* stackOrBorrowed;
};

struct OptionalDemangleEntry {
    bool hasValue;
    const char* mangledName;
    const char* demangledName;
};

static_assert(sizeof(StackTrace) == 24);
static_assert(offsetof(StackTrace, capacity) == 4);
static_assert(offsetof(StackTrace, stackOrBorrowed) == 16);
static_assert(sizeof(OptionalDemangleEntry) == 24);
static_assert(offsetof(OptionalDemangleEntry, mangledName) == 8);

} // namespace Jsc::WtfAbi

extern "C" {

Jsc::WtfAbi::StackTrace** APS5_VABI _ZN3WTF10StackTrace17captureStackTraceEii(Jsc::WtfAbi::StackTrace** result, int maxFrames, int framesToSkip);
Jsc::WtfAbi::OptionalDemangleEntry* APS5_VABI _ZN3WTF10StackTrace8demangleEPv(Jsc::WtfAbi::OptionalDemangleEntry* result, void* pc);
void APS5_VABI _ZNK3WTF10StackTrace4dumpERNS_11PrintStreamEPKc(const Jsc::WtfAbi::StackTrace* self, Jsc::WtfAbi::PrintStream* out, const char* indentString);

}
