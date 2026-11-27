#include "prx/libSceNKWebKit/include/PropertyDescriptorExports.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

using NKWebKit::JscAbi::PropertyDescriptor;

namespace {

void Check(bool value, int line) {
    if (value) return;
    std::fprintf(stderr, "PropertyDescriptor::setDescriptor check failed at line %d\n", line);
    std::abort();
}

#define Require(value) Check((value), __LINE__)

constexpr std::uint32_t ReadOnly = 1u << 1;
constexpr std::uint32_t DontEnum = 1u << 2;
constexpr std::uint32_t Accessor = 1u << 4;
constexpr std::uint32_t CustomAccessor = 1u << 5;
constexpr std::uint32_t CustomValue = 1u << 6;
constexpr std::uint32_t AllPresent = 1 | 2 | 4;
constexpr std::uint64_t Int32Seven = 0xfffe000000000007ull;
constexpr std::uint64_t EncodedNull = 0x2;
constexpr std::uint64_t Getter = 0x1111;
constexpr std::uint64_t Setter = 0x2222;

struct alignas(16) FakeCell {
    std::uint32_t structureID;
    std::uint8_t indexingType;
    std::uint8_t type;
    std::uint8_t flags;
    std::uint8_t cellState;
    std::uint64_t payload[2];
};

PropertyDescriptor Fresh() {
    PropertyDescriptor descriptor{};
    descriptor.value = 0;
    descriptor.getter = Getter;
    descriptor.setter = Setter;
    descriptor.attributes = 0xffffffffu;
    descriptor.seenAttributes = 0;
    return descriptor;
}

void CheckDataValue(std::uint64_t value, std::uint32_t attributes, std::uint32_t expectedAttributes) {
    PropertyDescriptor descriptor = Fresh();
    _ZN3JSC18PropertyDescriptor13setDescriptorENS_7JSValueEj(&descriptor, value, attributes);
    Require(descriptor.value == value);
    Require(descriptor.attributes == expectedAttributes);
    Require(descriptor.seenAttributes == AllPresent);
    Require(descriptor.getter == Getter && descriptor.setter == Setter);
    Require(_ZNK3JSC18PropertyDescriptor16isDataDescriptorEv(&descriptor));
}

template <typename Exception, typename Call>
std::string Thrown(Call call) {
    try {
        call();
    } catch (const Exception& error) {
        return error.what();
    }
    return {};
}

}

int main() {
    CheckDataValue(Int32Seven, ReadOnly | DontEnum | CustomValue, ReadOnly | DontEnum);
    CheckDataValue(EncodedNull, CustomAccessor | CustomValue, CustomAccessor);
    CheckDataValue(Int32Seven, Accessor | ReadOnly, Accessor | ReadOnly);

    FakeCell object{};
    object.type = 22;
    CheckDataValue(reinterpret_cast<std::uintptr_t>(&object), DontEnum, DontEnum);

    FakeCell getterSetter{};
    getterSetter.type = 5;
    PropertyDescriptor untouched = Fresh();
    const std::string message = Thrown<std::runtime_error>([&] {
        _ZN3JSC18PropertyDescriptor13setDescriptorENS_7JSValueEj(&untouched, reinterpret_cast<std::uintptr_t>(&getterSetter), Accessor);
    });
    Require(message.find("isGetterNull") != std::string::npos);
    const PropertyDescriptor expected = Fresh();
    Require(std::memcmp(&untouched, &expected, sizeof(expected)) == 0);

    PropertyDescriptor empty = Fresh();
    Require(!Thrown<std::invalid_argument>([&] { _ZN3JSC18PropertyDescriptor13setDescriptorENS_7JSValueEj(&empty, 0, 0); }).empty());
    Require(!Thrown<std::invalid_argument>([] { _ZN3JSC18PropertyDescriptor13setDescriptorENS_7JSValueEj(nullptr, Int32Seven, 0); }).empty());

    std::puts("PropertyDescriptor::setDescriptor checks passed");
    return 0;
}
