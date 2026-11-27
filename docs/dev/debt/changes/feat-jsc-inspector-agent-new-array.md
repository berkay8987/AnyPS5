# libSceJsc

## add Functional

- [Inspector agent operator new[]](../../../core/libs/prx/libSceJsc/src/InspectorAgentNew.cpp) (libSceJsc) - not compared with a console module. Both return `fastMalloc(size)`, as `WTF_MAKE_FAST_ALLOCATED` in pinned [InspectorAgent.h](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/JavaScriptCore/inspector/agents/InspectorAgent.h#L45-L47), [InspectorDebuggerAgent.h](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/JavaScriptCore/inspector/agents/InspectorDebuggerAgent.h#L53-L55) and [FastMalloc.h](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/WTF/wtf/FastMalloc.h#L331-L353) defines it. Memory comes from the libSceNKWebKit `fastMalloc`; unregistered it throws, and a failed allocation, where WebKit crashes, throws `std::bad_alloc`. Closes when: compared with a console module.
