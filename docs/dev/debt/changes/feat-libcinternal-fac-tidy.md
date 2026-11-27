# libSceLibcInternal

## change `_Fac_tidy`, `_Fail_s`

- [Additional imported names](../../../core/libs/prx/libSceLibcInternal/Export.cpp) (libSceLibcInternal) - `_Fail_s`

## add Functional

- [_Fac_tidy](../../../core/libs/prx/libSceLibcInternal/Export.cpp) (libSceLibcInternal) is taken to be the teardown of the facets `std::locale::facet::_Register` records, which [microsoft/STL `locale0.cpp`](https://github.com/microsoft/STL/blob/219514876ea86491de191ceaa88632616bbc0f19/stl/src/locale0.cpp#L25-L65) runs in the destructor of `_Fac_tidy_reg_t`: it empties the list libc's `_Register` builds and, from the most recently registered facet back, calls each facet's virtual `_Decref` and runs the virtual deleting destructor of the facets it returns, as `~_Fac_node` does. Nothing calls it at exit here, so registered facets are released only when a title calls it. The console's body and its signature were not read; it is declared as `void _Fac_tidy()`
