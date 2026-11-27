#pragma once

#include "prx/libSceJsc/src/WtfStringAbi.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

extern "C" {

Jsc::WtfAbi::StringImpl** APS5_VABI _ZN3WTF14FileSystemImpl17openTemporaryFileERKNS_6StringERiS3_(Jsc::WtfAbi::StringImpl** result,
    Jsc::WtfAbi::StringImpl* const* prefix, int* handle, Jsc::WtfAbi::StringImpl* const* suffix);

}
