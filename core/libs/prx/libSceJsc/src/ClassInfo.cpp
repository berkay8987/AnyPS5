/*
 * Copyright (C) 2008-2019 Apple Inc. All rights reserved.
 * Copyright (C) 2020 Alexey Shvayka <shvaikalesh@gmail.com>.
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
 * Derived from Source/JavaScriptCore/runtime/Structure.cpp@c55364d53f971f63a709976f168d932d558b8909
 */

#include "prx/libSceJsc/include/ClassInfoExports.hpp"
#include "prx/libc/include/General.hpp"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <unordered_set>

namespace {

using Jsc::RuntimeAbi::ClassInfo;
using Jsc::RuntimeAbi::HashTable;

template<typename T>
T ReadGuest(const T* source, const char* what) {
    T value{};
    const int savedErrno = errno;
    const bool copied = GuestCopyIn_nid_no_patch(&value, source, sizeof(value));
    errno = savedErrno;
    if (!copied) throw std::invalid_argument(what);
    return value;
}

}

extern "C" {

bool APS5_VABI _ZNK3JSC9ClassInfo35hasStaticSetterOrReadonlyPropertiesEv(const ClassInfo* self) {
    if (!self) throw std::invalid_argument("JSC::ClassInfo::hasStaticSetterOrReadonlyProperties: null class info");
    std::unordered_set<const ClassInfo*> visited;
    for (const ClassInfo* info = self; info; ) {
        if (!visited.insert(info).second)
            throw std::invalid_argument("JSC::ClassInfo::hasStaticSetterOrReadonlyProperties: cyclic parent class chain");
        const ClassInfo current = ReadGuest(info, "JSC::ClassInfo::hasStaticSetterOrReadonlyProperties: unreadable class info");
        if (current.staticPropHashTable) {
            const auto* flag = reinterpret_cast<const std::uint8_t*>(&current.staticPropHashTable->hasSetterOrReadonlyProperties);
            if (ReadGuest(flag, "JSC::ClassInfo::hasStaticSetterOrReadonlyProperties: unreadable static property table"))
                return true;
        }
        info = current.parentClass;
    }
    return false;
}

}
