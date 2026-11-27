#include "prx/libSceJsc/libSceJsc.hpp"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>

using Jsc::InspectorAbi::ConsoleMessage;
using Jsc::WtfAbi::PersistenceAllocator;
using Jsc::WtfAbi::StringImpl;

namespace {

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "ConsoleMessage check failed at line %d\n", line);
        std::abort();
    }
}

#define Require(value) Check((value), __LINE__)

int allocations = 0;

void* TestAllocate(std::size_t size) {
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

using Short = void (APS5_VABI*)(ConsoleMessage*, std::uint8_t, std::uint8_t, std::uint8_t, StringImpl* const*, std::uint64_t);
using Long = void (APS5_VABI*)(ConsoleMessage*, std::uint8_t, std::uint8_t, std::uint8_t, StringImpl* const*, StringImpl* const*, unsigned,
    unsigned, void*, std::uint64_t);

const Short shortConstructors[] = {
    &_ZN9Inspector14ConsoleMessageC1EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelERKN3WTF6StringEm,
    &_ZN9Inspector14ConsoleMessageC2EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelERKN3WTF6StringEm,
};

const Long longConstructors[] = {
    &_ZN9Inspector14ConsoleMessageC1EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelERKN3WTF6StringES8_jjPNS1_14JSGlobalObjectEm,
    &_ZN9Inspector14ConsoleMessageC2EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelERKN3WTF6StringES8_jjPNS1_14JSGlobalObjectEm,
};

struct FakeString {
    StringImpl impl;
    char characters[16];

    explicit FakeString(const char* text) {
        const auto length = static_cast<unsigned>(std::strlen(text));
        std::memcpy(characters, text, length);
        impl = StringImpl{2u, length, characters, Jsc::WtfAbi::StringImplFlag8Bit};
    }
};

std::string Text(const StringImpl* string) {
    return std::string(static_cast<const char*>(string->data), string->length);
}

ConsoleMessage Garbage() {
    ConsoleMessage message;
    std::memset(&message, 0xCD, sizeof(message));
    return message;
}

void RequireDefaults(const ConsoleMessage& message) {
    Require(!message.arguments && !message.callStack && !message.jsonLogValuesBuffer);
    Require(message.jsonLogValuesCapacity == 0 && message.jsonLogValuesSize == 0 && !message.globalObjectSlot);
    Require(message.repeatCount == 1);
}

void TestShort() {
    for (const auto construct : shortConstructors) {
        FakeString text("hello");
        StringImpl* message = &text.impl;
        ConsoleMessage value = Garbage();
        errno = 93;
        construct(&value, 3, 1, 2, &message, 0);
        Require(errno == 93);
        Require(value.source == 3 && value.type == 1 && value.level == 2);
        Require(value.message == &text.impl && text.impl.refCount == 4);
        Require(!value.url && value.line == 0 && value.column == 0 && !value.requestId);
        RequireDefaults(value);

        ConsoleMessage withId = Garbage();
        construct(&withId, 0, 0, 0, &message, 42);
        Require(withId.requestId && Text(withId.requestId) == "0.42" && withId.requestId->refCount == 2);
        Require(withId.requestId->hashAndFlags & Jsc::WtfAbi::StringImplFlag8Bit);
        Require(text.impl.refCount == 6);
        TestDeallocate(withId.requestId);

        ConsoleMessage large = Garbage();
        construct(&large, 0, 0, 0, &message, 18446744073709551615ull);
        Require(Text(large.requestId) == "0.18446744073709551615");
        TestDeallocate(large.requestId);

        StringImpl* nullMessage = nullptr;
        ConsoleMessage empty = Garbage();
        construct(&empty, 0, 0, 0, &nullMessage, 0);
        Require(!empty.message && !empty.requestId);
        RequireDefaults(empty);
    }
}

void TestLong() {
    for (const auto construct : longConstructors) {
        FakeString text("warn");
        FakeString source("file.js");
        StringImpl* message = &text.impl;
        StringImpl* url = &source.impl;
        ConsoleMessage value = Garbage();
        construct(&value, 1, 0, 1, &message, &url, 12, 34, nullptr, 7);
        Require(value.message == &text.impl && value.url == &source.impl && text.impl.refCount == 4 && source.impl.refCount == 4);
        Require(value.line == 12 && value.column == 34 && Text(value.requestId) == "0.7");
        RequireDefaults(value);
        TestDeallocate(value.requestId);

        int globalObject = 0;
        ConsoleMessage endGroup = Garbage();
        construct(&endGroup, 0, Jsc::InspectorAbi::MessageTypeEndGroup, 0, &message, &url, 1, 2, &globalObject, 0);
        Require(endGroup.type == Jsc::InspectorAbi::MessageTypeEndGroup && endGroup.line == 1 && endGroup.column == 2 && !endGroup.requestId);
        Require(text.impl.refCount == 6 && source.impl.refCount == 6);

        ConsoleMessage untouched = Garbage();
        const ConsoleMessage before = untouched;
        Require(Throws<std::runtime_error>([&] { construct(&untouched, 0, 0, 0, &message, &url, 1, 2, &globalObject, 5); }));
        Require(std::memcmp(&untouched, &before, sizeof(before)) == 0 && text.impl.refCount == 6 && allocations == 0);

        Require(Throws<std::invalid_argument>([&] { construct(&untouched, 0, 0, 0, &message, nullptr, 0, 0, nullptr, 0); }));
    }
}

void TestInvalid() {
    FakeString text("x");
    StringImpl* message = &text.impl;
    ConsoleMessage value = Garbage();
    errno = 94;
    Require(Throws<std::invalid_argument>([&] { shortConstructors[0](nullptr, 0, 0, 0, &message, 0); }));
    Require(Throws<std::invalid_argument>([&] { shortConstructors[1](&value, 0, 0, 0, nullptr, 0); }));
    Require(Throws<std::invalid_argument>([&] {
        shortConstructors[0](&value, 0, 0, 0, reinterpret_cast<StringImpl* const*>(std::uintptr_t{0x10}), 0);
    }));
    StringImpl* bad = reinterpret_cast<StringImpl*>(std::uintptr_t{0x20});
    Require(Throws<std::invalid_argument>([&] { shortConstructors[0](&value, 0, 0, 0, &bad, 0); }));
    Require(Throws<std::invalid_argument>([&] {
        shortConstructors[0](reinterpret_cast<ConsoleMessage*>(std::uintptr_t{0x10}), 0, 0, 0, &message, 9);
    }));
    Require(errno == 94 && text.impl.refCount == 2 && allocations == 0);
}

}

int main() {
    FakeString text("x");
    StringImpl* message = &text.impl;
    ConsoleMessage value = Garbage();
    Require(Throws<std::runtime_error>([&] { shortConstructors[0](&value, 0, 0, 0, &message, 1); }));
    shortConstructors[0](&value, 0, 0, 0, &message, 0);
    Require(value.message == &text.impl && !value.requestId);

    constexpr PersistenceAllocator allocator{&TestAllocate, &TestDeallocate};
    RegisterPersistenceAllocator_nid_no_patch(&allocator);
    TestShort();
    TestLong();
    TestInvalid();
    Require(allocations == 0);
    std::puts("ConsoleMessage tests passed");
    return 0;
}
