#pragma once

#include "prx/libSceJsc/src/WtfStringAbi.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstddef>
#include <cstdint>

namespace Jsc::InspectorAbi {

struct ConsoleMessage {
    std::uint8_t source;
    std::uint8_t type;
    std::uint8_t level;
    Jsc::WtfAbi::StringImpl* message;
    void* arguments;
    void* callStack;
    void* jsonLogValuesBuffer;
    unsigned jsonLogValuesCapacity;
    unsigned jsonLogValuesSize;
    Jsc::WtfAbi::StringImpl* url;
    void* globalObjectSlot;
    unsigned line;
    unsigned column;
    unsigned repeatCount;
    Jsc::WtfAbi::StringImpl* requestId;
};

inline constexpr std::uint8_t MessageTypeEndGroup = 7;

static_assert(sizeof(ConsoleMessage) == 88);
static_assert(offsetof(ConsoleMessage, message) == 8);
static_assert(offsetof(ConsoleMessage, jsonLogValuesBuffer) == 32);
static_assert(offsetof(ConsoleMessage, url) == 48);
static_assert(offsetof(ConsoleMessage, globalObjectSlot) == 56);
static_assert(offsetof(ConsoleMessage, line) == 64);
static_assert(offsetof(ConsoleMessage, repeatCount) == 72);
static_assert(offsetof(ConsoleMessage, requestId) == 80);

} // namespace Jsc::InspectorAbi

extern "C" {

void APS5_VABI _ZN9Inspector14ConsoleMessageC1EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelERKN3WTF6StringEm(
    Jsc::InspectorAbi::ConsoleMessage* self, std::uint8_t source, std::uint8_t type, std::uint8_t level,
    Jsc::WtfAbi::StringImpl* const* message, std::uint64_t requestIdentifier);
void APS5_VABI _ZN9Inspector14ConsoleMessageC2EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelERKN3WTF6StringEm(
    Jsc::InspectorAbi::ConsoleMessage* self, std::uint8_t source, std::uint8_t type, std::uint8_t level,
    Jsc::WtfAbi::StringImpl* const* message, std::uint64_t requestIdentifier);
void APS5_VABI _ZN9Inspector14ConsoleMessageC1EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelERKN3WTF6StringES8_jjPNS1_14JSGlobalObjectEm(
    Jsc::InspectorAbi::ConsoleMessage* self, std::uint8_t source, std::uint8_t type, std::uint8_t level,
    Jsc::WtfAbi::StringImpl* const* message, Jsc::WtfAbi::StringImpl* const* url, unsigned line, unsigned column, void* globalObject,
    std::uint64_t requestIdentifier);
void APS5_VABI _ZN9Inspector14ConsoleMessageC2EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelERKN3WTF6StringES8_jjPNS1_14JSGlobalObjectEm(
    Jsc::InspectorAbi::ConsoleMessage* self, std::uint8_t source, std::uint8_t type, std::uint8_t level,
    Jsc::WtfAbi::StringImpl* const* message, Jsc::WtfAbi::StringImpl* const* url, unsigned line, unsigned column, void* globalObject,
    std::uint64_t requestIdentifier);

}
