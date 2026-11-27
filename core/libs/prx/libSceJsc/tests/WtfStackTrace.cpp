#include "prx/libSceJsc/libSceJsc.hpp"
#include "prx/libc/include/Api.hpp"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

using Jsc::WtfAbi::OptionalDemangleEntry;
using Jsc::WtfAbi::PersistenceAllocator;
using Jsc::WtfAbi::StackTrace;

static_assert(std::is_same_v<decltype(&_ZN3WTF10StackTrace17captureStackTraceEii), StackTrace** (APS5_VABI*)(StackTrace**, int, int)>);
static_assert(std::is_same_v<decltype(&_ZN3WTF10StackTrace8demangleEPv), OptionalDemangleEntry* (APS5_VABI*)(OptionalDemangleEntry*, void*)>);
static_assert(std::is_same_v<decltype(&_ZNK3WTF10StackTrace4dumpERNS_11PrintStreamEPKc),
                             void (APS5_VABI*)(const StackTrace*, Jsc::WtfAbi::PrintStream*, const char*)>);

namespace {

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "StackTrace check failed at line %d\n", line);
        std::abort();
    }
}

#define Require(value) Check((value), __LINE__)

int allocations = 0;
std::size_t lastSize = 0;
bool failNext = false;

void* TestAllocate(std::size_t size) {
    lastSize = size;
    if (failNext) {
        failNext = false;
        return nullptr;
    }
    ++allocations;
    return std::malloc(size);
}

void TestDeallocate(void* pointer) {
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

struct TestStream {
    const void* const* vtable;
    std::string* log;
};

void APS5_VABI Unexpected(TestStream*) {
    std::abort();
}

void APS5_VABI Vprintf(TestStream* self, const char* format, VaList* arguments) {
    char buffer[256];
    Require(vsnprintf_nid_postfix(buffer, sizeof(buffer), format, arguments) >= 0);
    *self->log += buffer;
}

const void* const testVtable[] = {
    reinterpret_cast<const void*>(&Unexpected),
    reinterpret_cast<const void*>(&Unexpected),
    reinterpret_cast<const void*>(&Vprintf),
    reinterpret_cast<const void*>(&Unexpected),
    reinterpret_cast<const void*>(&Unexpected),
    reinterpret_cast<const void*>(&Unexpected),
};

std::string Pointer(const void* value) {
    char buffer[64];
    Require(snprintf_nid_postfix(buffer, sizeof(buffer), "%p", value) > 0);
    return buffer;
}

std::string Dump(const StackTrace* trace, const char* indent) {
    std::string log;
    TestStream stream{testVtable, &log};
    _ZNK3WTF10StackTrace4dumpERNS_11PrintStreamEPKc(trace, reinterpret_cast<Jsc::WtfAbi::PrintStream*>(&stream), indent);
    return log;
}

void TestCapture() {
    StackTrace* trace = nullptr;
    Require(Throws<std::runtime_error>([&] { _ZN3WTF10StackTrace17captureStackTraceEii(&trace, 4, 0); }));
    Require(trace == nullptr);
    constexpr PersistenceAllocator allocator{&TestAllocate, &TestDeallocate};
    RegisterPersistenceAllocator_nid_no_patch(&allocator);

    const struct {
        int maxFrames;
        int capacity;
    } cases[] = {{10, 10}, {1, 1}, {0, 1}, {-5, 1}, {256, 256}};
    for (const auto& entry : cases) {
        errno = 81;
        Require(_ZN3WTF10StackTrace17captureStackTraceEii(&trace, entry.maxFrames, 3) == &trace);
        Require(errno == 81);
        Require(trace && trace->size == 0 && trace->capacity == entry.capacity);
        Require(lastSize == sizeof(StackTrace) + std::size_t(entry.capacity - 1) * sizeof(void*));
        Require(Dump(trace, "  ").empty());
        TestDeallocate(trace);
        trace = nullptr;
    }

    failNext = true;
    Require(Throws<std::bad_alloc>([&] { _ZN3WTF10StackTrace17captureStackTraceEii(&trace, std::numeric_limits<int>::max(), 0); }));
    Require(lastSize == sizeof(StackTrace) + std::size_t(std::numeric_limits<int>::max() - 1) * sizeof(void*));
    Require(Throws<std::invalid_argument>([] { _ZN3WTF10StackTrace17captureStackTraceEii(nullptr, 1, 0); }));
    const int before = allocations;
    auto* unwritable = reinterpret_cast<StackTrace**>(std::uintptr_t{0x10});
    Require(Throws<std::invalid_argument>([&] { _ZN3WTF10StackTrace17captureStackTraceEii(unwritable, 1, 0); }));
    Require(allocations == before && trace == nullptr);
}

void TestDemangle() {
    OptionalDemangleEntry entry{true, "a", "b"};
    int local = 0;
    errno = 82;
    Require(_ZN3WTF10StackTrace8demangleEPv(&entry, &local) == &entry);
    Require(errno == 82);
    Require(!entry.hasValue);
    entry.hasValue = true;
    Require(_ZN3WTF10StackTrace8demangleEPv(&entry, nullptr) == &entry && !entry.hasValue);
    Require(Throws<std::invalid_argument>([] { _ZN3WTF10StackTrace8demangleEPv(nullptr, nullptr); }));
    Require(Throws<std::invalid_argument>([] {
        _ZN3WTF10StackTrace8demangleEPv(reinterpret_cast<OptionalDemangleEntry*>(std::uintptr_t{0x10}), nullptr);
    }));
}

void TestDumpOwned() {
    std::vector<unsigned char> storage(sizeof(StackTrace) + 2 * sizeof(void*));
    auto* trace = reinterpret_cast<StackTrace*>(storage.data());
    void* frames[3] = {reinterpret_cast<void*>(std::uintptr_t{0x1000}), reinterpret_cast<void*>(std::uintptr_t{0xdeadbeef}), nullptr};
    trace->size = 3;
    trace->capacity = 3;
    std::memcpy(&trace->stackOrBorrowed, frames, sizeof(frames));
    const std::string expected = "1   " + Pointer(frames[0]) + "\n2   " + Pointer(frames[1]) + "\n3   " + Pointer(frames[2]) + "\n";
    Require(Dump(trace, nullptr) == expected);
    Require(Dump(trace, "") == expected);
    Require(Dump(trace, ">>") == ">>1   " + Pointer(frames[0]) + "\n>>2   " + Pointer(frames[1]) + "\n>>3   " + Pointer(frames[2]) + "\n");
    trace->size = 2;
    Require(Dump(trace, nullptr) == "1   " + Pointer(frames[0]) + "\n2   " + Pointer(frames[1]) + "\n");
}

void TestDumpBorrowedAndWide() {
    std::vector<void*> frames(12);
    for (std::size_t i = 0; i < frames.size(); ++i) frames[i] = reinterpret_cast<void*>(std::uintptr_t{0x100} * (i + 1));
    StackTrace trace{static_cast<int>(frames.size()), 0, nullptr, frames.data()};
    const std::string log = Dump(&trace, " ");
    std::string expected;
    for (std::size_t i = 0; i < frames.size(); ++i) {
        char number[16];
        std::snprintf(number, sizeof(number), "%-3d", static_cast<int>(i + 1));
        expected += std::string(" ") + number + " " + Pointer(frames[i]) + "\n";
    }
    Require(log == expected);
    Require(log.find(" 10  ") != std::string::npos);
}

void TestDumpInvalid() {
    StackTrace empty{0, 0, nullptr, nullptr};
    Require(Dump(&empty, nullptr).empty());
    StackTrace negative{-4, 0, nullptr, nullptr};
    Require(Dump(&negative, nullptr).empty());
    _ZNK3WTF10StackTrace4dumpERNS_11PrintStreamEPKc(&empty, nullptr, nullptr);
    Require(Throws<std::invalid_argument>([] { Dump(nullptr, nullptr); }));
    Require(Throws<std::invalid_argument>([] { Dump(reinterpret_cast<const StackTrace*>(std::uintptr_t{0x10}), nullptr); }));
    StackTrace borrowedNull{2, 0, nullptr, nullptr};
    Require(Throws<std::invalid_argument>([&] { Dump(&borrowedNull, nullptr); }));
    void* frame = nullptr;
    StackTrace oneFrame{1, 0, nullptr, &frame};
    Require(Throws<std::invalid_argument>([&] {
        _ZNK3WTF10StackTrace4dumpERNS_11PrintStreamEPKc(&oneFrame, nullptr, nullptr);
    }));
}

}

int main() {
    TestCapture();
    TestDemangle();
    TestDumpOwned();
    TestDumpBorrowedAndWide();
    TestDumpInvalid();
    Require(allocations == 0);
    std::puts("StackTrace tests passed");
    return 0;
}
