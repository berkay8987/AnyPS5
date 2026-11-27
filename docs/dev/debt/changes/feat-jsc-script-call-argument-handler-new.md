# libSceJsc

## add Functional

- [Deprecated::ScriptCallArgumentHandler operator new](../../../core/libs/prx/libSceJsc/src/ScriptCallArgumentHandlerNew.cpp) (libSceJsc) - not compared with a console module; the console JavaScriptCore version is unverified. Both `operator new` and `operator new[]` ignore the size and return `0xbadbeef`, as the private overloads of pinned [ScriptFunctionCall.h](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/JavaScriptCore/bindings/ScriptFunctionCall.h#L64-L67) do in a release build, where their `ASSERT_NOT_REACHED` is compiled out. They keep the class on the stack; a caller that uses the pointer faults, as on WebKit. Closes when: compared with a console module.

## remove | `_ZN10Deprecated25ScriptCallArgumentHandlernaEm` | `gsnLR9VxDdc` |

## remove | `_ZN10Deprecated25ScriptCallArgumentHandlernwEm` | `HJshvcn63K4` |
