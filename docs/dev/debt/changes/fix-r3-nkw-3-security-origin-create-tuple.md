# libSceNKWebKit

## add Functional

- [WebCore::SecurityOrigin::create from protocol, host and port](../../../core/libs/prx/libSceNKWebKit/src/SecurityOrigin.cpp) (libSceNKWebKit) - not compared with a console module. It parses `protocol://host/` with the URL parser, creates the origin from that URL and sets a non-default port on a tuple origin, as the pinned [SecurityOrigin.cpp](https://github.com/neonmodder123/PS5-WebKit-11.x/blob/80f77e94c63ee7202eec3d4b5b1a021dd89011ba/Source/WebCore/page/SecurityOrigin.cpp#L553-L560) does; the decoded host it computes there is unused, so it is not computed here. The 3.20 `WTF::Optional<unsigned short>` argument is read by address as `{bool engaged; unsigned short value}`, like the other legacy `WTF::Optional` arguments of this library. Closes when: compared with a console module.
