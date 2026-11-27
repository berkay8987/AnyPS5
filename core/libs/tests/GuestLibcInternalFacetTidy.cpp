#include "prx/libSceLibcInternal/include/FacetReferences.hpp"
#include "prx/libSceLibcInternal/include/RuntimeInternals.hpp"
#include "prx/libc/include/Api.hpp"
#include "prx/libc/include/GuestLocale.hpp"
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

namespace {

std::array<GuestLocale::Facet*, 4> deleted{};
std::size_t deletedCount = 0;

void Require(bool condition, const char* message) {
    if (condition) return;
    std::fprintf(stderr, "libc internal _Fac_tidy: %s\n", message);
    std::abort();
}

void APS5_VABI Destroy(GuestLocale::Facet*) {
    Require(false, "_Fac_tidy runs the deleting destructor, not the complete one");
}

void APS5_VABI DeleteObject(GuestLocale::Facet* self) {
    Require(deletedCount < deleted.size(), "_Fac_tidy deletes each facet once");
    deleted[deletedCount++] = self;
}

void APS5_VABI Retain(GuestLocale::Facet* self) {
    _ZNSt6locale5facet7_IncrefEv_nid_postfix(self);
}

GuestLocale::Facet* APS5_VABI Release(GuestLocale::Facet* self) {
    return _ZNSt6locale5facet7_DecrefEv_nid_postfix(self);
}

const GuestLocale::FacetVtable vtable{Destroy, DeleteObject, Retain, Release};

}

int main() {
    std::array<GuestLocale::Facet, 3> facets{{{&vtable, 1, 0}, {&vtable, 2, 0}, {&vtable, 1, 0}}};
    for (GuestLocale::Facet& facet : facets) _ZNSt6locale5facet9_RegisterEv_nid_postfix(&facet);

    _Fac_tidy_nid_postfix();
    Require(deletedCount == 2, "_Fac_tidy deletes the facets whose last reference it drops");
    Require(deleted[0] == &facets[2] && deleted[1] == &facets[0], "_Fac_tidy releases the most recently registered facet first");
    Require(facets[1].references == 1, "_Fac_tidy drops one reference of a facet that is still in use");

    _Fac_tidy_nid_postfix();
    Require(deletedCount == 2 && facets[1].references == 1, "a second _Fac_tidy finds no registered facets");

    _ZNSt6locale5facet9_RegisterEv_nid_postfix(&facets[1]);
    _Fac_tidy_nid_postfix();
    Require(deletedCount == 3 && deleted[2] == &facets[1], "a facet registered again after _Fac_tidy is released by the next one");

    std::puts("libc internal _Fac_tidy checks passed");
    return 0;
}
