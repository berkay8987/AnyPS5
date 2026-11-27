#include "prx/libSceLibcInternal/include/InventoryExports.hpp"
#include "prx/libSceLibcInternal/include/InventoryExports6.hpp"
#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/GuestLocale.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace {

void Require(bool condition, const char* message) {
    if (condition) return;
    std::fprintf(stderr, "wide num_put size constructors: %s\n", message);
    std::abort();
}

void* APS5_VABI TestAllocate(std::size_t size) { return std::malloc(size); }
void APS5_VABI TestFree(void* pointer) { std::free(pointer); }
void* APS5_VABI TestCalloc(std::size_t count, std::size_t size) { return std::calloc(count, size); }
void* APS5_VABI TestReallocate(void* pointer, std::size_t size) { return std::realloc(pointer, size); }
void* APS5_VABI TestAlign(std::size_t, std::size_t size) { return std::malloc(size); }
void* APS5_VABI TestRealign(void* pointer, std::size_t size, std::size_t) { return std::realloc(pointer, size); }
int APS5_VABI TestPosixAlign(void** pointer, std::size_t, std::size_t size) {
    *pointer = std::malloc(size);
    return *pointer ? 0 : 12;
}

}

int main() {
    const std::array<void*, 10> api{
        reinterpret_cast<void*>(&TestAllocate), reinterpret_cast<void*>(&TestFree),
        reinterpret_cast<void*>(&TestCalloc), reinterpret_cast<void*>(&TestReallocate),
        reinterpret_cast<void*>(&TestAlign), reinterpret_cast<void*>(&TestRealign),
        reinterpret_cast<void*>(&TestPosixAlign), nullptr, nullptr, nullptr};
    ApplicationHeapRegister_nid_no_patch(api.data());

    GuestLocale::LocinfoStorage info{};
    GuestLocale::Facet expected{};
    _ZNSt7num_putIwSt19ostreambuf_iteratorIwSt11char_traitsIwEEEC2ERKSt8_Locinfom_nid_postfix(&expected, &info, 0);
    Require(expected.vtable != nullptr, "the _Locinfo constructor installs the num_put<wchar_t> vtable");

    using Constructor = void (APS5_VABI *)(GuestLocale::Facet*, std::size_t);
    for (const Constructor construct : {libSceLibcInternalInventory_134_nid_no_patch, libSceLibcInternalInventory_135_nid_no_patch}) {
        for (const std::size_t refs : {std::size_t{0}, std::size_t{3}, std::size_t{0x100000007}}) {
            GuestLocale::Facet facet{nullptr, 0xffffffffu, 0xffffffffu};
            construct(&facet, refs);
            Require(facet.vtable == expected.vtable, "num_put<wchar_t>(size_t) installs the num_put<wchar_t> vtable");
            Require(facet.references == static_cast<std::uint32_t>(refs), "num_put<wchar_t>(size_t) stores the low half of the reference count");
            Require(facet.reserved == static_cast<std::uint32_t>(static_cast<std::uint64_t>(refs) >> 32), "num_put<wchar_t>(size_t) stores the rest of the reference count as the _Locinfo constructor does");
        }
        bool rejected = false;
        try {
            construct(nullptr, 0);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        Require(rejected, "num_put<wchar_t>(size_t) rejects a null facet");
    }

    std::puts("wide num_put size constructor checks passed");
    return 0;
}
