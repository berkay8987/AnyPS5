#include "prx/libSceJsc/libSceJsc.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <type_traits>

using Jsc::WtfAbi::PersistenceAllocator;

static_assert(std::is_same_v<decltype(&_ZN9Inspector14InspectorAgentnaEm), void* (APS5_VABI*)(std::size_t)>);
static_assert(std::is_same_v<decltype(&_ZN9Inspector22InspectorDebuggerAgentnaEm), void* (APS5_VABI*)(std::size_t)>);

namespace {

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "Inspector agent operator new[] check failed at line %d\n", line);
        std::abort();
    }
}

#define Require(value) Check((value), __LINE__)

std::size_t lastSize = 0;
int calls = 0;
bool failNext = false;
alignas(16) unsigned char arena[256];

void* TestAllocate(std::size_t size) {
    ++calls;
    lastSize = size;
    if (failNext) {
        failNext = false;
        return nullptr;
    }
    return arena;
}

void TestDeallocate(void*) {}

template<typename TException, typename TFunction>
bool Throws(TFunction function) {
    try {
        function();
    } catch (const TException&) {
        return true;
    }
    return false;
}

using NewArray = void* (APS5_VABI*)(std::size_t);

void TestForwarding(NewArray function) {
    for (const std::size_t size : {std::size_t{0}, std::size_t{1}, std::size_t{96}, std::size_t{4096}}) {
        const int before = calls;
        Require(function(size) == arena);
        Require(calls == before + 1 && lastSize == size);
    }
    failNext = true;
    Require(Throws<std::bad_alloc>([&] { function(32); }));
}

}

int main() {
    Require(Throws<std::runtime_error>([] { _ZN9Inspector14InspectorAgentnaEm(8); }));
    Require(Throws<std::runtime_error>([] { _ZN9Inspector22InspectorDebuggerAgentnaEm(8); }));
    constexpr PersistenceAllocator allocator{&TestAllocate, &TestDeallocate};
    RegisterPersistenceAllocator_nid_no_patch(&allocator);
    TestForwarding(&_ZN9Inspector14InspectorAgentnaEm);
    TestForwarding(&_ZN9Inspector22InspectorDebuggerAgentnaEm);
    std::puts("Inspector agent operator new[] tests passed");
    return 0;
}
