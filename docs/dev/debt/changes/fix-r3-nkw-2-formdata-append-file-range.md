# libSceNKWebKit

## add Functional

- [WebCore::FormData::appendFileRange](../../../core/libs/prx/libSceNKWebKit/src/FormData.cpp) (libSceNKWebKit) - not compared with a console module. It appends a file element and drops the cached length, as the pinned [FormData.cpp](https://github.com/neonmodder123/PS5-WebKit-11.x/blob/80f77e94c63ee7202eec3d4b5b1a021dd89011ba/Source/WebCore/platform/network/FormData.cpp#L194-L198) does. The 3.20 `WTF::Optional<WTF::WallTime>` argument is read by address as `{bool engaged; double value}`, like the other legacy `WTF::Optional` arguments of this library, and an empty one stores a zero time. Closes when: compared with a console module.
