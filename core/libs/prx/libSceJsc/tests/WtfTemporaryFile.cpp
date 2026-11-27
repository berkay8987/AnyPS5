#include "prx/libSceJsc/libSceJsc.hpp"
#include "prx/libc/include/Api.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libkernel/include/Api.hpp"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

using Jsc::WtfAbi::PersistenceAllocator;
using Jsc::WtfAbi::StringImpl;

static_assert(std::is_same_v<decltype(&_ZN3WTF14FileSystemImpl17openTemporaryFileERKNS_6StringERiS3_),
                             StringImpl** (APS5_VABI*)(StringImpl**, StringImpl* const*, int*, StringImpl* const*)>);

namespace {

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "openTemporaryFile check failed at line %d\n", line);
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

struct OwnedImpl {
    std::vector<unsigned char> storage;
    StringImpl impl{};

    OwnedImpl(const void* characters, unsigned length, bool is8Bit)
        : storage(static_cast<const unsigned char*>(characters), static_cast<const unsigned char*>(characters) + length * (is8Bit ? 1 : 2)) {
        impl = StringImpl{2u, length, storage.data(), is8Bit ? Jsc::WtfAbi::StringImplFlag8Bit : 0u};
    }
};

struct Opened {
    StringImpl* string;
    int handle;
};

Opened Open(StringImpl* prefix) {
    StringImpl* result = reinterpret_cast<StringImpl*>(std::uintptr_t{1});
    int handle = 77;
    StringImpl* const suffix = nullptr;
    StringImpl** returned = _ZN3WTF14FileSystemImpl17openTemporaryFileERKNS_6StringERiS3_(&result, &prefix, &handle, &suffix);
    Require(returned == &result);
    return {result, handle};
}

std::string Text8(const StringImpl* string) {
    Require(string && (string->hashAndFlags & Jsc::WtfAbi::StringImplFlag8Bit) && string->refCount == 2u);
    return std::string(static_cast<const char*>(string->data), string->length);
}

void Close(Opened& opened) {
    Require(opened.handle >= 0);
    Require(close_nid_postfix(opened.handle) == 0);
    if (opened.string) TestDeallocate(opened.string);
}

void RequireCreated(const std::filesystem::path& root, const std::string& guestPath, const std::string& namePrefix) {
    const std::string directory = "/anyps5-jsc-tmp/";
    Require(guestPath.compare(0, directory.size(), directory) == 0);
    const std::string name = guestPath.substr(directory.size());
    Require(name.size() == namePrefix.size() + 6);
    Require(name.compare(0, namePrefix.size(), namePrefix) == 0);
    Require(name.substr(namePrefix.size()) != "XXXXXX");
    Require(std::filesystem::is_regular_file(root / name));
}

void TestAsciiPrefix(const std::filesystem::path& root) {
    const char text[] = "WebKit-";
    OwnedImpl prefix(text, 7, true);
    Opened opened = Open(&prefix.impl);
    const std::string path = Text8(opened.string);
    RequireCreated(root, path, "WebKit-");
    Close(opened);
}

void TestNullAndEmptyPrefix(const std::filesystem::path& root) {
    Opened nullPrefix = Open(nullptr);
    RequireCreated(root, Text8(nullPrefix.string), "");
    Close(nullPrefix);
    OwnedImpl empty("", 0, true);
    Opened emptyPrefix = Open(&empty.impl);
    RequireCreated(root, Text8(emptyPrefix.string), "");
    Close(emptyPrefix);
}

void TestNonAsciiPrefix(const std::filesystem::path& root) {
    const unsigned char latin1[] = {'c', 0xE9};
    OwnedImpl narrow(latin1, 2, true);
    Opened opened = Open(&narrow.impl);
    Require(opened.string && !(opened.string->hashAndFlags & Jsc::WtfAbi::StringImplFlag8Bit));
    const auto* units = static_cast<const char16_t*>(opened.string->data);
    const std::u16string prefix = std::u16string(u"/anyps5-jsc-tmp/c") + char16_t{0xE9};
    Require(opened.string->length == prefix.size() + 6);
    Require(std::u16string(units, prefix.size()) == prefix);
    Require(std::filesystem::is_regular_file(root / std::filesystem::path(std::u16string(units + 16, opened.string->length - 16))));
    Close(opened);

    const char16_t wide[] = {0x3b1, 0xD83D, 0xDE00, 0xDC00, u'x'};
    OwnedImpl wideImpl(wide, 5, false);
    Opened wideOpened = Open(&wideImpl.impl);
    Require(wideOpened.string && !(wideOpened.string->hashAndFlags & Jsc::WtfAbi::StringImplFlag8Bit));
    const auto* wideUnits = static_cast<const char16_t*>(wideOpened.string->data);
    const std::u16string widePrefix = std::u16string(u"/anyps5-jsc-tmp/") + char16_t{0x3b1} + char16_t{0xD83D} + char16_t{0xDE00} + char16_t{0xFFFD} + u"x";
    Require(std::u16string(wideUnits, widePrefix.size()) == widePrefix);
    Require(wideOpened.string->length == widePrefix.size() + 6);
    Close(wideOpened);
}

void TestTrailingLeadSurrogate() {
    const char16_t wide[] = {u'a', 0xD800};
    OwnedImpl prefix(wide, 2, false);
    Opened opened = Open(&prefix.impl);
    Require(opened.string == nullptr);
    if (opened.handle >= 0) Require(close_nid_postfix(opened.handle) == 0);
    else Require(opened.handle == -1);
}

void TestEmbeddedNul(const std::filesystem::path& root) {
    const char text[] = {'a', 0, 'b'};
    OwnedImpl prefix(text, 3, true);
    Opened opened = Open(&prefix.impl);
    RequireCreated(root, Text8(opened.string), "a");
    Close(opened);
}

void TestFailures() {
    std::string longPrefix(1100, 'p');
    OwnedImpl tooLong(longPrefix.data(), static_cast<unsigned>(longPrefix.size()), true);
    const int before = allocations;
    Opened opened = Open(&tooLong.impl);
    Require(opened.string == nullptr && opened.handle == -1 && allocations == before);

    std::string fitting(1024 - std::strlen("/anyps5-jsc-tmp/XXXXXX") - 1, 'q');
    OwnedImpl edge(fitting.data(), static_cast<unsigned>(fitting.size()), true);
    Opened edgeOpened = Open(&edge.impl);
    Require(edgeOpened.string == nullptr || edgeOpened.string->length == 1023);
    if (edgeOpened.handle >= 0) Close(edgeOpened);
    else Require(edgeOpened.string == nullptr);

    std::string over(fitting.size() + 1, 'q');
    OwnedImpl overImpl(over.data(), static_cast<unsigned>(over.size()), true);
    Opened overOpened = Open(&overImpl.impl);
    Require(overOpened.string == nullptr && overOpened.handle == -1);

    Require(setenv_nid_postfix("TMPDIR", "/anyps5-jsc-missing-directory", 1) == 0);
    OwnedImpl prefix("x", 1, true);
    Opened missing = Open(&prefix.impl);
    Require(missing.string == nullptr && missing.handle == -1);
    Require(setenv_nid_postfix("TMPDIR", "/anyps5-jsc-tmp", 1) == 0);
}

void TestInvalidArguments() {
    OwnedImpl prefix("x", 1, true);
    StringImpl* prefixPointer = &prefix.impl;
    StringImpl* result = nullptr;
    int handle = 5;
    errno = 45;
    Require(Throws<std::invalid_argument>([&] {
        _ZN3WTF14FileSystemImpl17openTemporaryFileERKNS_6StringERiS3_(nullptr, &prefixPointer, &handle, nullptr);
    }));
    Require(Throws<std::invalid_argument>([&] {
        _ZN3WTF14FileSystemImpl17openTemporaryFileERKNS_6StringERiS3_(&result, nullptr, &handle, nullptr);
    }));
    Require(Throws<std::invalid_argument>([&] {
        _ZN3WTF14FileSystemImpl17openTemporaryFileERKNS_6StringERiS3_(&result, &prefixPointer, nullptr, nullptr);
    }));
    auto* const unreadable = reinterpret_cast<StringImpl*>(std::uintptr_t{0x10});
    Require(Throws<std::invalid_argument>([&] {
        _ZN3WTF14FileSystemImpl17openTemporaryFileERKNS_6StringERiS3_(&result, &unreadable, &handle, nullptr);
    }));
    StringImpl badCharacters{2u, 3u, reinterpret_cast<const void*>(std::uintptr_t{0x10}), Jsc::WtfAbi::StringImplFlag8Bit};
    StringImpl* badPointer = &badCharacters;
    Require(Throws<std::invalid_argument>([&] {
        _ZN3WTF14FileSystemImpl17openTemporaryFileERKNS_6StringERiS3_(&result, &badPointer, &handle, nullptr);
    }));
    StringImpl noCharacters{2u, 3u, nullptr, Jsc::WtfAbi::StringImplFlag8Bit};
    StringImpl* noPointer = &noCharacters;
    Require(Throws<std::invalid_argument>([&] {
        _ZN3WTF14FileSystemImpl17openTemporaryFileERKNS_6StringERiS3_(&result, &noPointer, &handle, nullptr);
    }));
    Require(errno == 45 && result == nullptr && handle == 5);
}

void TestUnregistered() {
    OwnedImpl prefix("x", 1, true);
    StringImpl* prefixPointer = &prefix.impl;
    StringImpl* result = nullptr;
    int handle = 5;
    Require(Throws<std::runtime_error>([&] {
        _ZN3WTF14FileSystemImpl17openTemporaryFileERKNS_6StringERiS3_(&result, &prefixPointer, &handle, nullptr);
    }));
    Require(result == nullptr && handle == 5);
}

}

int main() {
    std::random_device random;
    const auto root = std::filesystem::temp_directory_path() /
        ("anyps5-jsc-tmp-" + std::to_string(random()) + "-" + std::to_string(random()));
    Require(std::filesystem::create_directory(root));
    AddPathAlias_nid_no_patch("/anyps5-jsc-tmp", root.string().c_str());
    Require(setenv_nid_postfix("TMPDIR", "/anyps5-jsc-tmp", 1) == 0);

    TestUnregistered();
    constexpr PersistenceAllocator allocator{&TestAllocate, &TestDeallocate};
    RegisterPersistenceAllocator_nid_no_patch(&allocator);
    TestAsciiPrefix(root);
    TestNullAndEmptyPrefix(root);
    TestNonAsciiPrefix(root);
    TestTrailingLeadSurrogate();
    TestEmbeddedNul(root);
    TestFailures();
    TestInvalidArguments();
    Require(allocations == 0);

    RemovePathAlias_nid_no_patch("/anyps5-jsc-tmp");
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::puts("openTemporaryFile tests passed");
    return 0;
}
