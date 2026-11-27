# libSceNKWebKit

## remove mxYK3TBPDro

## add Functional

- [WTF::setDataFile(const char*)](../../../core/libs/prx/libSceNKWebKit/src/WtfDataFile.cpp) (libSceNKWebKit, NID `mxYK3TBPDro`) follows [DataLog.cpp](https://github.com/neonmodder123/PS5-WebKit-11.x/blob/80f77e94c63ee7202eec3d4b5b1a021dd89011ba/Source/WTF/wtf/DataLog.cpp#L114-L167) with `PLATFORM(PLAYSTATION)`: it replaces the first `%pid` with `getpid()` within 1024 characters, opens the path with `FilePrintStream::open(path, "w")`, logs the outcome through `WTFLogAlways`, falls back to a borrowed stdout when the path is null or cannot be opened, makes any stream other than stdout unbuffered, flushes the previous data file and installs the new one behind libSceJsc's `LockedPrintStream`. The path is read through a bounded guest copy of at most 65536 bytes and an unreadable path throws `std::invalid_argument`. The console's body was not read
