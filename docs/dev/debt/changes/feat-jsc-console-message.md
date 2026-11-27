# libSceJsc

## add Functional

- [Inspector::ConsoleMessage constructors](../../../core/libs/prx/libSceJsc/src/ConsoleMessage.cpp) (libSceJsc) - not compared with a console module; the console version and the 88-byte layout are unverified. The `(message, requestIdentifier)` and `(message, url, line, column, globalObject, requestIdentifier)` constructors follow pinned [ConsoleMessage.cpp](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/JavaScriptCore/inspector/ConsoleMessage.cpp#L44-L67): they store the enums, ref the strings, set the repeat count to 1, and build the request ID `0.<id>` of [IdentifiersFactory.cpp](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/JavaScriptCore/inspector/IdentifiersFactory.cpp#L45-L50) with fastMalloc, null for 0. A non-null global object, except for `EndGroup`, throws: the call stack it captures needs a VM. Null or unreadable arguments throw. Closes when: libSceJsc can capture a console call stack.

## remove | `_ZN9Inspector14ConsoleMessageC1EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelERKN3WTF6StringES8_jjPNS1_14JSGlobalObjectEm` | `J4h9Iq-fkQ4` |

## remove | `_ZN9Inspector14ConsoleMessageC1EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelERKN3WTF6StringEm` | `bEOdEpgXaoo` |

## remove | `_ZN9Inspector14ConsoleMessageC2EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelERKN3WTF6StringES8_jjPNS1_14JSGlobalObjectEm` | `W7V0pqBz0b4` |

## remove | `_ZN9Inspector14ConsoleMessageC2EN3JSC13MessageSourceENS1_11MessageTypeENS1_12MessageLevelERKN3WTF6StringEm` | `aQKBnWRO6xU` |
