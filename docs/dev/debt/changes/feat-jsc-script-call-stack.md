# libSceJsc

## add Functional

- [Inspector::ScriptCallStack](../../../core/libs/prx/libSceJsc/src/ScriptCallStack.cpp) (libSceJsc) - not compared with a console module; the console version and layouts (24-byte stack: reference count, then `Vector`; 32-byte frames) are unverified. It follows pinned [ScriptCallStack.cpp](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/JavaScriptCore/inspector/ScriptCallStack.cpp#L42-L87): the constructors and `create` take the frames by swapping, leaving the vector empty, with reference count 1; `create` allocates with guest `operator new`, as `ScriptCallStack` is not fast-allocated; `append` copies the frame and refs its strings, growing the `Vector` through libSceNKWebKit `fastMalloc` as [Vector.h](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/WTF/wtf/Vector.h#L1054-L1057) does. Null or unreadable arguments, a corrupt vector and missing fastMalloc throw. Closes when: compared with a console module.

## remove | `_ZN9Inspector15ScriptCallStack6createERN3WTF6VectorINS_15ScriptCallFrameELm0ENS1_15CrashOnOverflowELm16ENS1_10FastMallocEEE` | `oRiKVtbDDgo` |

## remove | `_ZN9Inspector15ScriptCallStackC1ERN3WTF6VectorINS_15ScriptCallFrameELm0ENS1_15CrashOnOverflowELm16ENS1_10FastMallocEEE` | `kWQbgfGBRqQ` |
