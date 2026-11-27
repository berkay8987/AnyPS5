#include "prx/libSceLibcInternal/include/InventoryExports.hpp"
#include "prx/libc/include/Api.hpp"
#include "prx/libc/include/GuestLocale.hpp"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace {

void Require(bool condition, const char* message) {
    if (condition) return;
    std::fprintf(stderr, "locale::_Setgloballocale: %s\n", message);
    std::abort();
}

}

int main() {
    GuestLocale::Implementation* original = _ZNSt6locale16_GetgloballocaleEv_nid_postfix();
    Require(original != nullptr, "libc starts with a global locale");

    GuestLocale::Implementation replacement = *original;
    libSceLibcInternalInventory_073_nid_no_patch(&replacement);
    Require(_ZNSt6locale16_GetgloballocaleEv_nid_postfix() == &replacement, "_Setgloballocale stores the pointer _Getgloballocale returns");
    Require(replacement.base.references == original->base.references, "_Setgloballocale leaves the reference count alone");

    bool rejected = false;
    try {
        libSceLibcInternalInventory_073_nid_no_patch(nullptr);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    Require(rejected, "_Setgloballocale rejects a null locale");
    Require(_ZNSt6locale16_GetgloballocaleEv_nid_postfix() == &replacement, "a rejected null locale keeps the stored pointer");

    libSceLibcInternalInventory_073_nid_no_patch(original);
    Require(_ZNSt6locale16_GetgloballocaleEv_nid_postfix() == original, "_Setgloballocale restores the original locale");

    std::puts("locale::_Setgloballocale checks passed");
    return 0;
}
