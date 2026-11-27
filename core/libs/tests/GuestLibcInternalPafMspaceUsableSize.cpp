#include "prx/libc/include/Api.hpp"
#include "prx/libc/include/GuestErrno.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

extern "C" std::size_t APS5_VABI sceLibcInternalUnknown6_032_6JcY5RDA4jY_nid_no_patch(const void* pointer);

namespace {

void Require(bool condition, const char* message) {
    if (condition) return;
    std::fprintf(stderr, "sceLibcPafMspaceMallocUsableSize: %s\n", message);
    std::abort();
}

alignas(64) unsigned char arenaMemory[4096];

}

int main() {
    void* mspace = sceLibcPafMspaceCreate_nid_postfix("usable", arenaMemory, sizeof(arenaMemory), 0, nullptr);
    Require(mspace != nullptr, "the PAF mspace is created");

    void* block = sceLibcPafMspaceMalloc_nid_postfix(mspace, 40, nullptr);
    Require(block != nullptr, "the PAF mspace allocates");
    const std::size_t usable = sceLibcInternalUnknown6_032_6JcY5RDA4jY_nid_no_patch(block);
    Require(usable >= 40 && usable == sceLibcMspaceMallocUsableSize_nid_postfix(block), "a live block reports the usable size the mspace gave it");

    Require(sceLibcInternalUnknown6_032_6JcY5RDA4jY_nid_no_patch(nullptr) == 0, "a null pointer has no usable size");

    *__error_nid_postfix() = 0;
    Require(sceLibcInternalUnknown6_032_6JcY5RDA4jY_nid_no_patch(arenaMemory + 8) == 0 && *__error_nid_postfix() == EINVAL, "a pointer that is not a live block reports 0 and EINVAL");

    sceLibcPafMspaceFree_nid_postfix(mspace, block, nullptr);
    *__error_nid_postfix() = 0;
    Require(sceLibcInternalUnknown6_032_6JcY5RDA4jY_nid_no_patch(block) == 0 && *__error_nid_postfix() == EINVAL, "a freed block reports 0 and EINVAL");

    Require(sceLibcPafMspaceDestroy_nid_postfix(mspace) == 0, "the PAF mspace is destroyed");
    std::puts("sceLibcPafMspaceMallocUsableSize checks passed");
    return 0;
}
