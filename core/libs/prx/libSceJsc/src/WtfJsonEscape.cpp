/*
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

#include "prx/libSceJsc/include/WtfJsonEscapeExports.hpp"
#include "prx/libc/include/General.hpp"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace {

using Jsc::WtfAbi::StringBuilder;
using Jsc::WtfAbi::StringView;

constexpr unsigned ChunkLength = 1024;

using AppendFunction = StringBuilderAppendFunction;

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

void AppendCodeUnit(AppendFunction append, StringBuilder* builder, char16_t codeUnit) {
    if (const char letter = EscapeLetterFor(codeUnit)) {
        const char sequence[2] = {Backslash, letter};
        return append(builder, sequence, sizeof(sequence));
    }
    if (codeUnit >= 32 && codeUnit < 127 && codeUnit != '<' && codeUnit != '>') {
        const char character = static_cast<char>(codeUnit);
        return append(builder, &character, 1);
    }
    static constexpr char Digits[] = "0123456789ABCDEF";
    const char sequence[6] = {Backslash, 'u', Digits[(codeUnit >> 12) & 15], Digits[(codeUnit >> 8) & 15], Digits[(codeUnit >> 4) & 15],
        Digits[codeUnit & 15]};
    append(builder, sequence, sizeof(sequence));
}

template<typename CharacterType>
void AppendEscaped(AppendFunction append, StringBuilder* builder, const CharacterType* characters, unsigned length) {
    CharacterType chunk[ChunkLength];
    for (unsigned offset = 0; offset < length;) {
        const unsigned count = std::min(ChunkLength, length - offset);
        const int savedErrno = errno;
        const bool copied = GuestCopyIn_nid_no_patch(chunk, characters + offset, std::size_t{count} * sizeof(CharacterType));
        errno = savedErrno;
        if (!copied) throw std::invalid_argument("WTF::JSONImpl::Value::escapeString: unreadable characters");
        for (unsigned index = 0; index < count; ++index) AppendCodeUnit(append, builder, chunk[index]);
        offset += count;
    }
}

}

extern "C" {

void APS5_VABI _ZN3WTF8JSONImpl5Value12escapeStringERNS_13StringBuilderENS_10StringViewE(StringBuilder* builder, StringView string) {
    if (!builder) throw std::invalid_argument("WTF::JSONImpl::Value::escapeString: null builder");
    if (string.length && !string.characters) throw std::invalid_argument("WTF::JSONImpl::Value::escapeString: null characters with a nonzero length");
    if (!string.length) return;
    const AppendFunction append = StringBuilderAppendBridge_nid_no_patch(__func__);
    if (string.is8Bit) AppendEscaped(append, builder, static_cast<const std::uint8_t*>(string.characters), string.length);
    else AppendEscaped(append, builder, static_cast<const char16_t*>(string.characters), string.length);
}

}
