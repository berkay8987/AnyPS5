#pragma once

#include "prx/libc/include/general/VabiMacros.hpp"

extern "C" {

void APS5_VABI JSGlobalContextSetDebuggerRunLoopWithCurrentRunLoop(const void* context);
void APS5_VABI JSGlobalContextUnsetDebuggerRunLoop(const void* context);

}
