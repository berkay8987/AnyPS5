# libSceIcu

## add Functional

- [Collation keyword values](../../../core/libs/prx/libSceIcu/CollationKeywords.cpp) (libSceIcu) - what the console's ICU 68.2 answers is not measured. `ucol_getKeywordValues` and `ucol_getKeywordValuesForLocale` follow ICU 78.1 ([ucol_res.cpp](https://github.com/unicode-org/icu/blob/049e0d6a420629ac7db77256987d083a563287b5/icu4c/source/i18n/ucol_res.cpp#L602-L698)) over the bundled resource reader and the `coll` tree under the ICU 78.1 data name. This build ships no `icudt` collation data, so both answer `U_MISSING_RESOURCE_ERROR` unless a guest loads data of that name through `udata_setCommonData`; a title's ICU 68 data does not provide it. The per-locale enumeration is ICU's keyword-list enumeration rather than ICU's `UList` one; the strings, count and order are the same. An unreadable keyword or locale pointer throws where ICU reads it.
