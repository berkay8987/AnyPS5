#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceNKWebKit/libSceNKWebKit.hpp"

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>

using NKWebKit::WebCoreAbi::LegacyOriginPort;
using NKWebKit::WebCoreAbi::SecurityOrigin;
using NKWebKit::WtfAbi::String;
using OriginRef = WebCore::Ref<SecurityOrigin>;

namespace {

void Check(bool value, int line) {
    if (value) return;
    std::fprintf(stderr, "SecurityOrigin::create(protocol, host, port) check failed at line %d\n", line);
    std::abort();
}

#define Require(value) Check((value), __LINE__)

String Text(std::string_view text) {
    return _ZN3WTF10StringImpl6createEPKhj(reinterpret_cast<const unsigned char*>(text.data()), static_cast<unsigned>(text.size()));
}

std::string Narrow(const String& value) {
    if (!value.impl) return "<null>";
    std::string result;
    for (unsigned i = 0; i < value.impl->length; ++i) {
        const char16_t unit = (value.impl->hashAndFlags & 4) ? static_cast<const unsigned char*>(value.impl->data)[i] : static_cast<const char16_t*>(value.impl->data)[i];
        result.push_back(unit < 0x80 ? static_cast<char>(unit) : '?');
    }
    return result;
}

OriginRef Create(std::string_view protocol, std::string_view host, LegacyOriginPort port) {
    const String protocolText = Text(protocol);
    const String hostText = Text(host);
    return _ZN7WebCore14SecurityOrigin6createERKN3WTF6StringES4_NS1_8OptionalItEE(&protocolText, &hostText, &port);
}

std::string Origin(std::string_view protocol, std::string_view host, LegacyOriginPort port) {
    OriginRef origin = Create(protocol, host, port);
    Require(origin.get() != nullptr && origin.get()->refCount.load() == 1);
    return Narrow(_ZNK7WebCore14SecurityOrigin8toStringEv(origin.get()));
}

}

int main() {
    Require(Origin("https", "example.com", {true, 8443}) == "https://example.com:8443");
    Require(Origin("https", "example.com", {true, 443}) == "https://example.com");
    Require(Origin("http", "Example.COM", {false, 0}) == "http://example.com");
    Require(Origin("http", "example.com", {false, 8080}) == "http://example.com");
    Require(Origin("ws", "example.com", {true, 80}) == "ws://example.com");
    Require(Origin("http", "example.com", {true, 443}) == "http://example.com:443");
    Require(Origin("data", "example.com", {true, 99}) == "null");

    OriginRef fromParts = Create("https", "example.com", {true, 443});
    const String text = Text("https://example.com");
    OriginRef fromString = _ZN7WebCore14SecurityOrigin16createFromStringERKN3WTF6StringE(&text);
    Require(_ZNK7WebCore14SecurityOrigin20isSameSchemeHostPortERKS0_(fromParts.get(), fromString.get()));
    Require(_ZNK7WebCore14SecurityOrigin14isSameOriginAsERKS0_(fromParts.get(), fromString.get()));

    bool rejected = false;
    try {
        const String protocolText = Text("https");
        const String hostText = Text("example.com");
        _ZN7WebCore14SecurityOrigin6createERKN3WTF6StringES4_NS1_8OptionalItEE(&protocolText, &hostText, nullptr);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    Require(rejected);

    std::puts("SecurityOrigin::create(protocol, host, port) checks passed");
    return 0;
}
