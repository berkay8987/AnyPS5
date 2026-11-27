#include "prx/libSceJsc/libSceJsc.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>

static_assert(std::is_same_v<decltype(&_ZN3JSC2VM15dumpRegExpTraceEv), void (APS5_VABI*)(void*)>);

int main() {
    unsigned char vm[256];
    std::memset(vm, 0x6B, sizeof(vm));
    _ZN3JSC2VM15dumpRegExpTraceEv(vm);
    for (const unsigned char byte : vm) {
        if (byte != 0x6B) {
            std::fputs("VM::dumpRegExpTrace changed the VM\n", stderr);
            std::abort();
        }
    }
    _ZN3JSC2VM15dumpRegExpTraceEv(nullptr);
    std::puts("VM dumpRegExpTrace tests passed");
    return 0;
}
