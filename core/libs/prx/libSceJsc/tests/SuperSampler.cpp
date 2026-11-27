#include "prx/libSceJsc/libSceJsc.hpp"

#include <cstdio>
#include <cstdlib>
#include <thread>
#include <type_traits>
#include <vector>

static_assert(std::is_same_v<decltype(&_ZN3JSC22resetSuperSamplerStateEv), void (APS5_VABI*)()>);

namespace {

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "SuperSampler check failed at line %d\n", line);
        std::abort();
    }
}

#define Require(value) Check((value), __LINE__)

void TestResetClearsCounters() {
    _ZN3JSC22resetSuperSamplerStateEv();
    auto state = SuperSamplerSnapshot_nid_no_patch();
    Require(state.in == 0 && state.out == 0);
    SuperSamplerRecord_nid_no_patch(true);
    SuperSamplerRecord_nid_no_patch(true);
    SuperSamplerRecord_nid_no_patch(false);
    state = SuperSamplerSnapshot_nid_no_patch();
    Require(state.in == 2 && state.out == 1);
    _ZN3JSC22resetSuperSamplerStateEv();
    state = SuperSamplerSnapshot_nid_no_patch();
    Require(state.in == 0 && state.out == 0);
    _ZN3JSC22resetSuperSamplerStateEv();
    state = SuperSamplerSnapshot_nid_no_patch();
    Require(state.in == 0 && state.out == 0);
}

void TestConcurrentResets() {
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([t] {
            for (int i = 0; i < 2000; ++i) {
                if (t % 2) SuperSamplerRecord_nid_no_patch(i % 2);
                else _ZN3JSC22resetSuperSamplerStateEv();
            }
        });
    }
    for (auto& thread : threads) thread.join();
    _ZN3JSC22resetSuperSamplerStateEv();
    const auto state = SuperSamplerSnapshot_nid_no_patch();
    Require(state.in == 0 && state.out == 0);
}

}

int main() {
    TestResetClearsCounters();
    TestConcurrentResets();
    std::puts("SuperSampler tests passed");
    return 0;
}
