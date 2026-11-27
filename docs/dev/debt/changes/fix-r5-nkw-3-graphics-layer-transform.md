# libSceNKWebKit

## add Functional

- [WebCore::GraphicsLayerTransform constructor](../../../core/libs/prx/libSceNKWebKit/src/GraphicsLayerTransform.cpp) (libSceNKWebKit) - not compared with a console module. It sets anchor point (0.5, 0.5, 0), position and size 0, flattening on, both dirty flags off and four identity matrices, as [GraphicsLayerTransform.cpp](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/WebCore/platform/graphics/GraphicsLayerTransform.cpp#L25-L31) does; the pinned 11.x file is the same. The 544-byte layout (matrices at 32, 160, 288 and 416) follows the member order of its [header](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/WebCore/platform/graphics/GraphicsLayerTransform.h). Its other members are not exported. Closes when: compared with a console module.
