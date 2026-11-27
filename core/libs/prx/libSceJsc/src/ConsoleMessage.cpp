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
 */

#include "prx/libSceJsc/include/ConsoleMessageExports.hpp"
#include "prx/libSceJsc/src/WtfFastMalloc.hpp"
#include "prx/libc/include/General.hpp"

#include <cerrno>
#include <cstring>
#include <new>
#include <stdexcept>
#include <string>

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

StringImpl* RequestId(std::uint64_t identifier, const Jsc::WtfAbi::PersistenceAllocator& allocator) {
    if (!identifier) return nullptr;
    const std::string text = "0." + std::to_string(identifier);
    constexpr auto charactersOffset = offsetof(StringImpl, hashAndFlags) + sizeof(StringImpl::hashAndFlags);
    void* storage = allocator.allocate(charactersOffset + text.size());
    if (!storage) throw std::bad_alloc();
    auto* characters = static_cast<unsigned char*>(storage) + charactersOffset;
    auto* string = new (storage) StringImpl{2u, static_cast<unsigned>(text.size()), characters, Jsc::WtfAbi::StringImplFlag8Bit};
    std::memcpy(characters, text.data(), text.size());
    return string;
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

}
