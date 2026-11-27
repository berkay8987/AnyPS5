#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceNKWebKit/libSceNKWebKit.hpp"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace {

using NKWebKit::WtfAbi::String;
using NKWebKit::WtfAbi::URL;

String MakeString(std::string_view text, bool wide) {
    if (wide) {
        std::u16string units(text.begin(), text.end());
        return _ZN3WTF10StringImpl6createEPKDsj(units.data(), static_cast<unsigned>(units.size()));
    }
    return _ZN3WTF10StringImpl6createEPKhj(reinterpret_cast<const unsigned char*>(text.data()), static_cast<unsigned>(text.size()));
}

void Check(const URL& url, const String& referrer, bool expected, std::string_view label) {
    bool actual = _ZN7WebCore14SecurityPolicy18shouldHideReferrerERKN3WTF3URLERKNS1_6StringE(&url, &referrer);
    if (actual != expected) {
        std::fprintf(stderr, "shouldHideReferrer mismatch: %.*s, expected %d got %d\n", static_cast<int>(label.size()), label.data(), expected, actual);
        std::abort();
    }
}

void Test() {
    struct ReferrerCase { std::string_view text; int scheme; };
    const ReferrerCase cases[] = {
        {"http://source/", 1}, {"https://source/", 2}, {"ftp://source/", 0},
        {"file:///source", 0}, {"data:text/plain,source", 0}, {"relative/path", 0},
        {"HtTp://source/", 1}, {"hTtPs://source/", 2},
        {" \t\r\nhttps://source/", 2}, {"\x01\x1fhttps://source/", 2},
        {std::string_view("\0https://source/", 16), 2},
        {"h\tt\rt\nps\t://source/", 2}, {"h\tt\rt\np\n://source/", 1},
        {"htt ps://source/", 0}, {"https ://source/", 0}, {"httpsx://source/", 0},
        {"https", 0}, {"http", 0}, {"https:", 2}, {"http:", 1},
        {"https\t\r\n:", 2}, {"https\v:", 0}, {"https\x01:", 0},
        {" \t\r\n", 0}, {"", 0}, {"\xa0https://source/", 0}
    };
    struct DestinationCase { std::string_view text; unsigned schemeLength; bool secure; };
    const DestinationCase destinations[] = {
        {"http://target/", 4, false}, {"https://target/", 5, true},
        {"ftp://target/", 3, false}, {"file:///target", 4, false},
        {"HtTpS://target/", 5, true}, {"httpsx://target/", 6, false},
        {" https://target/", 6, false}, {"ht\ttps://target/", 6, false}
    };
    for (bool destinationWide : {false, true}) {
        for (const auto& destination : destinations) {
            auto storage = MakeString(destination.text, destinationWide);
            URL url{storage.impl, 1U | (destination.schemeLength << 6), 0, 0, 0, 0, 0, 0, 0};
            for (bool referrerWide : {false, true}) {
                for (const auto& referrer : cases) {
                    auto text = MakeString(referrer.text, referrerWide);
                    Check(url, text, referrer.scheme == 0 || (referrer.scheme == 2 && !destination.secure), referrer.text);
                }
            }
            Check(url, String(), true, "null referrer string");
            url.bits &= ~1U;
            Check(url, MakeString("https://source/", false), true, "invalid destination, HTTPS referrer");
            Check(url, MakeString("http://source/", true), false, "invalid destination, HTTP referrer");
        }
    }
    URL empty{};
    Check(empty, MakeString("https://source/", true), true, "empty invalid destination");
    Check(empty, String(), true, "both null strings");
    auto unicode = _ZN3WTF10StringImpl6createEPKDsj(u"\u0100https://source/", 16);
    auto secureStorage = MakeString("https://target/", true);
    URL secure{secureStorage.impl, 1U | (5U << 6), 0, 0, 0, 0, 0, 0, 0};
    Check(secure, unicode, true, "non-ASCII UTF-16 prefix");
}

struct ParsedURL {
    URL url{};
    String owner;
    explicit ParsedURL(std::string_view text) {
        const URL base{};
        const String string = MakeString(text, false);
        _ZN3WTF3URLC2ERKS0_RKNS_6StringEPKNS_15URLTextEncodingE(&url, &base, &string, nullptr);
        owner = String(url.impl);
    }
};

std::string Text(const String& value) {
    std::string text;
    if (!value.impl) return text;
    const auto view = NKWebKit::WtfAbi::ViewOf(value.impl);
    for (unsigned index = 0; index < view.length; ++index) text.push_back(static_cast<char>(NKWebKit::WtfAbi::CodeUnitAt(view, index)));
    return text;
}

String Referrer(std::uint8_t policy, const ParsedURL& destination, const String& referrer) {
    return _ZN7WebCore14SecurityPolicy22generateReferrerHeaderENS_14ReferrerPolicyERKN3WTF3URLERKNS2_6StringE(policy, &destination.url, &referrer);
}

void RequireReferrer(std::uint8_t policy, const ParsedURL& destination, const String& referrer, const char* expected, int line) {
    const String result = Referrer(policy, destination, referrer);
    const bool ok = expected ? result.impl && Text(result) == expected : !result.impl;
    if (!ok) {
        std::fprintf(stderr, "generateReferrerHeader mismatch at line %d: policy %u got \"%s\"\n", line, policy, Text(result).c_str());
        std::abort();
    }
}

#define Referred(policy, destination, referrer, expected) RequireReferrer((policy), (destination), (referrer), (expected), __LINE__)

bool ThrowsAny(std::uint8_t policy, const ParsedURL& destination, const String& referrer) {
    try {
        Referrer(policy, destination, referrer);
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

void TestReferrerHeader() {
    constexpr std::uint8_t EmptyString = 0, NoReferrer = 1, NoReferrerWhenDowngrade = 2, SameOrigin = 3, Origin = 4, StrictOrigin = 5,
        OriginWhenCrossOrigin = 6, StrictOriginWhenCrossOrigin = 7, UnsafeUrl = 8;
    const ParsedURL secure("https://target.example/page");
    const ParsedURL insecure("http://target.example/page");
    for (bool wide : {false, true}) {
        const String https = MakeString("https://source.example:8443/path?q=1", wide);
        const String http = MakeString("http://source.example/path", wide);
        Referred(EmptyString, secure, https, "https://source.example:8443/path?q=1");
        Referred(EmptyString, insecure, https, nullptr);
        Referred(NoReferrerWhenDowngrade, secure, https, "https://source.example:8443/path?q=1");
        Referred(NoReferrerWhenDowngrade, insecure, https, nullptr);
        Referred(NoReferrerWhenDowngrade, insecure, http, "http://source.example/path");
        Referred(NoReferrerWhenDowngrade, secure, http, "http://source.example/path");
        Referred(NoReferrer, secure, https, nullptr);
        Referred(NoReferrer, insecure, http, nullptr);
        Referred(Origin, secure, https, "https://source.example:8443/");
        Referred(Origin, insecure, https, "https://source.example:8443/");
        Referred(Origin, secure, http, "http://source.example/");
        Referred(StrictOrigin, secure, https, "https://source.example:8443/");
        Referred(StrictOrigin, insecure, https, nullptr);
        Referred(StrictOrigin, insecure, http, "http://source.example/");
        Referred(UnsafeUrl, insecure, https, "https://source.example:8443/path?q=1");
        Referred(9, secure, https, "https://source.example:8443/path?q=1");
        Referred(9, insecure, https, nullptr);
        for (std::uint8_t policy = EmptyString; policy <= UnsafeUrl; ++policy) {
            Referred(policy, secure, String(), nullptr);
            Referred(policy, secure, MakeString("", wide), nullptr);
            Referred(policy, secure, MakeString("ftp://source.example/", wide), nullptr);
            Referred(policy, secure, MakeString("data:text/plain,x", wide), nullptr);
            Referred(policy, secure, MakeString(" https://source.example/", wide), nullptr);
        }
        for (std::uint8_t policy : {SameOrigin, OriginWhenCrossOrigin, StrictOriginWhenCrossOrigin}) {
            if (!ThrowsAny(policy, secure, https)) std::abort();
        }
        const unsigned before = https.impl->refCount;
        {
            const String same = Referrer(UnsafeUrl, insecure, https);
            if (same.impl != https.impl || https.impl->refCount != before + 2) std::abort();
        }
        if (https.impl->refCount != before) std::abort();
        Referred(Origin, secure, MakeString("http://exa mple/", wide), nullptr);
        Referred(Origin, secure, MakeString("HTTPS://Source.Example:443/x", wide), "https://source.example/");
    }
    const String referrer = MakeString("https://source.example/", false);
    try {
        _ZN7WebCore14SecurityPolicy22generateReferrerHeaderENS_14ReferrerPolicyERKN3WTF3URLERKNS2_6StringE(Origin, nullptr, &referrer);
        std::abort();
    } catch (const std::invalid_argument&) {
    }
    try {
        _ZN7WebCore14SecurityPolicy22generateReferrerHeaderENS_14ReferrerPolicyERKN3WTF3URLERKNS2_6StringE(Origin, &secure.url, nullptr);
        std::abort();
    } catch (const std::invalid_argument&) {
    }
}

void Require(bool condition, int line) {
    if (!condition) {
        std::fprintf(stderr, "shouldHideReferrer guest pointer check failed at line %d\n", line);
        std::abort();
    }
}

bool Rejected(const String* referrer) {
    const URL url{};
    errno = 71;
    bool rejected = false;
    try {
        _ZN7WebCore14SecurityPolicy18shouldHideReferrerERKN3WTF3URLERKNS1_6StringE(&url, referrer);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    return rejected && errno == 71;
}

unsigned char* MapGuardedPage() {
#ifdef _WIN32
    auto* pages = static_cast<unsigned char*>(VirtualAlloc(nullptr, 8192, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    Require(pages != nullptr, __LINE__);
    DWORD previous = 0;
    Require(VirtualProtect(pages + 4096, 4096, PAGE_NOACCESS, &previous) != 0, __LINE__);
#else
    void* mapped = mmap(nullptr, 8192, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    Require(mapped != MAP_FAILED, __LINE__);
    auto* pages = static_cast<unsigned char*>(mapped);
    Require(mprotect(pages + 4096, 4096, PROT_NONE) == 0, __LINE__);
#endif
    return pages;
}

void UnmapGuardedPage(unsigned char* pages) {
#ifdef _WIN32
    Require(VirtualFree(pages, 0, MEM_RELEASE) != 0, __LINE__);
#else
    Require(munmap(pages, 8192) == 0, __LINE__);
#endif
}

void TestGuestPointers() {
    Require(Rejected(reinterpret_cast<const String*>(std::uintptr_t{0x10})), __LINE__);
    {
        String unmapped(reinterpret_cast<NKWebKit::WtfAbi::StringImpl*>(std::uintptr_t{0x10}));
        Require(Rejected(&unmapped), __LINE__);
        unmapped.impl = nullptr;
    }
    NKWebKit::WtfAbi::StringImpl impl{2, 0, nullptr, 4};
    String referrer(&impl);
    impl.length = INT32_MAX;
    impl.data = reinterpret_cast<const void*>(std::uintptr_t{0x10});
    Require(Rejected(&referrer), __LINE__);
    impl.hashAndFlags = 0;
    Require(Rejected(&referrer), __LINE__);

    unsigned char* pages = MapGuardedPage();
    std::memset(pages, ' ', 4096);
    impl.data = pages;
    impl.length = 8192;
    impl.hashAndFlags = 4;
    Require(Rejected(&referrer), __LINE__);

    const URL url{};
    std::memcpy(pages + 4096 - 5, "http:", 5);
    impl.data = pages + 4096 - 5;
    impl.length = 100;
    Require(!_ZN7WebCore14SecurityPolicy18shouldHideReferrerERKN3WTF3URLERKNS1_6StringE(&url, &referrer), __LINE__);
    const char16_t wide[] = u"http:";
    std::memcpy(pages + 4096 - 10, wide, 10);
    impl.data = pages + 4096 - 10;
    impl.hashAndFlags = 0;
    Require(!_ZN7WebCore14SecurityPolicy18shouldHideReferrerERKN3WTF3URLERKNS1_6StringE(&url, &referrer), __LINE__);
    UnmapGuardedPage(pages);
    referrer.impl = nullptr;
    std::puts("shouldHideReferrer: unreadable referrers throw and keep errno, reads stop at the scheme");
}

}

int main() {
    try {
        Test();
        TestGuestPointers();
        TestReferrerHeader();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "SecurityPolicy test exception: %s\n", error.what());
        return 1;
    }
}
