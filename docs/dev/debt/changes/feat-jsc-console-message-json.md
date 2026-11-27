# libSceJsc

## add Functional

- [Inspector::ConsoleMessage from JSON log values](../../../core/libs/prx/libSceJsc/src/ConsoleMessage.cpp) (libSceJsc) - not compared with a console module; the console version and the 16-byte `JSONLogValue` layout (32-bit type, then the string) are unverified. The `Vector<JSONLogValue>&&` constructor follows pinned [ConsoleMessage.cpp](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/JavaScriptCore/inspector/ConsoleMessage.cpp#L115-L152): it reserves one entry per input, joins each run of `String` values into one JSON string quoted as `JSON::Value::toJSONString` does, keeps `JSON` values as they are, and takes the first entry as the message. The input vector is left as it is. A non-null global object throws, as its `Strong` handle needs a VM, and so do an unknown value type, null or unreadable arguments and missing fastMalloc. Closes when: compared with a console module.

## remove | `_ZN9Inspector14ConsoleMessageC1EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelEON3WTF6VectorINS5_12JSONLogValueELm0ENS5_15CrashOnOverflowELm16ENS5_10FastMallocEEEPNS1_14JSGlobalObjectEm` | `JzLxHxosAio` |

## remove | `_ZN9Inspector14ConsoleMessageC2EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelEON3WTF6VectorINS5_12JSONLogValueELm0ENS5_15CrashOnOverflowELm16ENS5_10FastMallocEEEPNS1_14JSGlobalObjectEm` | `QcHBTWQf1Fo` |
