#include "prx/libSceJsc/libSceJsc.hpp"
#include "prx/libc/include/Api.hpp"
#include "prx/libc/include/GuestTime.hpp"

#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <type_traits>

using Jsc::RuntimeAbi::DateTimeFormat;
using Jsc::RuntimeAbi::GregorianDateTime;
using Jsc::WtfAbi::PersistenceAllocator;
using Jsc::WtfAbi::StringImpl;

static_assert(std::is_same_v<decltype(&_ZN3JSC14formatDateTimeERKN3WTF17GregorianDateTimeENS_14DateTimeFormatEb),
                             StringImpl** (APS5_VABI*)(StringImpl**, const GregorianDateTime*, DateTimeFormat, bool)>);

namespace {

void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "formatDateTime check failed at line %d\n", line);
        std::abort();
    }
}

#define Require(value) Check((value), __LINE__)

int allocations = 0;

void* TestAllocate(std::size_t size) {
    ++allocations;
    return std::malloc(size);
}

void TestDeallocate(void* pointer) {
    --allocations;
    std::free(pointer);
}

template<typename TException, typename TFunction>
bool Throws(TFunction function) {
    try {
        function();
    } catch (const TException&) {
        return true;
    }
    return false;
}

GregorianDateTime Sample() {
    GregorianDateTime value{};
    value.year = 2024;
    value.month = 1;
    value.yearDay = 59;
    value.monthDay = 29;
    value.weekDay = 4;
    value.hour = 7;
    value.minute = 5;
    value.second = 9;
    value.utcOffsetInMinute = -330;
    value.isDST = 0;
    return value;
}

std::string Format(const GregorianDateTime& value, int format, bool asUTCVariant) {
    StringImpl* result = nullptr;
    errno = 61;
    StringImpl** returned = _ZN3JSC14formatDateTimeERKN3WTF17GregorianDateTimeENS_14DateTimeFormatEb(&result, &value,
        static_cast<DateTimeFormat>(format), asUTCVariant);
    Require(errno == 61);
    Require(returned == &result && result != nullptr);
    Require(result->refCount == 2u && (result->hashAndFlags & Jsc::WtfAbi::StringImplFlag8Bit));
    Require(result->data == reinterpret_cast<const unsigned char*>(result) + offsetof(StringImpl, hashAndFlags) + sizeof(unsigned));
    std::string text(static_cast<const char*>(result->data), result->length);
    TestDeallocate(result);
    return text;
}

std::string ZoneSuffix(const GregorianDateTime& value) {
    GuestTm time{};
    time.tm_year = value.year - 1900;
    time.tm_mon = value.month;
    time.tm_mday = value.monthDay;
    time.tm_isdst = value.isDST;
    char name[70] = {};
    const std::size_t length = strftime_nid_postfix(name, sizeof(name), "%Z", &time);
    if (!length) return {};
    return " (" + std::string(name, length) + ")";
}

void TestUnregistered() {
    const GregorianDateTime value = Sample();
    StringImpl* result = nullptr;
    Require(Throws<std::runtime_error>([&] {
        _ZN3JSC14formatDateTimeERKN3WTF17GregorianDateTimeENS_14DateTimeFormatEb(&result, &value, Jsc::RuntimeAbi::DateTimeFormatDate, true);
    }));
    Require(result == nullptr);
}

void TestFormats() {
    GregorianDateTime value = Sample();
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatDate, false) == "Thu Feb 29 2024");
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatDate, true) == "Thu, 29 Feb 2024");
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatTime, true) == "07:05:09 GMT");
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatTime, false) == "07:05:09 GMT-0530" + ZoneSuffix(value));
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatDateAndTime, true) == "Thu, 29 Feb 2024 07:05:09 GMT");
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatDateAndTime, false) == "Thu Feb 29 2024 07:05:09 GMT-0530" + ZoneSuffix(value));
    Require(Format(value, 0, false).empty());
    value.utcOffsetInMinute = 60;
    value.isDST = 1;
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatTime, false) == "07:05:09 GMT+0100" + ZoneSuffix(value));
    value.utcOffsetInMinute = 0;
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatTime, false) == "07:05:09 GMT+0000" + ZoneSuffix(value));
}

void TestWeekdaysAndMonths() {
    const char* weekdays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char* months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    GregorianDateTime value = Sample();
    for (int day = 0; day < 7; ++day) {
        value.weekDay = day;
        Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatDate, true).substr(0, 3) == weekdays[day]);
    }
    value.weekDay = 8;
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatDate, true).substr(0, 3) == "Mon");
    value.weekDay = -6;
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatDate, true).substr(0, 3) == "Mon");
    value.weekDay = 0;
    for (int month = 0; month < 12; ++month) {
        value.month = month;
        Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatDate, false).substr(4, 3) == months[month]);
    }
}

void TestNumberWidths() {
    GregorianDateTime value = Sample();
    value.year = 5;
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatDate, true) == "Thu, 29 Feb 0005");
    value.year = -5;
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatDate, true) == "Thu, 29 Feb -0005");
    value.year = 275760;
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatDate, true) == "Thu, 29 Feb 275760");
    value.year = -271821;
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatDate, true) == "Thu, 29 Feb -271821");
    value.year = INT_MIN;
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatDate, true) == "Thu, 29 Feb --2147483648");
    value = Sample();
    value.monthDay = 1;
    value.hour = 0;
    value.minute = 59;
    value.second = 60;
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatDateAndTime, true) == "Thu, 01 Feb 2024 00:59:60 GMT");
    value.hour = 123;
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatTime, true) == std::string("") + static_cast<char>('0' + 12) + '3' + ":59:60 GMT");
}

void TestInvalidArguments() {
    GregorianDateTime value = Sample();
    StringImpl* result = nullptr;
    errno = 62;
    Require(Throws<std::invalid_argument>([&] {
        _ZN3JSC14formatDateTimeERKN3WTF17GregorianDateTimeENS_14DateTimeFormatEb(nullptr, &value, Jsc::RuntimeAbi::DateTimeFormatDate, true);
    }));
    Require(Throws<std::invalid_argument>([&] {
        _ZN3JSC14formatDateTimeERKN3WTF17GregorianDateTimeENS_14DateTimeFormatEb(&result, nullptr, Jsc::RuntimeAbi::DateTimeFormatDate, true);
    }));
    const auto* unreadable = reinterpret_cast<const GregorianDateTime*>(std::uintptr_t{0x10});
    Require(Throws<std::invalid_argument>([&] {
        _ZN3JSC14formatDateTimeERKN3WTF17GregorianDateTimeENS_14DateTimeFormatEb(&result, unreadable, Jsc::RuntimeAbi::DateTimeFormatDate, true);
    }));
    auto* unwritable = reinterpret_cast<StringImpl**>(std::uintptr_t{0x10});
    const int before = allocations;
    Require(Throws<std::invalid_argument>([&] {
        _ZN3JSC14formatDateTimeERKN3WTF17GregorianDateTimeENS_14DateTimeFormatEb(unwritable, &value, Jsc::RuntimeAbi::DateTimeFormatDate, true);
    }));
    Require(allocations == before);
    Require(errno == 62);
    value.month = 12;
    Require(Throws<std::invalid_argument>([&] {
        _ZN3JSC14formatDateTimeERKN3WTF17GregorianDateTimeENS_14DateTimeFormatEb(&result, &value, Jsc::RuntimeAbi::DateTimeFormatDate, true);
    }));
    value.month = -1;
    Require(Throws<std::invalid_argument>([&] {
        _ZN3JSC14formatDateTimeERKN3WTF17GregorianDateTimeENS_14DateTimeFormatEb(&result, &value, Jsc::RuntimeAbi::DateTimeFormatDate, true);
    }));
    value = Sample();
    value.weekDay = -7;
    Require(Throws<std::invalid_argument>([&] {
        _ZN3JSC14formatDateTimeERKN3WTF17GregorianDateTimeENS_14DateTimeFormatEb(&result, &value, Jsc::RuntimeAbi::DateTimeFormatDate, true);
    }));
    value.weekDay = INT_MAX;
    Require(Throws<std::invalid_argument>([&] {
        _ZN3JSC14formatDateTimeERKN3WTF17GregorianDateTimeENS_14DateTimeFormatEb(&result, &value, Jsc::RuntimeAbi::DateTimeFormatDate, true);
    }));
    value.month = 40;
    Require(Format(value, Jsc::RuntimeAbi::DateTimeFormatTime, true) == "07:05:09 GMT");
    Require(result == nullptr);
}

}

int main() {
    TestUnregistered();
    constexpr PersistenceAllocator allocator{&TestAllocate, &TestDeallocate};
    RegisterPersistenceAllocator_nid_no_patch(&allocator);
    TestFormats();
    TestWeekdaysAndMonths();
    TestNumberWidths();
    TestInvalidArguments();
    Require(allocations == 0);
    std::puts("formatDateTime tests passed");
    return 0;
}
