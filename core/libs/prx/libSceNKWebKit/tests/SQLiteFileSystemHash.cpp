#include "prx/libSceNKWebKit/include/LegacySQLiteFileSystemExports.hpp"
#include "prx/libSceNKWebKit/include/WtfExports.hpp"
#include "prx/libSceNKWebKit/src/WtfStringAbi.hpp"
#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

using NKWebKit::WtfAbi::String;

void Check(bool condition, int line) {
    if (condition) return;
    std::fprintf(stderr, "SQLiteFileSystem::computeHashForFileName check failed at line %d\n", line);
    std::exit(1);
}
#define Require(value) Check((value), __LINE__)

String Make(const std::string& text) {
    return _ZN3WTF6String8fromUTF8EPKhm(reinterpret_cast<const unsigned char*>(text.data()), text.size());
}

std::string Text(const String& string) {
    Require(string.impl != nullptr);
    const auto view = NKWebKit::WtfAbi::ViewOf(string.impl);
    std::string text;
    for (unsigned index = 0; index < view.length; ++index) text.push_back(static_cast<char>(NKWebKit::WtfAbi::CodeUnitAt(view, index)));
    return text;
}

void* APS5_VABI HeapAllocate(std::size_t bytes) { return std::malloc(bytes == 0 ? 1 : bytes); }
void APS5_VABI HeapFree(void* pointer) { std::free(pointer); }
void* APS5_VABI HeapCalloc(std::size_t count, std::size_t bytes) { return std::calloc(count == 0 ? 1 : count, bytes == 0 ? 1 : bytes); }
void* APS5_VABI HeapReallocate(void* pointer, std::size_t bytes) { return std::realloc(pointer, bytes == 0 ? 1 : bytes); }

void* APS5_VABI HeapAlign(std::size_t, std::size_t bytes) { return std::malloc(bytes == 0 ? 1 : bytes); }
void* APS5_VABI HeapRealign(void* pointer, std::size_t bytes, std::size_t) { return std::realloc(pointer, bytes == 0 ? 1 : bytes); }
int APS5_VABI HeapPosixAlign(void** pointer, std::size_t, std::size_t bytes) {
    *pointer = std::malloc(bytes == 0 ? 1 : bytes);
    return *pointer ? 0 : 12;
}

void RegisterHeap() {
    std::array<void*, 10> api{};
    api[0] = reinterpret_cast<void*>(&HeapAllocate);
    api[1] = reinterpret_cast<void*>(&HeapFree);
    api[2] = reinterpret_cast<void*>(&HeapCalloc);
    api[3] = reinterpret_cast<void*>(&HeapReallocate);
    api[4] = reinterpret_cast<void*>(&HeapAlign);
    api[5] = reinterpret_cast<void*>(&HeapRealign);
    api[6] = reinterpret_cast<void*>(&HeapPosixAlign);
    ApplicationHeapRegister_nid_no_patch(api.data());
}

std::string Hash(const String& name) {
    return Text(_ZN7WebCore16SQLiteFileSystem22computeHashForFileNameERKN3WTF6StringE(name));
}

}

int main() {
    RegisterHeap();
    Require(Hash(Make("abc")) == "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD");
    Require(Hash(Make("https_example.com_0")) == "A05523AA7038E9ADC16C59284190A5A16EDBAE3DDB398A00AECF0E777F4AC669");
    Require(Hash(Make("caf\xC3\xA9.db")) == "CAD56EA645200B42C0911BF591AB8D45408AFE34FAF8CF80C5FBB9B8BF01938B");
    const char16_t wide[] = {u'c', u'a', u'f', 0xE9, u'.', u'd', u'b'};
    String utf16{};
    _ZN3WTF6StringC1EPKDsj(&utf16, wide, 7);
    Require(Hash(utf16) == "CAD56EA645200B42C0911BF591AB8D45408AFE34FAF8CF80C5FBB9B8BF01938B");
    Require(Hash(Make("")) == "E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855");
    Require(Hash(String{}) == "E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855");
    const String hashed = _ZN7WebCore16SQLiteFileSystem22computeHashForFileNameERKN3WTF6StringE(Make("abc"));
    Require((hashed.impl->hashAndFlags & 4) != 0 && hashed.impl->length == 64);
    std::puts("SQLiteFileSystem::computeHashForFileName checks passed");
    return 0;
}
