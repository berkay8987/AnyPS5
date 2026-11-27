# libSceJsc

## add Silent stubs

- [JSC debugger run loop](../../../core/libs/prx/libSceJsc/src/DebuggerRunLoop.cpp) (libSceJsc) - `JSGlobalContextSetDebuggerRunLoopWithCurrentRunLoop` and `JSGlobalContextUnsetDebuggerRunLoop` return without effect. In the PS5 11.x [JSContextRefPrivatePlayStation.cpp](https://github.com/neonmodder123/PS5-WebKit-11.x/blob/80f77e94c63ee7202eec3d4b5b1a021dd89011ba/Source/JavaScriptCore/API/playstation/JSContextRefPrivatePlayStation.cpp#L42-L74) a null context returns at once, as here, and otherwise the context's remote-inspector debuggable gets the current thread's `RunLoop` (or none) as the dispatcher for Web Inspector commands. AnyPS5 has no remote inspector or JSC `RunLoop`: libSceJsc's `RemoteInspector` exports throw and contexts live in libSceNKWebKit, so that dispatcher would never be read. Closes when: libSceJsc models the remote inspector.

## remove | `JSGlobalContextSetDebuggerRunLoopWithCurrentRunLoop` | `DojSTghs5jU` |

## remove | `JSGlobalContextUnsetDebuggerRunLoop` | `rWPnHtWTIRg` |
