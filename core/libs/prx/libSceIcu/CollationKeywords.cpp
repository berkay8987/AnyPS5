// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
*******************************************************************************
*   Copyright (C) 1996-2016, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*/
// https://github.com/unicode-org/icu/blob/049e0d6a420629ac7db77256987d083a563287b5/icu4c/source/i18n/ucol_res.cpp

#include "prx/libSceIcu/DataName.hpp"
#include "prx/libSceIcu/HostEnumeration.hpp"
#include "prx/libc/include/General.hpp"
#include "charstr.h"
#include "resource.h"
#include "unicode/localpointer.h"
#include "unicode/ures.h"
#include "ulocimp.h"
#include "uresimp.h"
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr char CollationTree[] = U_ICUDATA_NAME U_TREE_SEPARATOR_STRING "coll";
constexpr char CollationsKey[] = "collations";

bool CollationKeyword(const char* keyword) {
    constexpr char expected[] = "collation";
    for (std::size_t index = 0; index < sizeof(expected); ++index) {
        char unit = 0;
        const int savedErrno = errno;
        const bool copied = GuestCopyIn_nid_no_patch(&unit, keyword + index, sizeof(unit));
        errno = savedErrno;
        if (!copied) throw std::invalid_argument("ucol_getKeywordValues: unreadable keyword");
        if (unit != expected[index]) return false;
    }
    return true;
}

struct KeywordsSink : public icu::ResourceSink {
    std::vector<std::string> values;
    bool hasDefault = false;

    void put(const char* key, icu::ResourceValue& value, UBool, UErrorCode& errorCode) override {
        if (U_FAILURE(errorCode)) return;
        icu::ResourceTable collations = value.getTable(errorCode);
        for (std::int32_t index = 0; collations.getKeyAndValue(index, key, value); ++index) {
            const UResType type = value.getType();
            if (type == URES_STRING) {
                if (!hasDefault && std::strcmp(key, "default") == 0) {
                    icu::CharString name;
                    name.appendInvariantChars(value.getUnicodeString(errorCode), errorCode);
                    if (U_SUCCESS(errorCode) && !name.isEmpty()) {
                        const std::string defaultName(name.data(), static_cast<std::size_t>(name.length()));
                        const auto found = std::find(values.begin(), values.end(), defaultName);
                        if (found != values.end()) values.erase(found);
                        values.insert(values.begin(), defaultName);
                        hasDefault = true;
                    }
                }
            } else if (type == URES_TABLE && std::strncmp(key, "private-", 8) != 0) {
                if (std::find(values.begin(), values.end(), key) == values.end()) values.emplace_back(key);
            }
            if (U_FAILURE(errorCode)) return;
        }
    }
};

UEnumeration* OpenValues(const std::vector<std::string>& values, UErrorCode* status) {
    std::string list;
    for (const auto& value : values) {
        list += value;
        list += '\0';
    }
    return uloc_openKeywordList(list.data(), static_cast<std::int32_t>(list.size()), status);
}

}

extern "C" {

UEnumeration* APS5_VABI ucol_getKeywordValues_nid_postfix(const char* keyword, std::int32_t* status) {
    return Icu::OpenHostEnumeration(
        status,
        [keyword](UErrorCode* local) -> UEnumeration* {
            if (!keyword || !CollationKeyword(keyword)) {
                *local = U_ILLEGAL_ARGUMENT_ERROR;
                return nullptr;
            }
            return ures_getKeywordValues(CollationTree, CollationsKey, local);
        },
        __func__);
}

UEnumeration* APS5_VABI ucol_getKeywordValuesForLocale_nid_postfix(const char*, const char* locale, std::int8_t, std::int32_t* status) {
    return Icu::OpenHostEnumeration(
        status,
        [locale](UErrorCode* local) -> UEnumeration* {
            std::string localeCopy;
            const char* hostLocale = Icu::CopyDataName(locale, localeCopy, *local, "ucol_getKeywordValuesForLocale");
            if (U_FAILURE(*local)) return nullptr;
            const icu::LocalUResourceBundlePointer bundle(ures_open(CollationTree, hostLocale, local));
            KeywordsSink sink;
            ures_getAllItemsWithFallback(bundle.getAlias(), CollationsKey, sink, *local);
            if (U_FAILURE(*local)) return nullptr;
            return OpenValues(sink.values, local);
        },
        __func__);
}

}
