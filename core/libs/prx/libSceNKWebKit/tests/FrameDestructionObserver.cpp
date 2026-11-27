#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceNKWebKit/libSceNKWebKit.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "FrameDestructionObserver check failed at line %d\n", line);
        std::abort();
    }
}
#define Require(value) Check((value), __LINE__)

struct Observer {
    const void* vtable;
    void* frame;
    std::uint64_t tail;
};

void TestFrameDestroyed() {
    const auto* vtable = reinterpret_cast<const void*>(std::uintptr_t{0x1000});
    auto* frame = reinterpret_cast<void*>(std::uintptr_t{0x2000});
    Observer observer{vtable, frame, 0xa5a5a5a5a5a5a5a5ULL};
    _ZN7WebCore24FrameDestructionObserver14frameDestroyedEv(&observer);
    Require(observer.vtable == vtable);
    Require(observer.frame == nullptr);
    Require(observer.tail == 0xa5a5a5a5a5a5a5a5ULL);
    _ZN7WebCore24FrameDestructionObserver14frameDestroyedEv(&observer);
    Require(observer.frame == nullptr);
    Require(observer.vtable == vtable);

    bool thrown = false;
    try {
        _ZN7WebCore24FrameDestructionObserver14frameDestroyedEv(nullptr);
    } catch (const std::invalid_argument&) {
        thrown = true;
    }
    Require(thrown);
}

}

int main() {
    TestFrameDestroyed();
    return 0;
}
