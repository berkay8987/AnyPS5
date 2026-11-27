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

using Json = void (APS5_VABI*)(ConsoleMessage*, std::uint8_t, std::uint8_t, std::uint8_t, Jsc::InspectorAbi::JSONLogValueVector*, void*,
    std::uint64_t);

const Json jsonConstructors[] = {
    &_ZN9Inspector14ConsoleMessageC1EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelEON3WTF6VectorINS5_12JSONLogValueELm0ENS5_15CrashOnOverflowELm16ENS5_10FastMallocEEEPNS1_14JSGlobalObjectEm,
    &_ZN9Inspector14ConsoleMessageC2EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelEON3WTF6VectorINS5_12JSONLogValueELm0ENS5_15CrashOnOverflowELm16ENS5_10FastMallocEEEPNS1_14JSGlobalObjectEm,
};

using Jsc::InspectorAbi::JSONLogValue;
using Jsc::InspectorAbi::JSONLogValueType;
using Jsc::InspectorAbi::JSONLogValueVector;

std::string JsonText(const char* pattern) {
    std::string text;
    for (; *pattern; ++pattern) text += *pattern == '|' ? char(0x5C) : *pattern == '~' ? char(0x22) : *pattern;
    return text;
}

void FreeJson(ConsoleMessage& message) {
    auto* values = static_cast<JSONLogValue*>(message.jsonLogValuesBuffer);
    for (unsigned i = 0; i < message.jsonLogValuesSize; ++i)
        if (values[i].type == JSONLogValueType::String) TestDeallocate(values[i].value);
    if (values) TestDeallocate(values);
    if (message.requestId) TestDeallocate(message.requestId);
}

void TestJson() {
    for (const auto construct : jsonConstructors) {
        FakeString hello("hello");
        FakeString world(" world");
        JSONLogValue twoStrings[] = {{JSONLogValueType::String, &hello.impl}, {JSONLogValueType::String, &world.impl}};
        JSONLogValueVector vector{twoStrings, 2, 2};
        ConsoleMessage value = Garbage();
        errno = 95;
        construct(&value, 2, 3, 1, &vector, nullptr, 11);
        Require(errno == 95);
        Require(value.source == 2 && value.type == 3 && value.level == 1 && value.repeatCount == 1);
        Require(value.jsonLogValuesCapacity == 2 && value.jsonLogValuesSize == 1);
        auto* values = static_cast<JSONLogValue*>(value.jsonLogValuesBuffer);
        Require(values[0].type == JSONLogValueType::String && Text(values[0].value) == JsonText("~hello world~"));
        Require(values[0].value->refCount == 4 && (values[0].value->hashAndFlags & Jsc::WtfAbi::StringImplFlag8Bit));
        Require(value.message == values[0].value && !value.url && !value.arguments && !value.callStack && !value.globalObjectSlot);
        Require(Text(value.requestId) == "0.11" && hello.impl.refCount == 2 && world.impl.refCount == 2);
        Require(vector.buffer == twoStrings && vector.size == 2);
        FreeJson(value);

        FakeString a("a");
        FakeString object("{1}");
        FakeString tags("<b>");
        const char16_t wideUnits[] = {0x0A, 0x3B1};
        StringImpl wide{2u, 2u, wideUnits, 0u};
        JSONLogValue mixed[] = {{JSONLogValueType::String, &a.impl}, {JSONLogValueType::JSON, &object.impl},
            {JSONLogValueType::String, &tags.impl}, {JSONLogValueType::String, &wide}, {JSONLogValueType::String, nullptr}};
        JSONLogValueVector mixedVector{mixed, 5, 5};
        ConsoleMessage mixedMessage = Garbage();
        construct(&mixedMessage, 0, 0, 0, &mixedVector, nullptr, 0);
        values = static_cast<JSONLogValue*>(mixedMessage.jsonLogValuesBuffer);
        Require(mixedMessage.jsonLogValuesCapacity == 5 && mixedMessage.jsonLogValuesSize == 3 && !mixedMessage.requestId);
        Require(values[0].type == JSONLogValueType::String && Text(values[0].value) == JsonText("~a~"));
        Require(values[1].type == JSONLogValueType::JSON && values[1].value == &object.impl && object.impl.refCount == 4);
        Require(values[2].type == JSONLogValueType::String && Text(values[2].value) == JsonText("~|u003Cb|u003E|n|u03B1~"));
        Require(mixedMessage.message == values[0].value && values[0].value->refCount == 4 && values[2].value->refCount == 2);
        FreeJson(mixedMessage);

        FakeString first("[0]");
        JSONLogValue jsonFirst[] = {{JSONLogValueType::JSON, &first.impl}};
        JSONLogValueVector jsonVector{jsonFirst, 1, 1};
        ConsoleMessage jsonMessage = Garbage();
        construct(&jsonMessage, 0, 0, 0, &jsonVector, nullptr, 0);
        Require(jsonMessage.message == &first.impl && first.impl.refCount == 6 && jsonMessage.jsonLogValuesSize == 1);
        FreeJson(jsonMessage);

        JSONLogValueVector emptyVector{};
        ConsoleMessage emptyMessage = Garbage();
        construct(&emptyMessage, 0, 0, 0, &emptyVector, nullptr, 0);
        Require(!emptyMessage.message && !emptyMessage.jsonLogValuesBuffer && emptyMessage.jsonLogValuesCapacity == 0);
        RequireDefaults(emptyMessage);

        FakeString blank("");
        JSONLogValue blankValues[] = {{JSONLogValueType::String, &blank.impl}};
        JSONLogValueVector blankVector{blankValues, 1, 1};
        ConsoleMessage blankMessage = Garbage();
        construct(&blankMessage, 0, 0, 0, &blankVector, nullptr, 0);
        Require(!blankMessage.message && blankMessage.jsonLogValuesBuffer && blankMessage.jsonLogValuesCapacity == 1);
        Require(blankMessage.jsonLogValuesSize == 0);
        FreeJson(blankMessage);

        int globalObject = 0;
        ConsoleMessage untouched = Garbage();
        const ConsoleMessage before = untouched;
        Require(Throws<std::runtime_error>([&] { construct(&untouched, 0, 0, 0, &mixedVector, &globalObject, 1); }));
        JSONLogValue badType[] = {{static_cast<JSONLogValueType>(5), &a.impl}};
        JSONLogValueVector badVector{badType, 1, 1};
        Require(Throws<std::invalid_argument>([&] { construct(&untouched, 0, 0, 0, &badVector, nullptr, 0); }));
        JSONLogValueVector corrupt{nullptr, 0, 2};
        Require(Throws<std::invalid_argument>([&] { construct(&untouched, 0, 0, 0, &corrupt, nullptr, 0); }));
        Require(Throws<std::invalid_argument>([&] { construct(&untouched, 0, 0, 0, nullptr, nullptr, 0); }));
        Require(Throws<std::invalid_argument>([&] { construct(nullptr, 0, 0, 0, &mixedVector, nullptr, 0); }));
        Require(Throws<std::invalid_argument>([&] {
            construct(reinterpret_cast<ConsoleMessage*>(std::uintptr_t{0x10}), 0, 0, 0, &mixedVector, nullptr, 3);
        }));
        Require(std::memcmp(&untouched, &before, sizeof(before)) == 0);
        Require(object.impl.refCount == 4 && a.impl.refCount == 2 && allocations == 0);
    }
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
    TestJson();
    Require(allocations == 0);
    std::puts("ConsoleMessage tests passed");
    return 0;
}
