# libSceJsc

## add Functional

- [JSC::formatDateTime](../../../core/libs/prx/libSceJsc/src/JscDateConversion.cpp) (libSceJsc) - not compared with a console module; the console version and the 40-byte `GregorianDateTime` layout are unverified. It follows pinned [DateConversion.cpp](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/JavaScriptCore/runtime/DateConversion.cpp#L39-L117): two-digit fields are `'0' + value / 10` and `'0' + value % 10` unchecked, the year is padded to four digits, and the zone name comes from guest libc `strftime("%Z")` on the `tm` of [GregorianDateTime.h](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/WTF/wtf/GregorianDateTime.h#L42-L62). The result is one fastMalloc'd 8-bit `StringImpl`; a format with neither bit gives an allocated empty string, not WebKit's static one. A week day or month outside the name tables, null or unreadable arguments and missing fastMalloc throw. Closes when: compared with a console module.

## remove | `_ZN3JSC14formatDateTimeERKN3WTF17GregorianDateTimeENS_14DateTimeFormatEb` | `r7U2ThCt1ag` |
