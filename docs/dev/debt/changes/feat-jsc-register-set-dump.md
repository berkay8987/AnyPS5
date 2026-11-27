# libSceJsc

## add Functional

- [JSC::RegisterSet::dump](../../../core/libs/prx/libSceJsc/src/RegisterSetDump.cpp) (libSceJsc) - not compared with a console module; the console JavaScriptCore version is unverified. It follows pinned [RegisterSet.cpp](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/JavaScriptCore/jit/RegisterSet.cpp#L345-L354) on the 64-bit `Bitmap<33>` word of [RegisterSet.h](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/JavaScriptCore/jit/RegisterSet.h#L38): bits 0 to 15 print as `%rax` to `%r15` and 16 to 31 as `%xmm0` to `%xmm15`, the non-Windows x86-64 names, in `[` and `]` with `, ` between, through the `begin`, `printf` and `end` calls of `PrintStream::print`. Higher bits are not printed. A null or unreadable set, a null stream or vtable entry and a null stream from `begin` throw where WebKit faults. Closes when: compared with a console module.

## remove | `_ZNK3JSC11RegisterSet4dumpERN3WTF11PrintStreamE` | `WW0x+WVuT58` |
