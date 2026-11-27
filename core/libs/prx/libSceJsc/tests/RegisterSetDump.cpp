#include "prx/libSceJsc/libSceJsc.hpp"
#include "prx/libc/include/Api.hpp"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <type_traits>

using Jsc::JitAbi::RegisterSet;

static_assert(std::is_same_v<decltype(&_ZNK3JSC11RegisterSet4dumpERN3WTF11PrintStreamE),
                             void (APS5_VABI*)(const RegisterSet*, Jsc::WtfAbi::PrintStream*)>);

namespace {

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "RegisterSet dump check failed at line %d\n", line);
        std::abort();
    }
}

#define Require(value) Check((value), __LINE__)

struct TestStream {
    const void* const* vtable;
    std::string* log;
    char tag;
    TestStream* target;
};

void APS5_VABI Unexpected(TestStream*) {
    std::abort();
}

void APS5_VABI Vprintf(TestStream* self, const char* format, VaList* arguments) {
    char buffer[64];
    Require(vsnprintf_nid_postfix(buffer, sizeof(buffer), format, arguments) >= 0);
    if (!self->tag) {
        *self->log += buffer;
        return;
    }
    *self->log += '[';
    *self->log += self->tag;
    *self->log += format;
    *self->log += ':';
    *self->log += buffer;
    *self->log += ']';
}

TestStream* APS5_VABI Begin(TestStream* self) {
    if (!self->tag) return self->target;
    *self->log += '<';
    *self->log += self->tag;
    return self->target;
}

void APS5_VABI End(TestStream* self) {
    if (!self->tag) return;
    *self->log += self->tag;
    *self->log += '>';
}

const void* const testVtable[] = {
    reinterpret_cast<const void*>(&Unexpected),
    reinterpret_cast<const void*>(&Unexpected),
    reinterpret_cast<const void*>(&Vprintf),
    reinterpret_cast<const void*>(&Unexpected),
    reinterpret_cast<const void*>(&Begin),
    reinterpret_cast<const void*>(&End),
};

struct Streams {
    std::string log;
    TestStream inner{testVtable, &log, 'B', nullptr};
    TestStream outer{testVtable, &log, 'A', &inner};

    Streams() {
        inner.target = &inner;
    }

    Jsc::WtfAbi::PrintStream* Out() {
        return reinterpret_cast<Jsc::WtfAbi::PrintStream*>(&outer);
    }
};

struct PlainStreams {
    std::string log;
    TestStream stream{testVtable, &log, 0, nullptr};

    PlainStreams() {
        stream.target = &stream;
    }
};

std::string Text(std::uint64_t bits) {
    PlainStreams streams;
    const RegisterSet set{bits};
    _ZNK3JSC11RegisterSet4dumpERN3WTF11PrintStreamE(&set, reinterpret_cast<Jsc::WtfAbi::PrintStream*>(&streams.stream));
    Require(set.bits == bits);
    return streams.log;
}

template<typename TFunction>
bool Throws(TFunction function) {
    try {
        function();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

void TestCallSequence() {
    Streams streams;
    const RegisterSet set{(std::uint64_t{1} << 3) | (std::uint64_t{1} << 17)};
    _ZNK3JSC11RegisterSet4dumpERN3WTF11PrintStreamE(&set, streams.Out());
    Require(streams.log ==
        "<A[B%s:[]A>"
        "<A<B[B%s:]B><B<B[B%s:%][B%s:rbx]B>B>A>"
        "<A<B[B%s:, ]B><B<B[B%s:%][B%s:xmm1]B>B>A>"
        "<A[B%s:]]A>");
}

void TestEmpty() {
    Streams streams;
    const RegisterSet set{0};
    _ZNK3JSC11RegisterSet4dumpERN3WTF11PrintStreamE(&set, streams.Out());
    Require(streams.log == "<A[B%s:[]A><A[B%s:]]A>");
}

void TestNames() {
    Require(Text(0) == "[]");
    Require(Text(0xffff) == "[%rax, %rcx, %rdx, %rbx, %rsp, %rbp, %rsi, %rdi, %r8, %r9, %r10, %r11, %r12, %r13, %r14, %r15]");
    Require(Text(0xffff0000) ==
        "[%xmm0, %xmm1, %xmm2, %xmm3, %xmm4, %xmm5, %xmm6, %xmm7, %xmm8, %xmm9, %xmm10, %xmm11, %xmm12, %xmm13, %xmm14, %xmm15]");
    Require(Text(0xf028) == "[%rbx, %rbp, %r12, %r13, %r14, %r15]");
    Require(Text(0x3c6) == "[%rcx, %rdx, %rsi, %rdi, %r8, %r9]");
    Require(Text(0x30) == "[%rsp, %rbp]");
    Require(Text(0x800) == "[%r11]");
    Require(Text(std::uint64_t{1} << 31) == "[%xmm15]");
}

void TestBitsPastLastRegister() {
    Require(Text(std::uint64_t{1} << 32) == "[]");
    Require(Text(~std::uint64_t{0} << 32) == "[]");
    Require(Text((std::uint64_t{1} << 32) | 1) == "[%rax]");
}

void TestInvalidArguments() {
    Streams streams;
    const RegisterSet set{1};
    errno = 41;
    Require(Throws([&] { _ZNK3JSC11RegisterSet4dumpERN3WTF11PrintStreamE(nullptr, streams.Out()); }));
    Require(errno == 41);
    const auto* unreadable = reinterpret_cast<const RegisterSet*>(std::uintptr_t{0x10});
    Require(Throws([&] { _ZNK3JSC11RegisterSet4dumpERN3WTF11PrintStreamE(unreadable, streams.Out()); }));
    Require(errno == 41);
    Require(Throws([&] { _ZNK3JSC11RegisterSet4dumpERN3WTF11PrintStreamE(&set, nullptr); }));
    TestStream noVtable{nullptr, &streams.log, 'N', nullptr};
    Require(Throws([&] {
        _ZNK3JSC11RegisterSet4dumpERN3WTF11PrintStreamE(&set, reinterpret_cast<Jsc::WtfAbi::PrintStream*>(&noVtable));
    }));
    Require(streams.log.empty());
    TestStream nullTarget{testVtable, &streams.log, 'C', nullptr};
    Require(Throws([&] {
        _ZNK3JSC11RegisterSet4dumpERN3WTF11PrintStreamE(&set, reinterpret_cast<Jsc::WtfAbi::PrintStream*>(&nullTarget));
    }));
}

}

int main() {
    TestCallSequence();
    TestEmpty();
    TestNames();
    TestBitsPastLastRegister();
    TestInvalidArguments();
    std::puts("RegisterSet dump tests passed");
    return 0;
}
