#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceNKWebKit/libSceNKWebKit.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "GraphicsLayerTransform check failed at line %d\n", line);
        std::abort();
    }
}
#define Require(value) Check((value), __LINE__)

constexpr std::size_t Size = 544;

template<typename T>
T Read(const unsigned char* storage, std::size_t offset) {
    T value;
    std::memcpy(&value, storage + offset, sizeof(value));
    return value;
}

bool IsIdentity(const unsigned char* storage, std::size_t offset) {
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            const double value = Read<double>(storage, offset + (row * 4 + column) * sizeof(double));
            if (value != (row == column ? 1.0 : 0.0)) return false;
        }
    }
    return true;
}

void RequireDefault(const unsigned char* storage) {
    Require(Read<float>(storage, 0) == 0.5f);
    Require(Read<float>(storage, 4) == 0.5f);
    Require(Read<float>(storage, 8) == 0.0f);
    for (std::size_t offset = 12; offset < 28; offset += 4) Require(Read<float>(storage, offset) == 0.0f);
    Require(storage[28] == 1);
    Require(storage[29] == 0);
    Require(storage[30] == 0);
    for (std::size_t offset : {32u, 160u, 288u, 416u}) Require(IsIdentity(storage, offset));
}

void TestConstructors() {
    alignas(16) unsigned char storage[Size + 16];
    for (auto construct : {&_ZN7WebCore22GraphicsLayerTransformC1Ev, &_ZN7WebCore22GraphicsLayerTransformC2Ev}) {
        std::memset(storage, 0xa5, sizeof(storage));
        construct(storage);
        RequireDefault(storage);
        for (std::size_t offset = Size; offset < sizeof(storage); ++offset) Require(storage[offset] == 0xa5);
    }
    bool thrown = false;
    try {
        _ZN7WebCore22GraphicsLayerTransformC1Ev(nullptr);
    } catch (const std::invalid_argument&) {
        thrown = true;
    }
    Require(thrown);
}

}

int main() {
    TestConstructors();
    return 0;
}
