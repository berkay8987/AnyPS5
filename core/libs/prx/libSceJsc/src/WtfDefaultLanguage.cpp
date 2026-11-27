/*
 * Copyright (C) 2019 Sony Interactive Entertainment Inc.
 * Copyright (C) 2010, 2013, 2016, 2017 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Derived from Source/WTF/wtf/playstation/LanguagePlayStation.cpp and
 * Source/WTF/wtf/Language.cpp@c55364d53f971f63a709976f168d932d558b8909
 */

#include "prx/libSceJsc/include/WtfDefaultLanguageExports.hpp"
#include "prx/libSceJsc/include/WtfSha1Exports.hpp"
#include "prx/libScePosixForWebKit/include/Api.hpp"
#include "prx/libc/include/General.hpp"

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <memory>
#include <new>
#include <stdexcept>

namespace {

using Jsc::WtfAbi::StringImpl;

constexpr std::size_t LanguageBufferSize = 16;

}

extern "C" {

StringImpl** APS5_VABI _ZN3WTF15defaultLanguageEv(StringImpl** result) {
    if (!result) throw std::invalid_argument("WTF::defaultLanguage: null return storage");
    const auto* allocator = RegisteredPersistenceAllocator_nid_no_patch();
    if (!allocator || !allocator->allocate || !allocator->deallocate)
        throw std::runtime_error("WTF::defaultLanguage: the string requires libSceNKWebKit fastMalloc");
    char buffer[LanguageBufferSize] = {};
    std::size_t length = 0;
    if (!GetenvNp_nid_no_patch("SYSTEM_LANGUAGE", buffer, sizeof(buffer))) length = std::strlen(buffer);
    constexpr auto charactersOffset = offsetof(StringImpl, hashAndFlags) + sizeof(StringImpl::hashAndFlags);
    std::unique_ptr<void, void (*)(void*)> storage(allocator->allocate(charactersOffset + length), allocator->deallocate);
    if (!storage) throw std::bad_alloc();
    auto* characters = static_cast<unsigned char*>(storage.get()) + charactersOffset;
    auto* string = new (storage.get()) StringImpl{2u, static_cast<unsigned>(length), characters, Jsc::WtfAbi::StringImplFlag8Bit};
    std::memcpy(characters, buffer, length);
    const int savedErrno = errno;
    const bool written = GuestCopyOut_nid_no_patch(result, &string, sizeof(string));
    errno = savedErrno;
    if (!written) throw std::invalid_argument("WTF::defaultLanguage: invalid return storage");
    storage.release();
    return result;
}

}
