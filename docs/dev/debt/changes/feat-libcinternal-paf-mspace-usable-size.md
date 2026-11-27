# libSceLibcInternal

## remove [sceLibcPafMspaceMallocUsableSize]

## add Functional

- [sceLibcPafMspaceMallocUsableSize](../../../core/libs/prx/libSceLibcInternal/InventoryExports6.cpp) (libSceLibcInternal, NID `6JcY5RDA4jY`) is taken to be `size_t (const void* pointer)`, the form of libc's `sceLibcMspaceMallocUsableSize` and of dlmalloc's `mspace_usable_size`, and runs that libc export, as the other PAF mspace exports in libc run their `sceLibcMspace` counterparts: the usable size of a live block in any mspace, 0 for a null pointer, and 0 with `errno` set to `EINVAL` for any other pointer. On a PS5 (firmware 11.60, payload process) the name resolved by `dlsym` in libSceLibcInternal.sprx, but nothing was called, so the signature and the console's behaviour stay unverified
