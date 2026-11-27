# libSceIcu

## add Functional

- [ICU collator query exports](../../../core/libs/prx/libSceIcu/Collator.cpp) implement only the early returns of ICU 68.2 [`i18n`](https://github.com/unicode-org/icu/tree/84e1f26ea77152936e70d53178a816dbfbf69989/icu4c/source/i18n). A failing `UErrorCode` (above zero) returns with the status untouched; then `ucol_getContractionsAndExpansions` (ucol_sit.cpp 635-658) sets `U_ILLEGAL_ARGUMENT_ERROR` for a null collator, and `ucol_strcollIter` (ucol.cpp 411-434) for a null collator or iterator; it also returns `UCOL_EQUAL` for a null `UErrorCode`. `ucol_getRules` (ucol.cpp 545-558) has no early return: it throws for every input. Every path that needs `UCollator` objects and collation data throws `NotImplemented_nid_no_patch`. Unverified on a console. Closes when: those are implemented and compared with the console.
