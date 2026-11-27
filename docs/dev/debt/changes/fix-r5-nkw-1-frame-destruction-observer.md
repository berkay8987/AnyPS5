# libSceNKWebKit

## add Functional

- [WebCore::FrameDestructionObserver::frameDestroyed](../../../core/libs/prx/libSceNKWebKit/src/FrameDestructionObserver.cpp) (libSceNKWebKit) - not compared with a console module. It clears the observed frame pointer, as [FrameDestructionObserver.cpp](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/WebCore/page/FrameDestructionObserver.cpp#L56-L59) does. The 16-byte layout (vtable pointer at 0, `Frame* m_frame` at 8) follows the class's [header](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/WebCore/page/FrameDestructionObserver.h#L32-L46). The constructor, destructor and `observeFrame` still throw because they register with a `Frame`, which is not modelled. Closes when: compared with a console module.
