# libSceNKWebKit

## add Functional

- [WebCore::SecurityPolicy::generateReferrerHeader](../../../core/libs/prx/libSceNKWebKit/src/SecurityPolicy.cpp) (libSceNKWebKit) - not compared with a console module. It follows the 3.20-signature [SecurityPolicy.cpp](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/WebCore/page/SecurityPolicy.cpp#L81-L142), whose body the pinned 11.x file keeps. The `same-origin`, `origin-when-cross-origin` and `strict-origin-when-cross-origin` policies throw `NotImplemented_nid_no_patch` for an HTTP(S) referrer, because they need `SecurityOrigin::canRequest`, which is not implemented here. The `ReferrerPolicy` argument is read as its 8-bit enum value; values past `unsafe-url` take the default path, as the source's `switch` does. Closes when: `SecurityOrigin::canRequest` is implemented.
