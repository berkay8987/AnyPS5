#pragma once

#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstddef>

namespace Jsc::RuntimeAbi {

struct HashTable {
    int numberOfValues;
    int indexMask;
    bool hasSetterOrReadonlyProperties;
    const void* classForThis;
    const void* values;
    const void* index;
};

struct ClassInfo {
    const char* className;
    const ClassInfo* parentClass;
    const HashTable* staticPropHashTable;
};

static_assert(sizeof(HashTable) == 40);
static_assert(offsetof(HashTable, hasSetterOrReadonlyProperties) == 8);
static_assert(offsetof(HashTable, classForThis) == 16);
static_assert(offsetof(ClassInfo, parentClass) == 8);
static_assert(offsetof(ClassInfo, staticPropHashTable) == 16);

} // namespace Jsc::RuntimeAbi

extern "C" {

bool APS5_VABI _ZNK3JSC9ClassInfo35hasStaticSetterOrReadonlyPropertiesEv(const Jsc::RuntimeAbi::ClassInfo* self);

}
