#include "prx/libSceJsc/libSceJsc.hpp"
#include "prx/libc/include/Api.hpp"
#include "prx/libc/include/ApplicationHeap.hpp"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <array>
#include <stdexcept>
#include <type_traits>

using Jsc::InspectorAbi::ScriptCallFrame;
using Jsc::InspectorAbi::ScriptCallFrameVector;
using Jsc::InspectorAbi::ScriptCallStack;
using Jsc::WtfAbi::PersistenceAllocator;
using Jsc::WtfAbi::StringImpl;

namespace {

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "ScriptCallStack check failed at line %d\n", line);
        std::abort();
    }
}

#define Require(value) Check((value), __LINE__)

int allocations = 0;
std::size_t lastSize = 0;
int heapObjects = 0;

void* APS5_VABI HeapAllocate(std::size_t size) {
    ++heapObjects;
    return std::malloc(size);
}

void APS5_VABI HeapFree(void* pointer) {
    if (pointer) --heapObjects;
    std::free(pointer);
}

void* APS5_VABI HeapCalloc(std::size_t count, std::size_t size) {
    ++heapObjects;
    return std::calloc(count, size);
}

void* APS5_VABI HeapReallocate(void* pointer, std::size_t size) {
    return std::realloc(pointer, size);
}

void* APS5_VABI HeapAlign(std::size_t, std::size_t size) {
    ++heapObjects;
    return std::malloc(size);
}

void* APS5_VABI HeapRealign(void* pointer, std::size_t size, std::size_t) {
    return std::realloc(pointer, size);
}

int APS5_VABI HeapPosixAlign(void** pointer, std::size_t, std::size_t size) {
    ++heapObjects;
    *pointer = std::malloc(size);
    return *pointer ? 0 : 12;
}

void* TestAllocate(std::size_t size) {
    ++allocations;
    lastSize = size;
    return std::malloc(size);
}

void TestDeallocate(void* pointer) {
    if (!pointer) return;
    --allocations;
    std::free(pointer);
}

template<typename TException, typename TFunction>
bool Throws(TFunction function) {
    try {
        function();
    } catch (const TException&) {
        return true;
    }
    return false;
}

void Construct1(ScriptCallStack* self, ScriptCallFrameVector* frames) {
    _ZN9Inspector15ScriptCallStackC1ERN3WTF6VectorINS_15ScriptCallFrameELm0ENS1_15CrashOnOverflowELm16ENS1_10FastMallocEEE(self, frames);
}

void Construct2(ScriptCallStack* self, ScriptCallFrameVector* frames) {
    _ZN9Inspector15ScriptCallStackC2ERN3WTF6VectorINS_15ScriptCallFrameELm0ENS1_15CrashOnOverflowELm16ENS1_10FastMallocEEE(self, frames);
}

ScriptCallStack** Create(ScriptCallStack** result, ScriptCallFrameVector* frames) {
    return _ZN9Inspector15ScriptCallStack6createERN3WTF6VectorINS_15ScriptCallFrameELm0ENS1_15CrashOnOverflowELm16ENS1_10FastMallocEEE(result, frames);
}

void Append(ScriptCallStack* self, const ScriptCallFrame* frame) {
    _ZN9Inspector15ScriptCallStack6appendERKNS_15ScriptCallFrameE(self, frame);
}

void TestConstructors() {
    ScriptCallFrame storage[3]{};
    for (auto construct : {&Construct1, &Construct2}) {
        ScriptCallFrameVector frames{storage, 3, 2};
        ScriptCallStack stack;
        std::memset(&stack, 0xA5, sizeof(stack));
        errno = 91;
        construct(&stack, &frames);
        Require(errno == 91);
        Require(stack.refCount == 1 && stack.frames.buffer == storage && stack.frames.capacity == 3 && stack.frames.size == 2);
        Require(!frames.buffer && frames.capacity == 0 && frames.size == 0);
        ScriptCallFrameVector empty{};
        construct(&stack, &empty);
        Require(stack.refCount == 1 && !stack.frames.buffer && stack.frames.capacity == 0 && stack.frames.size == 0);
    }
    ScriptCallFrameVector frames{};
    ScriptCallStack stack{};
    Require(Throws<std::invalid_argument>([&] { Construct1(nullptr, &frames); }));
    Require(Throws<std::invalid_argument>([&] { Construct2(&stack, nullptr); }));
    Require(Throws<std::invalid_argument>([&] { Construct1(&stack, reinterpret_cast<ScriptCallFrameVector*>(std::uintptr_t{0x10})); }));
    ScriptCallFrameVector kept{storage, 3, 2};
    Require(Throws<std::invalid_argument>([&] { Construct2(reinterpret_cast<ScriptCallStack*>(std::uintptr_t{0x10}), &kept); }));
    Require(kept.buffer == storage && kept.capacity == 3 && kept.size == 2);
}

void TestCreate() {
    ScriptCallFrame storage[4]{};
    ScriptCallFrameVector frames{storage, 4, 4};
    ScriptCallStack* stack = nullptr;
    Require(Create(&stack, &frames) == &stack);
    Require(stack && stack->refCount == 1 && stack->frames.buffer == storage && stack->frames.capacity == 4 && stack->frames.size == 4);
    Require(!frames.buffer && frames.capacity == 0 && frames.size == 0);
    _ZdlPv_nid_postfix(stack);
    Require(Throws<std::invalid_argument>([&] { Create(nullptr, &frames); }));
    Require(Throws<std::invalid_argument>([&] { Create(&stack, nullptr); }));
    ScriptCallFrameVector kept{storage, 4, 1};
    Require(Throws<std::invalid_argument>([&] { Create(reinterpret_cast<ScriptCallStack**>(std::uintptr_t{0x10}), &kept); }));
    Require(kept.buffer == storage && kept.size == 1);
    Require(heapObjects == 0);
}

void TestAppend() {
    unsigned char functionStorage[32]{};
    unsigned char scriptStorage[32]{};
    auto* function = reinterpret_cast<StringImpl*>(functionStorage);
    auto* script = reinterpret_cast<StringImpl*>(scriptStorage);
    function->refCount = 2;
    script->refCount = 5;

    ScriptCallStack stack{1u, {}};
    const ScriptCallFrame frame{function, script, 42, 7, 9};
    Require(Throws<std::runtime_error>([&] { Append(&stack, &frame); }));
    Require(function->refCount == 2 && !stack.frames.buffer);

    constexpr PersistenceAllocator allocator{&TestAllocate, &TestDeallocate};
    RegisterPersistenceAllocator_nid_no_patch(&allocator);

    errno = 92;
    Append(&stack, &frame);
    Require(errno == 92);
    Require(stack.frames.size == 1 && stack.frames.capacity == 16 && lastSize == 16 * sizeof(ScriptCallFrame));
    Require(std::memcmp(&stack.frames.buffer[0], &frame, sizeof(frame)) == 0);
    Require(function->refCount == 4 && script->refCount == 7);

    const ScriptCallFrame nullNames{nullptr, nullptr, -1, 0, 0};
    for (int i = 1; i < 16; ++i) Append(&stack, &nullNames);
    Require(stack.frames.size == 16 && stack.frames.capacity == 16 && allocations == 1);

    auto* oldBuffer = stack.frames.buffer;
    Append(&stack, &stack.frames.buffer[0]);
    Require(stack.frames.size == 17 && stack.frames.capacity == 21 && stack.frames.buffer != oldBuffer && allocations == 1);
    Require(lastSize == 21 * sizeof(ScriptCallFrame));
    Require(std::memcmp(&stack.frames.buffer[16], &frame, sizeof(frame)) == 0);
    Require(std::memcmp(&stack.frames.buffer[0], &frame, sizeof(frame)) == 0);
    Require(function->refCount == 6 && script->refCount == 9);
    for (int i = 1; i < 16; ++i) Require(std::memcmp(&stack.frames.buffer[i], &nullNames, sizeof(nullNames)) == 0);

    while (stack.frames.size < 21) Append(&stack, &nullNames);
    Append(&stack, &nullNames);
    Require(stack.frames.size == 22 && stack.frames.capacity == 27);
    TestDeallocate(stack.frames.buffer);
    Require(allocations == 0);

    Require(Throws<std::invalid_argument>([&] { Append(nullptr, &frame); }));
    Require(Throws<std::invalid_argument>([&] { Append(&stack, nullptr); }));
    ScriptCallStack corrupt{1u, {nullptr, 4, 0}};
    Require(Throws<std::invalid_argument>([&] { Append(&corrupt, &frame); }));
    ScriptCallStack oversized{1u, {nullptr, 0, 3}};
    Require(Throws<std::invalid_argument>([&] { Append(&oversized, &frame); }));
    ScriptCallStack empty{1u, {}};
    Require(Throws<std::invalid_argument>([&] { Append(&empty, reinterpret_cast<const ScriptCallFrame*>(std::uintptr_t{0x10})); }));
    Require(!empty.frames.buffer && allocations == 0);
}

}

int main() {
    const std::array<void*, 10> heap{
        reinterpret_cast<void*>(&HeapAllocate), reinterpret_cast<void*>(&HeapFree),
        reinterpret_cast<void*>(&HeapCalloc), reinterpret_cast<void*>(&HeapReallocate),
        reinterpret_cast<void*>(&HeapAlign), reinterpret_cast<void*>(&HeapRealign),
        reinterpret_cast<void*>(&HeapPosixAlign), nullptr, nullptr, nullptr};
    ApplicationHeapRegister_nid_no_patch(heap.data());
    TestConstructors();
    TestCreate();
    TestAppend();
    std::puts("ScriptCallStack tests passed");
    return 0;
}
