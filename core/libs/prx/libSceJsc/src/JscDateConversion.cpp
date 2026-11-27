/*
 * Copyright (C) 2012 Patrick Gansterer <paroga@paroga.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Derived from Source/JavaScriptCore/runtime/DateConversion.cpp, Source/WTF/wtf/DateMath.cpp and
 * Source/WTF/wtf/GregorianDateTime.h@c55364d53f971f63a709976f168d932d558b8909
 */

#include "prx/libSceJsc/include/JscDateConversionExports.hpp"
#include "prx/libSceJsc/include/WtfSha1Exports.hpp"
#include "prx/libc/include/Api.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/GuestTime.hpp"

#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>

namespace {

using Jsc::RuntimeAbi::GregorianDateTime;
using Jsc::WtfAbi::StringImpl;

constexpr const char* WeekdayName[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
constexpr const char* MonthName[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
constexpr std::size_t TimeZoneNameSize = 70;

void AppendTwoDigits(std::string& text, int value) {
    text += static_cast<char>('0' + value / 10);
    text += static_cast<char>('0' + value % 10);
}

void AppendFourDigits(std::string& text, int value) {
    std::string digits;
    if (value == INT_MIN) {
        text += '-';
        digits = std::to_string(value);
    } else {
        if (value < 0) {
            text += '-';
            value = -value;
        }
        digits = std::to_string(value);
    }
    for (int i = 0; i < 4 - static_cast<int>(digits.size()); ++i) text += '0';
    text += digits;
}

std::string TimeZoneName(const GregorianDateTime& dateTime) {
    GuestTm time{};
    time.tm_year = static_cast<int>(static_cast<unsigned>(dateTime.year) - 1900u);
    time.tm_mon = dateTime.month;
    time.tm_yday = dateTime.yearDay;
    time.tm_mday = dateTime.monthDay;
    time.tm_wday = dateTime.weekDay;
    time.tm_hour = dateTime.hour;
    time.tm_min = dateTime.minute;
    time.tm_sec = dateTime.second;
    time.tm_isdst = dateTime.isDST;
    time.tm_gmtoff = static_cast<std::int64_t>(dateTime.utcOffsetInMinute) * 60;
    char name[TimeZoneNameSize] = {};
    const int savedErrno = errno;
    const std::size_t length = strftime_nid_postfix(name, sizeof(name), "%Z", &time);
    errno = savedErrno;
    if (!length) return {};
    return std::string(name, length);
}

std::string Format(const GregorianDateTime& dateTime, int format, bool asUTCVariant) {
    const bool appendDate = format & Jsc::RuntimeAbi::DateTimeFormatDate;
    const bool appendTime = format & Jsc::RuntimeAbi::DateTimeFormatTime;
    std::string text;
    if (appendDate) {
        if (dateTime.weekDay < -6 || dateTime.weekDay > INT_MAX - 6)
            throw std::invalid_argument("JSC::formatDateTime: week day out of range");
        const int weekday = (dateTime.weekDay + 6) % 7;
        if (weekday < 0) throw std::invalid_argument("JSC::formatDateTime: week day out of range");
        if (dateTime.month < 0 || dateTime.month > 11) throw std::invalid_argument("JSC::formatDateTime: month out of range");
        text += WeekdayName[weekday];
        if (asUTCVariant) {
            text += ", ";
            AppendTwoDigits(text, dateTime.monthDay);
            text += ' ';
            text += MonthName[dateTime.month];
        } else {
            text += ' ';
            text += MonthName[dateTime.month];
            text += ' ';
            AppendTwoDigits(text, dateTime.monthDay);
        }
        text += ' ';
        AppendFourDigits(text, dateTime.year);
    }
    if (appendDate && appendTime) text += ' ';
    if (appendTime) {
        AppendTwoDigits(text, dateTime.hour);
        text += ':';
        AppendTwoDigits(text, dateTime.minute);
        text += ':';
        AppendTwoDigits(text, dateTime.second);
        text += " GMT";
        if (!asUTCVariant) {
            const int utcOffset = dateTime.utcOffsetInMinute;
            const int offset = utcOffset == INT_MIN ? INT_MIN : (utcOffset < 0 ? -utcOffset : utcOffset);
            text += utcOffset < 0 ? '-' : '+';
            AppendTwoDigits(text, offset / 60);
            AppendTwoDigits(text, offset % 60);
            const std::string zone = TimeZoneName(dateTime);
            if (!zone.empty()) {
                text += " (";
                text += zone;
                text += ')';
            }
        }
    }
    return text;
}

}

extern "C" {

StringImpl** APS5_VABI _ZN3JSC14formatDateTimeERKN3WTF17GregorianDateTimeENS_14DateTimeFormatEb(StringImpl** result,
    const GregorianDateTime* dateTime, Jsc::RuntimeAbi::DateTimeFormat format, bool asUTCVariant) {
    if (!result) throw std::invalid_argument("JSC::formatDateTime: null return storage");
    if (!dateTime) throw std::invalid_argument("JSC::formatDateTime: null date");
    GregorianDateTime value{};
    int savedErrno = errno;
    const bool read = GuestCopyIn_nid_no_patch(&value, dateTime, sizeof(value));
    errno = savedErrno;
    if (!read) throw std::invalid_argument("JSC::formatDateTime: unreadable date");
    const std::string text = Format(value, format, asUTCVariant);
    constexpr auto charactersOffset = offsetof(StringImpl, hashAndFlags) + sizeof(StringImpl::hashAndFlags);
    const auto* allocator = RegisteredPersistenceAllocator_nid_no_patch();
    if (!allocator || !allocator->allocate || !allocator->deallocate)
        throw std::runtime_error("JSC::formatDateTime: the string requires libSceNKWebKit fastMalloc");
    const auto length = static_cast<unsigned>(text.size());
    std::unique_ptr<void, void (*)(void*)> storage(allocator->allocate(charactersOffset + length), allocator->deallocate);
    if (!storage) throw std::bad_alloc();
    auto* characters = static_cast<unsigned char*>(storage.get()) + charactersOffset;
    auto* string = new (storage.get()) StringImpl{2u, length, characters, Jsc::WtfAbi::StringImplFlag8Bit};
    std::memcpy(characters, text.data(), length);
    savedErrno = errno;
    const bool written = GuestCopyOut_nid_no_patch(result, &string, sizeof(string));
    errno = savedErrno;
    if (!written) throw std::invalid_argument("JSC::formatDateTime: invalid return storage");
    storage.release();
    return result;
}

}
