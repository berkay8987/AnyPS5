#pragma once

#include "prx/libSceJsc/include/WtfDataLogExports.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>

namespace Jsc::JitAbi {

struct RegisterSet {
    std::uint64_t bits;
};

static_assert(sizeof(RegisterSet) == 8);

} // namespace Jsc::JitAbi

extern "C" {

void APS5_VABI _ZNK3JSC11RegisterSet4dumpERN3WTF11PrintStreamE(const Jsc::JitAbi::RegisterSet* self, Jsc::WtfAbi::PrintStream* out);

}
