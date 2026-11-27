/*
 * Copyright (C) 2013-2019 Apple Inc. All rights reserved.
 * Copyright (C) 2014, 2016 Apple Inc. All rights reserved.
 * Copyright (C) 2011-2019 Apple Inc. All rights reserved.
 * Copyright (C) 2013, 2016 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * Derived from Source/JavaScriptCore/jit/RegisterSet.cpp, Source/JavaScriptCore/jit/Reg.cpp,
 * Source/JavaScriptCore/jit/GPRInfo.h, Source/JavaScriptCore/jit/FPRInfo.h,
 * Source/JavaScriptCore/assembler/X86_64Registers.h, Source/WTF/wtf/CommaPrinter.h and
 * Source/WTF/wtf/PrintStream.cpp@c55364d53f971f63a709976f168d932d558b8909
 */

#include "prx/libSceJsc/include/RegisterSetDumpExports.hpp"
#include "prx/libc/include/General.hpp"

#include <cerrno>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace {

using Jsc::JitAbi::RegisterSet;
using Jsc::WtfAbi::PrintStream;

using VprintfFunction = void (APS5_VABI*)(PrintStream*, const char*, VaList*);
using BeginFunction = PrintStream* (APS5_VABI*)(PrintStream*);
using EndFunction = void (APS5_VABI*)(PrintStream*);

constexpr unsigned NumberOfRegisters = 32;

constexpr const char* RegisterNames[NumberOfRegisters] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
    "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
};

PrintStream& Checked(PrintStream* out) {
    if (!out || !out->vtable) throw std::invalid_argument("JSC::RegisterSet::dump: null WTF::PrintStream or vtable");
    return *out;
}

template<typename TFunction>
TFunction Slot(PrintStream& out, Jsc::WtfAbi::PrintStreamSlot slot) {
    const void* entry = out.vtable[slot];
    if (!entry) throw std::invalid_argument("JSC::RegisterSet::dump: null WTF::PrintStream virtual function");
    return reinterpret_cast<TFunction>(const_cast<void*>(entry));
}

void APS5_VABI RegisterSetDumpPrintf_nid_no_patch(PrintStream& out, const char* format, ...) {
    const auto vprintf = Slot<VprintfFunction>(out, Jsc::WtfAbi::PrintStreamVprintf);
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

PrintStream& Begin(PrintStream& out) {
    return Checked(Slot<BeginFunction>(out, Jsc::WtfAbi::PrintStreamBegin)(&out));
}

void End(PrintStream& out) {
    Slot<EndFunction>(out, Jsc::WtfAbi::PrintStreamEnd)(&out);
}

void PrintText(PrintStream& out, const char* text) {
    RegisterSetDumpPrintf_nid_no_patch(out, "%s", text);
}

void Print(PrintStream& out, const char* text) {
    PrintStream& stream = Begin(out);
    PrintText(stream, text);
    End(out);
}

void PrintRegister(PrintStream& out, unsigned index) {
    PrintStream& stream = Begin(out);
    PrintText(stream, "%");
    PrintText(stream, RegisterNames[index]);
    End(out);
}

std::uint64_t ReadBits(const RegisterSet* self) {
    if (!self) throw std::invalid_argument("JSC::RegisterSet::dump: null register set");
    std::uint64_t bits = 0;
    const int savedErrno = errno;
    const bool copied = GuestCopyIn_nid_no_patch(&bits, self, sizeof(bits));
    errno = savedErrno;
    if (!copied) throw std::invalid_argument("JSC::RegisterSet::dump: unreadable register set");
    return bits;
}

}

extern "C" {

void APS5_VABI _ZNK3JSC11RegisterSet4dumpERN3WTF11PrintStreamE(const RegisterSet* self, PrintStream* out) {
    const std::uint64_t bits = ReadBits(self);
    PrintStream& outer = Checked(out);
    Print(outer, "[");
    bool didPrint = false;
    for (unsigned index = 0; index < NumberOfRegisters; ++index) {
        if (!((bits >> index) & 1)) continue;
        PrintStream& stream = Begin(outer);
        Print(stream, didPrint ? ", " : "");
        didPrint = true;
        PrintRegister(Begin(stream), index);
        End(stream);
        End(outer);
    }
    Print(outer, "]");
}

}
