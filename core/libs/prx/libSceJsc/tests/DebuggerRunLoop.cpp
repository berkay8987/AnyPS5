#include "prx/libSceJsc/libSceJsc.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <type_traits>

static_assert(std::is_same_v<decltype(&JSGlobalContextSetDebuggerRunLoopWithCurrentRunLoop), void (APS5_VABI*)(const void*)>);
static_assert(std::is_same_v<decltype(&JSGlobalContextUnsetDebuggerRunLoop), void (APS5_VABI*)(const void*)>);

namespace {

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "debugger run loop check failed at line %d\n", line);
        std::abort();
    }
}

#define Require(value) Check((value), __LINE__)

bool Unchanged(const unsigned char* context, std::size_t size) {
    for (std::size_t i = 0; i < size; ++i)
        if (context[i] != 0x3C) return false;
    return true;
}

}

int main() {
    JSGlobalContextSetDebuggerRunLoopWithCurrentRunLoop(nullptr);
    JSGlobalContextUnsetDebuggerRunLoop(nullptr);
    unsigned char context[128];
    std::memset(context, 0x3C, sizeof(context));
    JSGlobalContextSetDebuggerRunLoopWithCurrentRunLoop(context);
    Require(Unchanged(context, sizeof(context)));
    std::thread other([&] { JSGlobalContextSetDebuggerRunLoopWithCurrentRunLoop(context); });
    other.join();
    JSGlobalContextUnsetDebuggerRunLoop(context);
    JSGlobalContextUnsetDebuggerRunLoop(context);
    Require(Unchanged(context, sizeof(context)));
    std::puts("debugger run loop tests passed");
    return 0;
}
