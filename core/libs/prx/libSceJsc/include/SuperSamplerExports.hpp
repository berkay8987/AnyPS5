#pragma once

#include "prx/libc/include/general/VabiMacros.hpp"

namespace Jsc::SuperSampler {

struct State {
    double in;
    double out;
};

} // namespace Jsc::SuperSampler

extern "C" {

void APS5_VABI _ZN3JSC22resetSuperSamplerStateEv();
Jsc::SuperSampler::State SuperSamplerSnapshot_nid_no_patch();
void SuperSamplerRecord_nid_no_patch(bool inside);

}
