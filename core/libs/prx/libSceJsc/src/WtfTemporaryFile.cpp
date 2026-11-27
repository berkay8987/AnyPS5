/*
 * Copyright (C) 2007-2017 Apple Inc. All rights reserved.
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
 * Derived from Source/WTF/wtf/posix/FileSystemPOSIX.cpp@c55364d53f971f63a709976f168d932d558b8909
 */

#include "prx/libSceJsc/include/WtfTemporaryFileExports.hpp"
#include "prx/libSceJsc/include/WtfSha1Exports.hpp"
#include "prx/libScePosixForWebKit/include/Api.hpp"
#include "prx/libc/include/Api.hpp"
#include "prx/libc/include/General.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Jsc::WtfAbi::StringImpl;

constexpr std::size_t PathMax = 1024;
constexpr int InvalidPlatformFileHandle = -1;
constexpr std::size_t CopyChunk = 4096;

template<typename T>
void ReadGuest(T* destination, const void* source, std::size_t bytes, const char* what) {
    const int savedErrno = errno;
    const bool copied = GuestCopyIn_nid_no_patch(destination, source, bytes);
    errno = savedErrno;
    if (!copied) throw std::invalid_argument(what);
}

template<typename T>
void WriteGuest(T* destination, const void* source, std::size_t bytes, const char* what) {
    const int savedErrno = errno;
    const bool copied = GuestCopyOut_nid_no_patch(destination, source, bytes);
    errno = savedErrno;
    if (!copied) throw std::invalid_argument(what);
}

void AppendUtf8(std::string& output, std::uint32_t codePoint) {
    if (codePoint < 0x80) {
        output += static_cast<char>(codePoint);
    } else if (codePoint < 0x800) {
        output += static_cast<char>(0xC0 | (codePoint >> 6));
        output += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else if (codePoint < 0x10000) {
        output += static_cast<char>(0xE0 | (codePoint >> 12));
        output += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        output += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else {
        output += static_cast<char>(0xF0 | (codePoint >> 18));
        output += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
        output += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        output += static_cast<char>(0x80 | (codePoint & 0x3F));
    }
}

std::string LenientUtf8(const std::vector<char16_t>& units) {
    std::string output;
    for (std::size_t index = 0; index < units.size(); ++index) {
        const std::uint32_t unit = units[index];
        if (unit >= 0xD800 && unit <= 0xDBFF) {
            if (index + 1 == units.size()) {
                AppendUtf8(output, unit);
                break;
            }
            const std::uint32_t next = units[index + 1];
            if (next >= 0xDC00 && next <= 0xDFFF) {
                AppendUtf8(output, 0x10000 + ((unit - 0xD800) << 10) + (next - 0xDC00));
                ++index;
                continue;
            }
            AppendUtf8(output, 0xFFFD);
            continue;
        }
        if (unit >= 0xDC00 && unit <= 0xDFFF) {
            AppendUtf8(output, 0xFFFD);
            continue;
        }
        AppendUtf8(output, unit);
    }
    return output;
}

std::string PrefixUtf8(StringImpl* const* prefix) {
    if (!prefix) throw std::invalid_argument("WTF::FileSystemImpl::openTemporaryFile: null prefix");
    StringImpl* pointer = nullptr;
    ReadGuest(&pointer, prefix, sizeof(pointer), "WTF::FileSystemImpl::openTemporaryFile: unreadable prefix");
    if (!pointer) return {};
    StringImpl impl{};
    ReadGuest(&impl, pointer, sizeof(impl), "WTF::FileSystemImpl::openTemporaryFile: unreadable prefix string");
    if (impl.length && !impl.data) throw std::invalid_argument("WTF::FileSystemImpl::openTemporaryFile: prefix without characters");
    const bool is8Bit = impl.hashAndFlags & Jsc::WtfAbi::StringImplFlag8Bit;
    std::vector<char16_t> units(impl.length);
    std::vector<unsigned char> latin1(is8Bit ? CopyChunk : 0);
    for (std::size_t offset = 0; offset < impl.length;) {
        const std::size_t count = std::min<std::size_t>(CopyChunk, impl.length - offset);
        if (is8Bit) {
            ReadGuest(latin1.data(), static_cast<const unsigned char*>(impl.data) + offset, count,
                "WTF::FileSystemImpl::openTemporaryFile: unreadable prefix characters");
            for (std::size_t index = 0; index < count; ++index) units[offset + index] = latin1[index];
        } else {
            ReadGuest(units.data() + offset, static_cast<const char16_t*>(impl.data) + offset, count * sizeof(char16_t),
                "WTF::FileSystemImpl::openTemporaryFile: unreadable prefix characters");
        }
        offset += count;
    }
    return LenientUtf8(units);
}

bool StrictUtf8ToUtf16(const std::string& input, std::u16string& output) {
    for (std::size_t index = 0; index < input.size();) {
        const auto lead = static_cast<unsigned char>(input[index]);
        std::uint32_t codePoint = 0;
        std::size_t extra = 0;
        std::uint32_t minimum = 0;
        if (lead < 0x80) {
            codePoint = lead;
        } else if (lead >= 0xC2 && lead <= 0xDF) {
            codePoint = lead & 0x1F;
            extra = 1;
            minimum = 0x80;
        } else if (lead >= 0xE0 && lead <= 0xEF) {
            codePoint = lead & 0x0F;
            extra = 2;
            minimum = 0x800;
        } else if (lead >= 0xF0 && lead <= 0xF4) {
            codePoint = lead & 0x07;
            extra = 3;
            minimum = 0x10000;
        } else {
            return false;
        }
        if (extra && index + extra >= input.size()) return false;
        for (std::size_t i = 1; i <= extra; ++i) {
            const auto trail = static_cast<unsigned char>(input[index + i]);
            if ((trail & 0xC0) != 0x80) return false;
            codePoint = (codePoint << 6) | (trail & 0x3F);
        }
        if (codePoint < minimum || codePoint > 0x10FFFF || (codePoint >= 0xD800 && codePoint <= 0xDFFF)) return false;
        if (codePoint >= 0x10000) {
            output += static_cast<char16_t>(0xD800 + ((codePoint - 0x10000) >> 10));
            output += static_cast<char16_t>(0xDC00 + ((codePoint - 0x10000) & 0x3FF));
        } else {
            output += static_cast<char16_t>(codePoint);
        }
        index += extra + 1;
    }
    return true;
}

StringImpl* CreateString(const std::string& path) {
    bool allAscii = true;
    for (const char c : path) allAscii = allAscii && !(static_cast<unsigned char>(c) & 0x80);
    std::u16string wide;
    if (!allAscii && !StrictUtf8ToUtf16(path, wide)) return nullptr;
    const std::size_t length = allAscii ? path.size() : wide.size();
    const std::size_t unitSize = allAscii ? 1 : sizeof(char16_t);
    const auto* allocator = RegisteredPersistenceAllocator_nid_no_patch();
    constexpr auto tailOffset = offsetof(StringImpl, hashAndFlags) + sizeof(StringImpl::hashAndFlags);
    const std::size_t charactersOffset = allAscii ? tailOffset : (tailOffset + 1) & ~std::size_t{1};
    void* storage = allocator->allocate(charactersOffset + length * unitSize);
    if (!storage) throw std::bad_alloc();
    auto* characters = static_cast<unsigned char*>(storage) + charactersOffset;
    auto* string = new (storage) StringImpl{2u, static_cast<unsigned>(length), characters, allAscii ? Jsc::WtfAbi::StringImplFlag8Bit : 0u};
    if (allAscii) std::memcpy(characters, path.data(), length);
    else std::memcpy(characters, wide.data(), length * unitSize);
    return string;
}

}

extern "C" {

StringImpl** APS5_VABI _ZN3WTF14FileSystemImpl17openTemporaryFileERKNS_6StringERiS3_(StringImpl** result, StringImpl* const* prefix,
    int* handle, StringImpl* const*) {
    if (!result) throw std::invalid_argument("WTF::FileSystemImpl::openTemporaryFile: null return storage");
    if (!handle) throw std::invalid_argument("WTF::FileSystemImpl::openTemporaryFile: null handle");
    const std::string prefixText = PrefixUtf8(prefix);
    const auto* allocator = RegisteredPersistenceAllocator_nid_no_patch();
    if (!allocator || !allocator->allocate || !allocator->deallocate)
        throw std::runtime_error("WTF::FileSystemImpl::openTemporaryFile: the string requires libSceNKWebKit fastMalloc");
    const char* tmpDir = getenv_nid_postfix("TMPDIR");
    if (!tmpDir) tmpDir = "/tmp";
    char buffer[PathMax];
    StringImpl* string = nullptr;
    int descriptor = InvalidPlatformFileHandle;
    const int written = std::snprintf(buffer, sizeof(buffer), "%s/%sXXXXXX", tmpDir, prefixText.c_str());
    if (written >= 0 && static_cast<std::size_t>(written) < sizeof(buffer)) {
        descriptor = mkstemp_nid_postfix(buffer);
        if (descriptor >= 0) string = CreateString(buffer);
        else descriptor = InvalidPlatformFileHandle;
    }
    WriteGuest(handle, &descriptor, sizeof(descriptor), "WTF::FileSystemImpl::openTemporaryFile: unwritable handle");
    try {
        WriteGuest(result, &string, sizeof(string), "WTF::FileSystemImpl::openTemporaryFile: invalid return storage");
    } catch (...) {
        if (string) allocator->deallocate(string);
        throw;
    }
    return result;
}

}
