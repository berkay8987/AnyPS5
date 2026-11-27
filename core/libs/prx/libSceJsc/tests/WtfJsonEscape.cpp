#include "prx/libSceJsc/libSceJsc.hpp"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

using Jsc::WtfAbi::StringBuilder;
using Jsc::WtfAbi::StringView;

static_assert(std::is_same_v<decltype(&_ZN3WTF8JSONImpl5Value12escapeStringERNS_13StringBuilderENS_10StringViewE),
                             void (APS5_VABI*)(StringBuilder*, StringView)>);

namespace {

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "JSON escapeString check failed at line %d\n", line);
        std::abort();
    }
}

#define Require(value) Check((value), __LINE__)

StringBuilder* lastBuilder = nullptr;
std::vector<std::string> appends;

void RecordAppend(StringBuilder* builder, const char* characters, unsigned length) {
    lastBuilder = builder;
    appends.emplace_back(characters, length);
}

template<typename TException, typename TFunction>
bool Throws(TFunction function) {
    try {
        function();
    } catch (const TException&) {
        return true;
    }
    return false;
}

std::string Joined() {
    std::string text;
    for (const auto& piece : appends) text += piece;
    return text;
}

std::string Escape8(const std::string& input) {
    appends.clear();
    StringBuilder builder{};
    _ZN3WTF8JSONImpl5Value12escapeStringERNS_13StringBuilderENS_10StringViewE(&builder,
        StringView{input.data(), static_cast<unsigned>(input.size()), true});
    Require(appends.empty() || lastBuilder == &builder);
    return Joined();
}

std::string Escape16(const std::u16string& input) {
    appends.clear();
    StringBuilder builder{};
    _ZN3WTF8JSONImpl5Value12escapeStringERNS_13StringBuilderENS_10StringViewE(&builder,
        StringView{input.data(), static_cast<unsigned>(input.size()), false});
    Require(appends.empty() || lastBuilder == &builder);
    return Joined();
}

void TestUnbridged() {
    StringBuilder builder{};
    const char text[] = "a";
    Require(Throws<std::runtime_error>([&] {
        _ZN3WTF8JSONImpl5Value12escapeStringERNS_13StringBuilderENS_10StringViewE(&builder, StringView{text, 1, true});
    }));
    _ZN3WTF8JSONImpl5Value12escapeStringERNS_13StringBuilderENS_10StringViewE(&builder, StringView{nullptr, 0, true});
}

void TestNamedEscapes() {
    Require(Escape8("\b\f\n\r\t\\\"") == "\\b\\f\\n\\r\\t\\\\\\\"");
    Require(appends.size() == 7);
    Require(appends[0] == "\\b" && appends[6] == "\\\"");
}

void TestPrintableAscii() {
    std::string printable;
    for (char c = 32; c < 127; ++c)
        if (c != '"' && c != '\\' && c != '<' && c != '>') printable += c;
    Require(Escape8(printable) == printable);
    Require(appends.size() == printable.size());
    Require(Escape8("") == "" && appends.empty());
}

void TestUnicodeEscapes() {
    Require(Escape8("<a>") == "\\u003Ca\\u003E");
    Require(appends.size() == 3 && appends[0] == "\\u003C");
    Require(Escape8(std::string("\0\x01\x1f\x7f", 4)) == "\\u0000\\u0001\\u001F\\u007F");
    Require(Escape8("\xe9\xff") == "\\u00E9\\u00FF");
    Require(Escape8("\x0b") == "\\u000B");
}

void TestSixteenBit() {
    Require(Escape16(u"a\"\n") == "a\\\"\\n");
    Require(Escape16(std::u16string{char16_t{0x3B1}, char16_t{0xFFFF}}) =="\\u03B1\\uFFFF");
    Require(Escape16(u"\U0001F600") == "\\uD83D\\uDE00");
    const std::u16string lone{static_cast<char16_t>(0xDC00), u'x'};
    Require(Escape16(lone) == "\\uDC00x");
    Require(Escape16(u"<>") == "\\u003C\\u003E");
}

void TestLongInput() {
    std::string input(5000, 'x');
    input[1023] = '\n';
    input[1024] = '<';
    input[4999] = '"';
    std::string expected(5000, 'x');
    expected.replace(4999, 1, "\\\"");
    expected.replace(1024, 1, "\\u003C");
    expected.replace(1023, 1, "\\n");
    Require(Escape8(input) == expected);
    std::u16string wide(3000, u'y');
    wide[2048] = 0x100;
    std::string wideExpected(3000, 'y');
    wideExpected.replace(2048, 1, "\\u0100");
    Require(Escape16(wide) == wideExpected);
}

void TestInvalidArguments() {
    appends.clear();
    StringBuilder builder{};
    const char text[] = "a";
    errno = 29;
    Require(Throws<std::invalid_argument>([&] {
        _ZN3WTF8JSONImpl5Value12escapeStringERNS_13StringBuilderENS_10StringViewE(nullptr, StringView{text, 1, true});
    }));
    Require(Throws<std::invalid_argument>([&] {
        _ZN3WTF8JSONImpl5Value12escapeStringERNS_13StringBuilderENS_10StringViewE(&builder, StringView{nullptr, 1, true});
    }));
    const auto* unreadable = reinterpret_cast<const void*>(std::uintptr_t{0x10});
    Require(Throws<std::invalid_argument>([&] {
        _ZN3WTF8JSONImpl5Value12escapeStringERNS_13StringBuilderENS_10StringViewE(&builder, StringView{unreadable, 4, true});
    }));
    Require(Throws<std::invalid_argument>([&] {
        _ZN3WTF8JSONImpl5Value12escapeStringERNS_13StringBuilderENS_10StringViewE(&builder, StringView{unreadable, 4, false});
    }));
    Require(errno == 29);
    Require(appends.empty());
}

}

int main() {
    TestUnbridged();
    Jsc::ApiBridge bridge{};
    bridge.appendStringBuilderCharacters = &RecordAppend;
    RegisterApiBridge_nid_no_patch(&bridge);
    TestNamedEscapes();
    TestPrintableAscii();
    TestUnicodeEscapes();
    TestSixteenBit();
    TestLongInput();
    TestInvalidArguments();
    RegisterApiBridge_nid_no_patch(nullptr);
    TestUnbridged();
    std::puts("JSON escapeString tests passed");
    return 0;
}
