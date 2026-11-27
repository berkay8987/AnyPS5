#pragma once

#include "prx/libSceJsc/src/WtfStringAbi.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstddef>

namespace Jsc::RuntimeAbi {

struct GregorianDateTime {
    int year;
    int month;
    int yearDay;
    int monthDay;
    int weekDay;
    int hour;
    int minute;
    int second;
    int utcOffsetInMinute;
    int isDST;
};

enum DateTimeFormat : int {
    DateTimeFormatDate = 1,
    DateTimeFormatTime = 2,
    DateTimeFormatDateAndTime = DateTimeFormatDate | DateTimeFormatTime,
};

static_assert(sizeof(GregorianDateTime) == 40);
static_assert(offsetof(GregorianDateTime, weekDay) == 16);
static_assert(offsetof(GregorianDateTime, utcOffsetInMinute) == 32);
static_assert(offsetof(GregorianDateTime, isDST) == 36);

} // namespace Jsc::RuntimeAbi

extern "C" {

Jsc::WtfAbi::StringImpl** APS5_VABI _ZN3JSC14formatDateTimeERKN3WTF17GregorianDateTimeENS_14DateTimeFormatEb(Jsc::WtfAbi::StringImpl** result,
    const Jsc::RuntimeAbi::GregorianDateTime* dateTime, Jsc::RuntimeAbi::DateTimeFormat format, bool asUTCVariant);

}
