# libSceLibcInternal

## remove xdHqQoggdfo

## remove f1ZGLUnQGgo

## add Functional

- [num_put<wchar_t>(size_t)](../../../core/libs/prx/libSceLibcInternal/InventoryExports.cpp) (libSceLibcInternal, NIDs `xdHqQoggdfo` and `f1ZGLUnQGgo`) follow [crt.vc12 `xlocnum`](https://github.com/ganboing/crt.vc12/blob/f1d47126bf13d1c9dccaa6c53b72217fd0474968/src/xlocnum#L1109-L1115): they build a `"C"` `_Locinfo` and run the `num_put<wchar_t>(const _Locinfo&, size_t)` constructor on it, so that constructor's limits apply, including a null facet throwing `std::invalid_argument` where the console faults. The console's bodies were not read
