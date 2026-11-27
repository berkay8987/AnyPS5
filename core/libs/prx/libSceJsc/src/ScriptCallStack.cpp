/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
 * Copyright (c) 2008, Google Inc. All rights reserved.
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
 * Derived from Source/JavaScriptCore/inspector/ScriptCallStack.cpp and
 * Source/WTF/wtf/Vector.h@c55364d53f971f63a709976f168d932d558b8909
 */

#include "prx/libSceJsc/include/ScriptCallStackExports.hpp"
#include "prx/libSceJsc/src/WtfFastMalloc.hpp"
#include "prx/libc/include/Api.hpp"
#include "prx/libc/include/General.hpp"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>

namespace {

using Jsc::InspectorAbi::ScriptCallFrame;
using Jsc::InspectorAbi::ScriptCallFrameVector;
using Jsc::InspectorAbi::ScriptCallStack;
using Jsc::WtfAbi::StringImpl;

constexpr unsigned RefCountIncrement = 2;
constexpr std::size_t MinCapacity = 16;

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

void Ref(StringImpl* string) {
    if (!string) return;
    const unsigned refCount = ReadGuest(&string->refCount, "Inspector::ScriptCallStack::append: unreadable string");
    WriteGuest(&string->refCount, refCount + RefCountIncrement, "Inspector::ScriptCallStack::append: unwritable string");
}

void Construct(ScriptCallStack* self, ScriptCallFrameVector* frames, const char* function) {
    if (!self) throw std::invalid_argument(std::string(function) + ": null this");
    if (!frames) throw std::invalid_argument(std::string(function) + ": null frames");
    const ScriptCallFrameVector taken = ReadGuest(frames, "Inspector::ScriptCallStack: unreadable frames");
    WriteGuest(frames, ScriptCallFrameVector{}, "Inspector::ScriptCallStack: unwritable frames");
    WriteGuest(self, ScriptCallStack{1u, taken}, "Inspector::ScriptCallStack: unwritable this");
}

}

extern "C" {

void APS5_VABI _ZN9Inspector15ScriptCallStackC1ERN3WTF6VectorINS_15ScriptCallFrameELm0ENS1_15CrashOnOverflowELm16ENS1_10FastMallocEEE(
    ScriptCallStack* self, ScriptCallFrameVector* frames) {
    Construct(self, frames, "Inspector::ScriptCallStack::ScriptCallStack");
}

void APS5_VABI _ZN9Inspector15ScriptCallStackC2ERN3WTF6VectorINS_15ScriptCallFrameELm0ENS1_15CrashOnOverflowELm16ENS1_10FastMallocEEE(
    ScriptCallStack* self, ScriptCallFrameVector* frames) {
    Construct(self, frames, "Inspector::ScriptCallStack::ScriptCallStack");
}

ScriptCallStack** APS5_VABI
_ZN9Inspector15ScriptCallStack6createERN3WTF6VectorINS_15ScriptCallFrameELm0ENS1_15CrashOnOverflowELm16ENS1_10FastMallocEEE(
    ScriptCallStack** result, ScriptCallFrameVector* frames) {
    if (!result) throw std::invalid_argument("Inspector::ScriptCallStack::create: null return storage");
    if (!frames) throw std::invalid_argument("Inspector::ScriptCallStack::create: null frames");
    const ScriptCallFrameVector taken = ReadGuest(frames, "Inspector::ScriptCallStack::create: unreadable frames");
    auto* stack = new (_Znwm_nid_postfix(sizeof(ScriptCallStack))) ScriptCallStack{1u, taken};
    try {
        WriteGuest(result, stack, "Inspector::ScriptCallStack::create: invalid return storage");
    } catch (...) {
        _ZdlPv_nid_postfix(stack);
        throw;
    }
    WriteGuest(frames, ScriptCallFrameVector{}, "Inspector::ScriptCallStack::create: unwritable frames");
    return result;
}

void APS5_VABI _ZN9Inspector15ScriptCallStack6appendERKNS_15ScriptCallFrameE(ScriptCallStack* self, const ScriptCallFrame* frame) {
    if (!self) throw std::invalid_argument("Inspector::ScriptCallStack::append: null this");
    if (!frame) throw std::invalid_argument("Inspector::ScriptCallStack::append: null frame");
    ScriptCallFrameVector frames = ReadGuest(&self->frames, "Inspector::ScriptCallStack::append: unreadable this");
    if (frames.size > frames.capacity || (frames.capacity && !frames.buffer))
        throw std::invalid_argument("Inspector::ScriptCallStack::append: corrupt frame vector");
    const ScriptCallFrame value = ReadGuest(frame, "Inspector::ScriptCallStack::append: unreadable frame");
    if (frames.size == frames.capacity) {
        const std::size_t capacity = frames.capacity;
        const std::size_t newCapacity = std::max<std::size_t>(capacity + 1, std::max(MinCapacity, capacity + capacity / 4 + 1));
        if (newCapacity > std::numeric_limits<unsigned>::max() / sizeof(ScriptCallFrame))
            throw std::length_error("Inspector::ScriptCallStack::append: frame vector overflow");
        const auto* allocator = Jsc::WtfAbi::RegisteredFastMalloc();
        if (!allocator || !allocator->allocate || !allocator->deallocate)
            throw std::runtime_error("Inspector::ScriptCallStack::append: Vector storage requires libSceNKWebKit fastMalloc");
        auto* buffer = static_cast<ScriptCallFrame*>(allocator->allocate(newCapacity * sizeof(ScriptCallFrame)));
        if (!buffer) throw std::bad_alloc();
        if (frames.size) {
            const int savedErrno = errno;
            const bool copied = GuestCopyIn_nid_no_patch(buffer, frames.buffer, frames.size * sizeof(ScriptCallFrame));
            errno = savedErrno;
            if (!copied) {
                allocator->deallocate(buffer);
                throw std::invalid_argument("Inspector::ScriptCallStack::append: unreadable frames");
            }
        }
        if (frames.buffer) allocator->deallocate(frames.buffer);
        frames.buffer = buffer;
        frames.capacity = static_cast<unsigned>(newCapacity);
    }
    Ref(value.functionName);
    Ref(value.scriptName);
    WriteGuest(frames.buffer + frames.size, value, "Inspector::ScriptCallStack::append: unwritable frame storage");
    ++frames.size;
    WriteGuest(&self->frames, frames, "Inspector::ScriptCallStack::append: unwritable this");
}

}
