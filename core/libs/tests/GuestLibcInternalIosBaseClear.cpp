#include "prx/libSceLibcInternal/include/InventoryExports.hpp"
#include "prx/libc/include/GuestLocale.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {

constexpr std::int32_t EofBit = 0x1;
constexpr std::int32_t FailBit = 0x2;
constexpr std::int32_t BadBit = 0x4;
constexpr std::int32_t HardFail = 0x10;

void Require(bool condition, const char* message) {
    if (condition) return;
    std::fprintf(stderr, "ios_base::clear: %s\n", message);
    std::abort();
}

std::string ThrownMessage(GuestLocale::IosBase& stream, std::int32_t state, bool reraise) {
    try {
        libSceLibcInternalInventory_147_nid_no_patch(&stream, state, reraise);
    } catch (const std::runtime_error& error) {
        return error.what();
    }
    return {};
}

}

int main() {
    GuestLocale::IosBase stream{};
    stream.state = FailBit;
    stream.flags = 0x201;
    libSceLibcInternalInventory_147_nid_no_patch(&stream, 0, false);
    Require(stream.state == 0 && stream.flags == 0x201, "clear(goodbit) resets the state and nothing else");

    libSceLibcInternalInventory_147_nid_no_patch(&stream, -1, false);
    Require(stream.state == (EofBit | FailBit | BadBit | HardFail), "clear keeps only the bits of _Statmask");

    libSceLibcInternalInventory_147_nid_no_patch(&stream, 0x8 | EofBit, true);
    Require(stream.state == EofBit, "clear drops bits outside _Statmask and ignores _Reraise when no exception is enabled");

    stream.exceptions = BadBit;
    libSceLibcInternalInventory_147_nid_no_patch(&stream, FailBit | EofBit, false);
    Require(stream.state == (FailBit | EofBit), "clear does not throw for states outside the exception mask");

    stream.exceptions = FailBit;
    Require(ThrownMessage(stream, FailBit, false).find("ios_base::failure") != std::string::npos, "an enabled failbit reaches the unimplemented ios_base::failure path");
    Require(stream.state == FailBit, "clear stores the state before it throws");
    Require(ThrownMessage(stream, FailBit | BadBit, true).find("rethrowing") != std::string::npos, "an enabled state with _Reraise reaches the unimplemented rethrow path");
    Require(stream.state == (FailBit | BadBit), "clear stores the state before it rethrows");

    bool rejected = false;
    try {
        libSceLibcInternalInventory_147_nid_no_patch(nullptr, 0, false);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    Require(rejected, "clear rejects a null stream");

    std::puts("ios_base::clear checks passed");
    return 0;
}
