# libSceJsc

## add Functional

- [JSC::VM::dumpRegExpTrace](../../../core/libs/prx/libSceJsc/src/VmRegExpTrace.cpp) (libSceJsc) - not compared with a console module; the console JavaScriptCore version and build options are unverified. It returns without touching the VM, as the `#else` branch of pinned [VM.cpp](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/JavaScriptCore/runtime/VM.cpp#L1197-L1201) does when `ENABLE_REGEXP_TRACING` is 0, its default in [PlatformEnable.h](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/WTF/wtf/PlatformEnable.h#L728-L731), which `OptionsPlayStation.cmake` does not change. A console built with RegExp tracing would print its trace list here instead. Closes when: compared with a console module.

## remove | `_ZN3JSC2VM15dumpRegExpTraceEv` | `4tfuolBZ-fw` |
