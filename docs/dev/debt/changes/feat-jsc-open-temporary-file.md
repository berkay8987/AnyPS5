# libSceJsc

## add Functional

- [WTF::FileSystemImpl::openTemporaryFile](../../../core/libs/prx/libSceJsc/src/WtfTemporaryFile.cpp) (libSceJsc) - not compared with a console module; the console WTF version is unverified. It follows pinned [FileSystemPOSIX.cpp](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/WTF/wtf/posix/FileSystemPOSIX.cpp#L319-L341): guest `TMPDIR` or `/tmp`, the lenient UTF-8 prefix and `XXXXXX` in a 1024-byte `PATH_MAX` buffer, libScePosixForWebKit `mkstemp`, and `String::fromUTF8` of the path, null when that fails, as for a trailing lone lead surrogate. The suffix is ignored as in WebKit. Failure gives handle -1 and a null string. The string is a fastMalloc'd `StringImpl`, 8-bit when ASCII. Null or unreadable arguments and missing fastMalloc throw. Closes when: compared with a console module.

## remove | `_ZN3WTF14FileSystemImpl17openTemporaryFileERKNS_6StringERiS3_` | `kMydohzpE+Q` |
