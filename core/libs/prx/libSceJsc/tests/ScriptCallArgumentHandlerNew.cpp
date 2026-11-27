#include "prx/libSceJsc/libSceJsc.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <type_traits>

static_assert(std::is_same_v<decltype(&_ZN10Deprecated25ScriptCallArgumentHandlernwEm), void* (APS5_VABI*)(std::size_t)>);
static_assert(std::is_same_v<decltype(&_ZN10Deprecated25ScriptCallArgumentHandlernaEm), void* (APS5_VABI*)(std::size_t)>);

namespace {

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "ScriptCallArgumentHandler operator new check failed at line %d\n", line);
        std::abort();
    }
}

#define Require(value) Check((value), __LINE__)

}

int main() {
    const auto badBeef = reinterpret_cast<void*>(std::uintptr_t{0xbadbeef});
    for (const std::size_t size : {std::size_t{0}, std::size_t{1}, std::size_t{104}, std::size_t{4096},
             std::numeric_limits<std::size_t>::max()}) {
        Require(_ZN10Deprecated25ScriptCallArgumentHandlernwEm(size) == badBeef);
        Require(_ZN10Deprecated25ScriptCallArgumentHandlernaEm(size) == badBeef);
    }
    std::puts("ScriptCallArgumentHandler operator new tests passed");
    return 0;
}
