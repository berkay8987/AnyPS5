# libSceJsc

## add Functional

- [WTF::StackTrace](../../../core/libs/prx/libSceJsc/src/WtfStackTrace.cpp) (libSceJsc) - not compared with a console module; the console WTF version and the 24-byte layout (size and capacity, then the inline or borrowed frames at byte 16) are unverified. It follows pinned [StackTrace.cpp](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/WTF/wtf/StackTrace.cpp#L46-L145) as built for PlayStation, where [PlatformHave.h](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/WTF/wtf/PlatformHave.h#L209-L219) sets neither `HAVE_BACKTRACE` nor `HAVE_DLADDR`: `captureStackTrace` allocates room for at least one frame and records none, `demangle` gives no entry, and `dump` prints `%s%-3d %p` per frame. Null or unreadable arguments and missing fastMalloc throw, and a failed allocation throws `std::bad_alloc` where WebKit crashes. Closes when: compared with a console module.

## remove | `_ZN3WTF10StackTrace17captureStackTraceEii` | `fyBcK2uVX9E` |

## remove | `_ZN3WTF10StackTrace8demangleEPv` | `9U1DoHp0Kcs` |

## remove | `_ZNK3WTF10StackTrace4dumpERNS_11PrintStreamEPKc` | `gfTs2aPeHoM` |
