# libSceNKWebKit

## add Functional

- [WebCore::FrameLoaderClient base defaults](../../../core/libs/prx/libSceNKWebKit/src/NoOpExports.cpp) (libSceNKWebKit) - `finishedLoadingIcon` and `notifyPageOfAppBoundBehavior` are empty and `void`, and [`shouldEnableInAppBrowserPrivacyProtections`](../../../core/libs/prx/libSceNKWebKit/src/ConstantExports.cpp) returns `false`, by the inline base-class bodies of [FrameLoaderClient.h](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/WebCore/loader/FrameLoaderClient.h#L367-L386) only; their 11.00 bodies were not checked.
