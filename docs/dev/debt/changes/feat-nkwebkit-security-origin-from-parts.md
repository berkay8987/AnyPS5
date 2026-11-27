# libSceNKWebKit

## add Functional

- [WebCore::SecurityOrigin::create(const String&, const String&, Optional<uint16_t>)](../../../core/libs/prx/libSceNKWebKit/src/SecurityOrigin.cpp) (libSceNKWebKit, NID `3mzq2F6BGBE`) - the console's body was not read, and the `WTF::Optional<uint16_t>` argument is taken by hidden reference with the engaged flag at offset 0 and the value at offset 2, as `LegacyOriginPort` models it. As in [SecurityOrigin.cpp](https://github.com/neonmodder123/PS5-WebKit-11.x/blob/80f77e94c63ee7202eec3d4b5b1a021dd89011ba/Source/WebCore/page/SecurityOrigin.cpp#L553-L560) it parses `protocol + "://" + host + "/"`, builds the origin from that URL, and stores a given port only when it is not the protocol's default; a port given for an opaque origin is dropped, as that tree's release `setPort` does. A null argument throws `std::invalid_argument`
