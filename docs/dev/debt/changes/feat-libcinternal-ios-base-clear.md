# libSceLibcInternal

## remove QXJCcrXoqpU

## add Functional

- [ios_base::clear(iostate, bool)](../../../core/libs/prx/libSceLibcInternal/InventoryExports.cpp) (libSceLibcInternal, NID `QXJCcrXoqpU`) follows [crt.vc12 `xiosbase`](https://github.com/ganboing/crt.vc12/blob/f1d47126bf13d1c9dccaa6c53b72217fd0474968/src/xiosbase#L280-L294): it stores the state masked with `_Statmask` (`0x17`) and returns when no stored bit is in the exception mask. When one is, it throws through `NotImplemented_nid_no_patch` instead of rethrowing the current exception (`_Reraise`) or throwing `ios_base::failure` with "ios_base::badbit set", "ios_base::failbit set" or "ios_base::eofbit set", because libc builds no guest `ios_base::failure` with an iostream error code. A null stream throws `std::invalid_argument`. The console's body was not read
