#include "prx/libSceJsc/include/InspectorAgentNewExports.hpp"
#include "prx/libSceJsc/src/WtfFastMalloc.hpp"

#include <new>
#include <stdexcept>
#include <string>

namespace {

void* FastMalloc(std::size_t size, const char* function) {
    const auto* allocator = Jsc::WtfAbi::RegisteredFastMalloc();
    if (!allocator || !allocator->allocate) throw std::runtime_error(std::string(function) + ": requires libSceNKWebKit fastMalloc");
    void* memory = allocator->allocate(size);
    if (!memory) throw std::bad_alloc();
    return memory;
}

}

extern "C" {

void* APS5_VABI _ZN9Inspector14InspectorAgentnaEm(std::size_t size) {
    return FastMalloc(size, "Inspector::InspectorAgent::operator new[]");
}

void* APS5_VABI _ZN9Inspector22InspectorDebuggerAgentnaEm(std::size_t size) {
    return FastMalloc(size, "Inspector::InspectorDebuggerAgent::operator new[]");
}

}
