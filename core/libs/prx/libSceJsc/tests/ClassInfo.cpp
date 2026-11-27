#include "prx/libSceJsc/libSceJsc.hpp"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <type_traits>

using Jsc::RuntimeAbi::ClassInfo;
using Jsc::RuntimeAbi::HashTable;

static_assert(std::is_same_v<decltype(&_ZNK3JSC9ClassInfo35hasStaticSetterOrReadonlyPropertiesEv),
                             bool (APS5_VABI*)(const ClassInfo*)>);

namespace {

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "ClassInfo check failed at line %d\n", line);
        std::abort();
    }
}

#define Require(value) Check((value), __LINE__)

bool Has(const ClassInfo* info) {
    return _ZNK3JSC9ClassInfo35hasStaticSetterOrReadonlyPropertiesEv(info);
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

HashTable Table(bool flag) {
    HashTable table{};
    table.numberOfValues = 3;
    table.indexMask = 7;
    table.hasSetterOrReadonlyProperties = flag;
    return table;
}

void TestOwnTable() {
    const HashTable withFlag = Table(true);
    const HashTable withoutFlag = Table(false);
    const ClassInfo none{"None", nullptr, nullptr};
    const ClassInfo set{"Set", nullptr, &withFlag};
    const ClassInfo clear{"Clear", nullptr, &withoutFlag};
    Require(!Has(&none));
    Require(Has(&set));
    Require(!Has(&clear));
}

void TestParentChain() {
    const HashTable withFlag = Table(true);
    const HashTable withoutFlag = Table(false);
    const ClassInfo root{"Root", nullptr, &withFlag};
    const ClassInfo middle{"Middle", &root, nullptr};
    const ClassInfo leaf{"Leaf", &middle, &withoutFlag};
    Require(Has(&leaf));
    Require(Has(&middle));
    const ClassInfo plainRoot{"PlainRoot", nullptr, &withoutFlag};
    const ClassInfo plainMiddle{"PlainMiddle", &plainRoot, nullptr};
    const ClassInfo plainLeaf{"PlainLeaf", &plainMiddle, &withoutFlag};
    Require(!Has(&plainLeaf));
    const ClassInfo flaggedLeaf{"FlaggedLeaf", &plainMiddle, &withFlag};
    Require(Has(&flaggedLeaf));
}

void TestNonzeroFlagByte() {
    HashTable table = Table(false);
    const std::uint8_t two = 2;
    std::memcpy(&table.hasSetterOrReadonlyProperties, &two, sizeof(two));
    const ClassInfo info{"Two", nullptr, &table};
    Require(Has(&info));
}

void TestStopsAtFirstFlag() {
    const HashTable withFlag = Table(true);
    const auto* unreadableTable = reinterpret_cast<const HashTable*>(std::uintptr_t{0x10});
    const ClassInfo broken{"Broken", nullptr, unreadableTable};
    const ClassInfo leaf{"Leaf", &broken, &withFlag};
    Require(Has(&leaf));
}

void TestInvalidArguments() {
    errno = 37;
    Require(Throws([] { Has(nullptr); }));
    const auto* unreadableInfo = reinterpret_cast<const ClassInfo*>(std::uintptr_t{0x10});
    Require(Throws([&] { Has(unreadableInfo); }));
    Require(errno == 37);
    const auto* unreadableTable = reinterpret_cast<const HashTable*>(std::uintptr_t{0x20});
    const ClassInfo broken{"Broken", nullptr, unreadableTable};
    Require(Throws([&] { Has(&broken); }));
    Require(errno == 37);
    const ClassInfo leaf{"Leaf", unreadableInfo, nullptr};
    Require(Throws([&] { Has(&leaf); }));
    ClassInfo first{"First", nullptr, nullptr};
    const ClassInfo second{"Second", &first, nullptr};
    first.parentClass = &second;
    Require(Throws([&] { Has(&first); }));
    ClassInfo self{"Self", nullptr, nullptr};
    self.parentClass = &self;
    Require(Throws([&] { Has(&self); }));
}

}

int main() {
    TestOwnTable();
    TestParentChain();
    TestNonzeroFlagByte();
    TestStopsAtFirstFlag();
    TestInvalidArguments();
    std::puts("ClassInfo tests passed");
    return 0;
}
