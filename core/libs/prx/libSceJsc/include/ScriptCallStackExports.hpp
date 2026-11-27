#pragma once

#include "prx/libSceJsc/src/WtfStringAbi.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstddef>
#include <cstdint>

namespace Jsc::InspectorAbi {

struct ScriptCallFrame {
    Jsc::WtfAbi::StringImpl* functionName;
    Jsc::WtfAbi::StringImpl* scriptName;
    std::intptr_t sourceID;
    unsigned lineNumber;
    unsigned column;
};

struct ScriptCallFrameVector {
    ScriptCallFrame* buffer;
    unsigned capacity;
    unsigned size;
};

struct ScriptCallStack {
    unsigned refCount;
    ScriptCallFrameVector frames;
};

static_assert(sizeof(ScriptCallFrame) == 32);
static_assert(offsetof(ScriptCallFrame, sourceID) == 16);
static_assert(offsetof(ScriptCallFrame, lineNumber) == 24);
static_assert(offsetof(ScriptCallFrame, column) == 28);
static_assert(sizeof(ScriptCallFrameVector) == 16);
static_assert(sizeof(ScriptCallStack) == 24);
static_assert(offsetof(ScriptCallStack, frames) == 8);

} // namespace Jsc::InspectorAbi

extern "C" {

void APS5_VABI _ZN9Inspector15ScriptCallStackC1ERN3WTF6VectorINS_15ScriptCallFrameELm0ENS1_15CrashOnOverflowELm16ENS1_10FastMallocEEE(
    Jsc::InspectorAbi::ScriptCallStack* self, Jsc::InspectorAbi::ScriptCallFrameVector* frames);
void APS5_VABI _ZN9Inspector15ScriptCallStackC2ERN3WTF6VectorINS_15ScriptCallFrameELm0ENS1_15CrashOnOverflowELm16ENS1_10FastMallocEEE(
    Jsc::InspectorAbi::ScriptCallStack* self, Jsc::InspectorAbi::ScriptCallFrameVector* frames);
Jsc::InspectorAbi::ScriptCallStack** APS5_VABI
_ZN9Inspector15ScriptCallStack6createERN3WTF6VectorINS_15ScriptCallFrameELm0ENS1_15CrashOnOverflowELm16ENS1_10FastMallocEEE(
    Jsc::InspectorAbi::ScriptCallStack** result, Jsc::InspectorAbi::ScriptCallFrameVector* frames);
void APS5_VABI _ZN9Inspector15ScriptCallStack6appendERKNS_15ScriptCallFrameE(Jsc::InspectorAbi::ScriptCallStack* self,
    const Jsc::InspectorAbi::ScriptCallFrame* frame);

}
