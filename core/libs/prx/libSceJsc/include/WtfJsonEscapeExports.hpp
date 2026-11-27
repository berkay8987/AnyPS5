#pragma once

#include "prx/libSceJsc/include/ApiBridgeExports.hpp"
#include "prx/libSceJsc/src/WtfStringAbi.hpp"
#include "prx/libSceJsc/src/WtfUrlAbi.hpp"

extern "C" {

void APS5_VABI _ZN3WTF8JSONImpl5Value12escapeStringERNS_13StringBuilderENS_10StringViewE(Jsc::WtfAbi::StringBuilder* builder, Jsc::WtfAbi::StringView string);

}
