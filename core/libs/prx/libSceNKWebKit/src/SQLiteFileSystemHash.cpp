// Derived from https://github.com/WebKit/WebKit/blob/73a35b43be7ae44c084885bd3fa4b403e59723c8/Source/WebCore/platform/sql/SQLiteFileSystem.cpp
/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
 */

#include "prx/libSceNKWebKit/include/LegacySQLiteFileSystemExports.hpp"
#include "prx/libSceNKWebKit/include/PalCryptoDigestExports.hpp"
#include "prx/libSceNKWebKit/include/WtfExports.hpp"
#include "prx/libSceNKWebKit/src/WtfStringAbi.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <string>

using NKWebKit::WtfAbi::CString;
using NKWebKit::WtfAbi::String;

extern "C" {

String APS5_VABI _ZN7WebCore16SQLiteFileSystem22computeHashForFileNameERKN3WTF6StringE(const String& fileName) {
    const CString representation = _ZNK3WTF6String4utf8ENS_14ConversionModeE(&fileName, 0);
    const char* data = representation.buffer ? reinterpret_cast<const char*>(representation.buffer + 1) : nullptr;
    const std::size_t length = representation.buffer ? representation.buffer->length : 0;
    auto cryptoDigest = _ZN3PAL12CryptoDigest6createENS0_9AlgorithmE(PAL::CryptoDigest::Algorithm::SHA_256);
    if (!cryptoDigest) throw std::runtime_error("WebCore::SQLiteFileSystem::computeHashForFileName: no SHA-256 digest");
    _ZN3PAL12CryptoDigest8addBytesEPKvm(cryptoDigest.get(), data, length);
    const auto digest = _ZN3PAL12CryptoDigest11computeHashEv(cryptoDigest.get());
    std::string hex(static_cast<std::size_t>(digest.size) * 2, '\0');
    for (unsigned index = 0; index < digest.size; ++index) {
        char pair[3];
        std::snprintf(pair, sizeof(pair), "%02X", digest.buffer[index]);
        hex[index * 2] = pair[0];
        hex[index * 2 + 1] = pair[1];
    }
    return _ZN3WTF6String8fromUTF8EPKhm(reinterpret_cast<const unsigned char*>(hex.data()), hex.size());
}

}
