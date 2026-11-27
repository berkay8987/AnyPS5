#include "prx/libSceJsc/libSceJsc.hpp"
#include "prx/libc/include/Api.hpp"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <type_traits>

using Jsc::WtfAbi::PersistenceAllocator;
using Jsc::WtfAbi::StringImpl;

static_assert(std::is_same_v<decltype(&_ZN3WTF15defaultLanguageEv), StringImpl** (APS5_VABI*)(StringImpl**)>);

namespace {

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "defaultLanguage check failed at line %d\n", line);
        std::abort();
    }
}

#define Require(value) Check((value), __LINE__)

int allocations = 0;

void* TestAllocate(std::size_t size) {
    ++allocations;
    return std::malloc(size);
}

void TestDeallocate(void* pointer) {
    --allocations;
    std::free(pointer);
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

std::string Language() {
    StringImpl* result = nullptr;
    errno = 71;
    Require(_ZN3WTF15defaultLanguageEv(&result) == &result);
    Require(errno == 71);
    Require(result && result->refCount == 2u && (result->hashAndFlags & Jsc::WtfAbi::StringImplFlag8Bit));
    Require(result->data == reinterpret_cast<const unsigned char*>(result) + offsetof(StringImpl, hashAndFlags) + sizeof(unsigned));
    std::string text(static_cast<const char*>(result->data), result->length);
    TestDeallocate(result);
    return text;
}

void SetLanguage(const char* value) {
    Require(setenv_nid_postfix("SYSTEM_LANGUAGE", value, 1) == 0);
}

}

int main() {
    Require(unsetenv_nid_postfix("SYSTEM_LANGUAGE") == 0);
    StringImpl* result = nullptr;
    Require(Throws<std::runtime_error>([&] { _ZN3WTF15defaultLanguageEv(&result); }));
    Require(result == nullptr);

    constexpr PersistenceAllocator allocator{&TestAllocate, &TestDeallocate};
    RegisterPersistenceAllocator_nid_no_patch(&allocator);

    Require(Language().empty());
    SetLanguage("ja-JP");
    Require(Language() == "ja-JP");
    SetLanguage("en-US");
    Require(Language() == "en-US");
    SetLanguage("");
    Require(Language().empty());
    SetLanguage("abcdefghijklmno");
    Require(Language() == "abcdefghijklmno");
    const char latin1[] = {'f', 'r', static_cast<char>(0xE9), 0};
    SetLanguage(latin1);
    Require(Language() == std::string(latin1));

    SetLanguage("abcdefghijklmnop");
    Require(Throws<std::runtime_error>([&] { _ZN3WTF15defaultLanguageEv(&result); }));
    SetLanguage("en-US");

    Require(Throws<std::invalid_argument>([] { _ZN3WTF15defaultLanguageEv(nullptr); }));
    errno = 72;
    auto* unwritable = reinterpret_cast<StringImpl**>(std::uintptr_t{0x10});
    Require(Throws<std::invalid_argument>([&] { _ZN3WTF15defaultLanguageEv(unwritable); }));
    Require(errno == 72);
    Require(result == nullptr && allocations == 0);
    std::puts("defaultLanguage tests passed");
    return 0;
}
