# libSceLibcInternal

## remove 1TaQLyPDJEY

## add Functional

- [std::locale::_Setgloballocale](../../../core/libs/prx/libSceLibcInternal/InventoryExports.cpp) (libSceLibcInternal, NID `1TaQLyPDJEY`) stores the pointer that libc's `std::locale::_Getgloballocale` returns, as [microsoft/STL `locale0.cpp`](https://github.com/microsoft/STL/blob/219514876ea86491de191ceaa88632616bbc0f19/stl/src/locale0.cpp#L111-L123) does, without touching reference counts. Unlike that source it registers no exit handler that releases the global locale, and a null pointer throws `std::invalid_argument`, as libc's `std::__sce_v2::locale::_Setgloballocale` does, where the console stores it. The console's body was not read
