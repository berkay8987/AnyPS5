# libSceJsc

## add Functional

- [JSC::ClassInfo::hasStaticSetterOrReadonlyProperties](../../../core/libs/prx/libSceJsc/src/ClassInfo.cpp) (libSceJsc) - not compared with a console module; the console version and layouts are unverified. It follows pinned [Structure.cpp](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/JavaScriptCore/runtime/Structure.cpp#L1384-L1393): it walks the parent chain, true at the first static table with a nonzero flag byte, reading `parentClass` at byte 8 and `staticPropHashTable` at byte 16 of [ClassInfo.h](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/JavaScriptCore/runtime/ClassInfo.h#L192-L205) and the flag at byte 8 of `HashTable` in [Lookup.h](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/JavaScriptCore/runtime/Lookup.h#L107-L114). A null or unreadable class info or table throws where WebKit faults, and a cyclic chain throws where WebKit loops. Closes when: compared with a console module.

## remove | `_ZNK3JSC9ClassInfo35hasStaticSetterOrReadonlyPropertiesEv` | `iGxZRUg9NPM` |
