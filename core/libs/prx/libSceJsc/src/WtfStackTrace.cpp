/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
 * Copyright (C) 2017 Yusuke Suzuki <utatane.tea@gmail.com>
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
 * Derived from Source/WTF/wtf/StackTrace.cpp@c55364d53f971f63a709976f168d932d558b8909
 */

#include "prx/libSceJsc/include/WtfStackTraceExports.hpp"
#include "prx/libSceJsc/src/WtfFastMalloc.hpp"
#include "prx/libc/include/General.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <new>
#include <stdexcept>

namespace {

using Jsc::WtfAbi::OptionalDemangleEntry;
using Jsc::WtfAbi::PrintStream;
using Jsc::WtfAbi::StackTrace;

using VprintfFunction = void (APS5_VABI*)(PrintStream*, const char*, VaList*);

template<typename T>
void ReadGuest(T* destination, const void* source, std::size_t bytes, const char* what) {
    const int savedErrno = errno;
    const bool copied = GuestCopyIn_nid_no_patch(destination, source, bytes);
    errno = savedErrno;
    if (!copied) throw std::invalid_argument(what);
}

template<typename T>
void WriteGuest(void* destination, const T* source, std::size_t bytes, const char* what) {
    const int savedErrno = errno;
    const bool copied = GuestCopyOut_nid_no_patch(destination, source, bytes);
    errno = savedErrno;
    if (!copied) throw std::invalid_argument(what);
}

void APS5_VABI StackTracePrintf_nid_no_patch(PrintStream& out, const char* format, ...) {
    const void* entry = out.vtable[Jsc::WtfAbi::PrintStreamVprintf];
    if (!entry) throw std::invalid_argument("WTF::StackTrace::dump: null WTF::PrintStream::vprintf");
    const auto vprintf = reinterpret_cast<VprintfFunction>(const_cast<void*>(entry));
#ifdef _WIN32
    __builtin_sysv_va_list arguments;
    __builtin_sysv_va_start(arguments, format);
    vprintf(&out, format, reinterpret_cast<VaList*>(arguments));
    __builtin_sysv_va_end(arguments);
#else
    std::va_list arguments;
    va_start(arguments, format);
    vprintf(&out, format, reinterpret_cast<VaList*>(arguments));
    va_end(arguments);
#endif
}

}

extern "C" {

StackTrace** APS5_VABI _ZN3WTF10StackTrace17captureStackTraceEii(StackTrace** result, int maxFrames, int) {
    if (!result) throw std::invalid_argument("WTF::StackTrace::captureStackTrace: null return storage");
    maxFrames = std::max(1, maxFrames);
    const std::size_t sizeToAllocate = sizeof(StackTrace) + static_cast<std::size_t>(maxFrames - 1) * sizeof(void*);
    const auto* allocator = Jsc::WtfAbi::RegisteredFastMalloc();
    if (!allocator || !allocator->allocate || !allocator->deallocate)
        throw std::runtime_error("WTF::StackTrace::captureStackTrace: requires libSceNKWebKit fastMalloc");
    void* memory = allocator->allocate(sizeToAllocate);
    if (!memory) throw std::bad_alloc();
    auto* trace = static_cast<StackTrace*>(memory);
    trace->size = 0;
    trace->capacity = maxFrames;
    try {
        WriteGuest(result, &trace, sizeof(trace), "WTF::StackTrace::captureStackTrace: invalid return storage");
    } catch (...) {
        allocator->deallocate(memory);
        throw;
    }
    return result;
}

OptionalDemangleEntry* APS5_VABI _ZN3WTF10StackTrace8demangleEPv(OptionalDemangleEntry* result, void*) {
    if (!result) throw std::invalid_argument("WTF::StackTrace::demangle: null return storage");
    const bool hasValue = false;
    WriteGuest(result, &hasValue, sizeof(hasValue), "WTF::StackTrace::demangle: invalid return storage");
    return result;
}

void APS5_VABI _ZNK3WTF10StackTrace4dumpERNS_11PrintStreamEPKc(const StackTrace* self, PrintStream* out, const char* indentString) {
    if (!self) throw std::invalid_argument("WTF::StackTrace::dump: null stack trace");
    StackTrace header{};
    ReadGuest(&header, self, sizeof(header), "WTF::StackTrace::dump: unreadable stack trace");
    if (header.size <= 0) return;
    if (!out || !out->vtable) throw std::invalid_argument("WTF::StackTrace::dump: null WTF::PrintStream or vtable");
    const auto* stack = header.capacity ? reinterpret_cast<void* const*>(&self->stackOrBorrowed)
                                        : static_cast<void* const*>(header.stackOrBorrowed);
    if (!indentString) indentString = "";
    for (int i = 0; i < header.size; ++i) {
        void* frame = nullptr;
        ReadGuest(&frame, stack + i, sizeof(frame), "WTF::StackTrace::dump: unreadable frame");
        StackTracePrintf_nid_no_patch(*out, "%s%-3d %p\n", indentString, i + 1, frame);
    }
}

}
