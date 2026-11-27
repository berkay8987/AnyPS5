# libSceNKWebKit

## add Functional

- [WebCore::SQLiteFileSystem::computeHashForFileName](../../../core/libs/prx/libSceNKWebKit/src/SQLiteFileSystemHash.cpp) (libSceNKWebKit, NID `kqK-S56CL9s`) follows [SQLiteFileSystem.cpp](https://github.com/WebKit/WebKit/blob/73a35b43be7ae44c084885bd3fa4b403e59723c8/Source/WebCore/platform/sql/SQLiteFileSystem.cpp#L128-L144): the SHA-256 digest of the name's UTF-8 form, through `PAL::CryptoDigest`, as 64 uppercase hexadecimal digits in an 8-bit `WTF::String`; a null name hashes as the empty string. The console's body was not read
