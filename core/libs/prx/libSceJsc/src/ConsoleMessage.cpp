/*
 * Copyright (C) 2007, 2008, 2014, 2015 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
 * Copyright (C) 2009, 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Derived from Source/JavaScriptCore/inspector/ConsoleMessage.cpp@c55364d53f971f63a709976f168d932d558b8909
 *
 * Copyright (C) 2014-2018 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Derived from Source/JavaScriptCore/inspector/IdentifiersFactory.cpp@c55364d53f971f63a709976f168d932d558b8909
 *
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2014 University of Washington. All rights reserved.
 * Copyright (C) 2017-2019 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Derived from Source/WTF/wtf/JSONValues.cpp@c55364d53f971f63a709976f168d932d558b8909
 */

#include "prx/libSceJsc/include/ConsoleMessageExports.hpp"
#include "prx/libSceJsc/src/WtfFastMalloc.hpp"
#include "prx/libc/include/General.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Jsc::InspectorAbi::ConsoleMessage;
using Jsc::WtfAbi::StringImpl;

constexpr unsigned RefCountIncrement = 2;

template<typename T>
T ReadGuest(const T* source, const char* what) {
    T value{};
    const int savedErrno = errno;
    const bool copied = GuestCopyIn_nid_no_patch(&value, source, sizeof(value));
    errno = savedErrno;
    if (!copied) throw std::invalid_argument(what);
    return value;
}

template<typename T>
void WriteGuest(T* destination, const T& value, const char* what) {
    const int savedErrno = errno;
    const bool copied = GuestCopyOut_nid_no_patch(destination, &value, sizeof(value));
    errno = savedErrno;
    if (!copied) throw std::invalid_argument(what);
}

StringImpl* ReadString(StringImpl* const* string, const char* what) {
    if (!string) throw std::invalid_argument(what);
    StringImpl* impl = ReadGuest(string, what);
    if (impl) ReadGuest(&impl->refCount, what);
    return impl;
}

void Ref(StringImpl* string) {
    if (!string) return;
    const unsigned refCount = ReadGuest(&string->refCount, "Inspector::ConsoleMessage: unreadable string");
    WriteGuest(&string->refCount, refCount + RefCountIncrement, "Inspector::ConsoleMessage: unwritable string");
}

StringImpl* CreateLatin1(const std::string& text, const Jsc::WtfAbi::PersistenceAllocator& allocator) {
    constexpr auto charactersOffset = offsetof(StringImpl, hashAndFlags) + sizeof(StringImpl::hashAndFlags);
    void* storage = allocator.allocate(charactersOffset + text.size());
    if (!storage) throw std::bad_alloc();
    auto* characters = static_cast<unsigned char*>(storage) + charactersOffset;
    auto* string = new (storage) StringImpl{2u, static_cast<unsigned>(text.size()), characters, Jsc::WtfAbi::StringImplFlag8Bit};
    std::memcpy(characters, text.data(), text.size());
    return string;
}

StringImpl* RequestId(std::uint64_t identifier, const Jsc::WtfAbi::PersistenceAllocator& allocator) {
    if (!identifier) return nullptr;
    return CreateLatin1("0." + std::to_string(identifier), allocator);
}

constexpr std::size_t CopyChunk = 2048;

void AppendUnits(std::u16string& output, StringImpl* string) {
    if (!string) return;
    const StringImpl impl = ReadGuest(string, "Inspector::ConsoleMessage: unreadable log value");
    if (impl.length && !impl.data) throw std::invalid_argument("Inspector::ConsoleMessage: log value without characters");
    const bool is8Bit = impl.hashAndFlags & Jsc::WtfAbi::StringImplFlag8Bit;
    unsigned char latin1[CopyChunk];
    char16_t wide[CopyChunk];
    for (std::size_t offset = 0; offset < impl.length;) {
        const std::size_t count = std::min<std::size_t>(CopyChunk, impl.length - offset);
        const int savedErrno = errno;
        const bool copied = is8Bit
            ? GuestCopyIn_nid_no_patch(latin1, static_cast<const unsigned char*>(impl.data) + offset, count)
            : GuestCopyIn_nid_no_patch(wide, static_cast<const char16_t*>(impl.data) + offset, count * sizeof(char16_t));
        errno = savedErrno;
        if (!copied) throw std::invalid_argument("Inspector::ConsoleMessage: unreadable log value characters");
        if (is8Bit) output.append(latin1, latin1 + count);
        else output.append(wide, count);
        offset += count;
    }
}

constexpr char Backslash = 0x5C;
constexpr char Quote = 0x22;

char EscapeLetterFor(char16_t codeUnit) {
    switch (codeUnit) {
        case 0x08: return 'b';
        case 0x0C: return 'f';
        case 0x0A: return 'n';
        case 0x0D: return 'r';
        case 0x09: return 't';
        case Backslash: return Backslash;
        case Quote: return Quote;
    }
    return 0;
}

std::string QuotedJson(const std::u16string& units) {
    static constexpr char Digits[] = "0123456789ABCDEF";
    std::string output(1, Quote);
    for (const char16_t codeUnit : units) {
        if (const char letter = EscapeLetterFor(codeUnit)) {
            output += Backslash;
            output += letter;
        } else if (codeUnit >= 32 && codeUnit < 127 && codeUnit != '<' && codeUnit != '>') {
            output += static_cast<char>(codeUnit);
        } else {
            output += Backslash;
            output += 'u';
            output += Digits[(codeUnit >> 12) & 15];
            output += Digits[(codeUnit >> 8) & 15];
            output += Digits[(codeUnit >> 4) & 15];
            output += Digits[codeUnit & 15];
        }
    }
    output += Quote;
    return output;
}

void ConstructFromJsonLogValues(ConsoleMessage* self, std::uint8_t source, std::uint8_t type, std::uint8_t level,
    Jsc::InspectorAbi::JSONLogValueVector* messages, void* globalObject, std::uint64_t requestIdentifier) {
    using Jsc::InspectorAbi::JSONLogValue;
    using Jsc::InspectorAbi::JSONLogValueType;
    if (!self) throw std::invalid_argument("Inspector::ConsoleMessage: null this");
    if (!messages) throw std::invalid_argument("Inspector::ConsoleMessage: null log values");
    const auto input = ReadGuest(messages, "Inspector::ConsoleMessage: unreadable log values");
    if (input.size > input.capacity || (input.size && !input.buffer))
        throw std::invalid_argument("Inspector::ConsoleMessage: corrupt log value vector");
    if (globalObject) NotImplemented_nid_no_patch("Inspector::ConsoleMessage with a JSGlobalObject");
    if (input.size > std::numeric_limits<unsigned>::max() / sizeof(JSONLogValue))
        throw std::length_error("Inspector::ConsoleMessage: log value vector overflow");
    std::vector<JSONLogValue> values(input.size);
    if (input.size) {
        const int savedErrno = errno;
        const bool copied = GuestCopyIn_nid_no_patch(values.data(), input.buffer, input.size * sizeof(JSONLogValue));
        errno = savedErrno;
        if (!copied) throw std::invalid_argument("Inspector::ConsoleMessage: unreadable log values");
    }
    struct Output {
        bool created;
        std::string text;
        StringImpl* value;
    };
    std::vector<Output> outputs;
    std::u16string builder;
    for (const auto& value : values) {
        if (value.type == JSONLogValueType::String) {
            AppendUnits(builder, value.value);
            continue;
        }
        if (value.type != JSONLogValueType::JSON) throw std::invalid_argument("Inspector::ConsoleMessage: invalid log value type");
        if (!builder.empty()) {
            outputs.push_back({true, QuotedJson(builder), nullptr});
            builder.clear();
        }
        if (value.value) ReadGuest(&value.value->refCount, "Inspector::ConsoleMessage: unreadable log value");
        outputs.push_back({false, {}, value.value});
    }
    if (!builder.empty()) outputs.push_back({true, QuotedJson(builder), nullptr});

    const auto* allocator = Jsc::WtfAbi::RegisteredFastMalloc();
    const bool needsAllocator = requestIdentifier || !values.empty();
    if (needsAllocator && (!allocator || !allocator->allocate || !allocator->deallocate))
        throw std::runtime_error("Inspector::ConsoleMessage: the log values require libSceNKWebKit fastMalloc");
    std::vector<void*> allocated;
    try {
        ConsoleMessage message{};
        message.source = source;
        message.type = type;
        message.level = level;
        message.repeatCount = 1;
        message.requestId = RequestId(requestIdentifier, *allocator);
        if (message.requestId) allocated.push_back(message.requestId);
        if (!values.empty()) {
            auto* buffer = static_cast<JSONLogValue*>(allocator->allocate(values.size() * sizeof(JSONLogValue)));
            if (!buffer) throw std::bad_alloc();
            allocated.push_back(buffer);
            for (std::size_t i = 0; i < outputs.size(); ++i) {
                StringImpl* string = outputs[i].created ? CreateLatin1(outputs[i].text, *allocator) : outputs[i].value;
                if (outputs[i].created) allocated.push_back(string);
                buffer[i] = JSONLogValue{outputs[i].created ? JSONLogValueType::String : JSONLogValueType::JSON, string};
            }
            message.jsonLogValuesBuffer = buffer;
            message.jsonLogValuesCapacity = static_cast<unsigned>(values.size());
            message.jsonLogValuesSize = static_cast<unsigned>(outputs.size());
            if (!outputs.empty()) message.message = buffer[0].value;
        }
        WriteGuest(self, message, "Inspector::ConsoleMessage: unwritable this");
    } catch (...) {
        for (void* memory : allocated) allocator->deallocate(memory);
        throw;
    }
    for (const auto& output : outputs)
        if (!output.created) Ref(output.value);
    if (!outputs.empty()) {
        ConsoleMessage written = ReadGuest(self, "Inspector::ConsoleMessage: unreadable this");
        Ref(written.message);
    }
}

void Construct(ConsoleMessage* self, std::uint8_t source, std::uint8_t type, std::uint8_t level, StringImpl* const* message,
    StringImpl* const* url, unsigned line, unsigned column, void* globalObject, std::uint64_t requestIdentifier) {
    if (!self) throw std::invalid_argument("Inspector::ConsoleMessage: null this");
    StringImpl* messageImpl = ReadString(message, "Inspector::ConsoleMessage: unreadable message");
    StringImpl* urlImpl = url ? ReadString(url, "Inspector::ConsoleMessage: unreadable url") : nullptr;
    if (globalObject && type != Jsc::InspectorAbi::MessageTypeEndGroup)
        NotImplemented_nid_no_patch("Inspector::ConsoleMessage::autogenerateMetadata with a JSGlobalObject");
    const auto* allocator = Jsc::WtfAbi::RegisteredFastMalloc();
    if (requestIdentifier && (!allocator || !allocator->allocate || !allocator->deallocate))
        throw std::runtime_error("Inspector::ConsoleMessage: the request identifier requires libSceNKWebKit fastMalloc");
    StringImpl* requestId = requestIdentifier ? RequestId(requestIdentifier, *allocator) : nullptr;
    ConsoleMessage value{};
    value.source = source;
    value.type = type;
    value.level = level;
    value.message = messageImpl;
    value.url = urlImpl;
    value.line = line;
    value.column = column;
    value.repeatCount = 1;
    value.requestId = requestId;
    try {
        WriteGuest(self, value, "Inspector::ConsoleMessage: unwritable this");
    } catch (...) {
        if (requestId) allocator->deallocate(requestId);
        throw;
    }
    Ref(messageImpl);
    Ref(urlImpl);
}

}

extern "C" {

void APS5_VABI _ZN9Inspector14ConsoleMessageC1EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelERKN3WTF6StringEm(
    ConsoleMessage* self, std::uint8_t source, std::uint8_t type, std::uint8_t level, StringImpl* const* message, std::uint64_t requestIdentifier) {
    Construct(self, source, type, level, message, nullptr, 0, 0, nullptr, requestIdentifier);
}

void APS5_VABI _ZN9Inspector14ConsoleMessageC2EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelERKN3WTF6StringEm(
    ConsoleMessage* self, std::uint8_t source, std::uint8_t type, std::uint8_t level, StringImpl* const* message, std::uint64_t requestIdentifier) {
    Construct(self, source, type, level, message, nullptr, 0, 0, nullptr, requestIdentifier);
}

void APS5_VABI _ZN9Inspector14ConsoleMessageC1EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelERKN3WTF6StringES8_jjPNS1_14JSGlobalObjectEm(
    ConsoleMessage* self, std::uint8_t source, std::uint8_t type, std::uint8_t level, StringImpl* const* message, StringImpl* const* url,
    unsigned line, unsigned column, void* globalObject, std::uint64_t requestIdentifier) {
    if (!url) throw std::invalid_argument("Inspector::ConsoleMessage: null url");
    Construct(self, source, type, level, message, url, line, column, globalObject, requestIdentifier);
}

void APS5_VABI _ZN9Inspector14ConsoleMessageC2EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelERKN3WTF6StringES8_jjPNS1_14JSGlobalObjectEm(
    ConsoleMessage* self, std::uint8_t source, std::uint8_t type, std::uint8_t level, StringImpl* const* message, StringImpl* const* url,
    unsigned line, unsigned column, void* globalObject, std::uint64_t requestIdentifier) {
    if (!url) throw std::invalid_argument("Inspector::ConsoleMessage: null url");
    Construct(self, source, type, level, message, url, line, column, globalObject, requestIdentifier);
}

void APS5_VABI
_ZN9Inspector14ConsoleMessageC1EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelEON3WTF6VectorINS5_12JSONLogValueELm0ENS5_15CrashOnOverflowELm16ENS5_10FastMallocEEEPNS1_14JSGlobalObjectEm(
    ConsoleMessage* self, std::uint8_t source, std::uint8_t type, std::uint8_t level, Jsc::InspectorAbi::JSONLogValueVector* messages,
    void* globalObject, std::uint64_t requestIdentifier) {
    ConstructFromJsonLogValues(self, source, type, level, messages, globalObject, requestIdentifier);
}

void APS5_VABI
_ZN9Inspector14ConsoleMessageC2EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelEON3WTF6VectorINS5_12JSONLogValueELm0ENS5_15CrashOnOverflowELm16ENS5_10FastMallocEEEPNS1_14JSGlobalObjectEm(
    ConsoleMessage* self, std::uint8_t source, std::uint8_t type, std::uint8_t level, Jsc::InspectorAbi::JSONLogValueVector* messages,
    void* globalObject, std::uint64_t requestIdentifier) {
    ConstructFromJsonLogValues(self, source, type, level, messages, globalObject, requestIdentifier);
}

}
