# libSceNKWebKit

## add Functional

- [WebCore::PerspectiveTransformOperation::dump](../../../core/libs/prx/libSceNKWebKit/src/PerspectiveTransformOperation.cpp) (libSceNKWebKit) - not compared with a console module. It writes `perspective(`, the length and `)` as the 3.20-era [PerspectiveTransformOperation.cpp](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/WebCore/platform/graphics/transforms/PerspectiveTransformOperation.cpp#L72-L75) and [Length.cpp](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/WebCore/platform/Length.cpp#L344-L394) do, numbers formatted respecting integers. A calculated length throws `NotImplemented_nid_no_patch` (needs the calculation-value map), and an operation whose type is not `perspective` throws. Closes when: compared with a console module.
