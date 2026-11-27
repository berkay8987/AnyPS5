# libSceLibcInternal

## remove 1LvbNeZZJ-o

## remove CDm+TUClE7E

## remove TEtyeXjcZ0w

## remove yIn4l8OO1zA

## remove 1eVdDzPtzD4

## remove WK24j1F3rCU

## remove Cb1hI+w9nyU

## add Functional

- [numpunct<char> constructors and _Init](../../../core/libs/prx/libSceLibcInternal/InventoryExports.cpp) (libSceLibcInternal, NIDs `1LvbNeZZJ-o`, `CDm+TUClE7E`, `TEtyeXjcZ0w`, `yIn4l8OO1zA`, `1eVdDzPtzD4`, `WK24j1F3rCU`, `Cb1hI+w9nyU`) run libc's bodies for the same NIDs, so the limits of the libc `numpunct<char>` `_Init` and constructor entries apply: the named forms accept only `"C"`, and a null facet throws `std::invalid_argument` where the console faults. Neither `libSceLibcInternal.sprx` body was read, so whether it differs from the `libc.prx` one those entries follow is not known
