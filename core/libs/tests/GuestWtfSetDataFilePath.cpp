#include "prx/libSceJsc/include/WtfDataLogExports.hpp"
#include "prx/libSceNKWebKit/include/WtfDataFileExports.hpp"
#include "prx/libc/include/Api.hpp"
#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/FileStream.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/include/Api.hpp"

#include <array>
#include <chrono>
#include <cstdarg>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

using Jsc::WtfAbi::FilePrintStream;
using Jsc::WtfAbi::PrintStream;

using Vprintf = void (APS5_VABI*)(PrintStream*, const char*, VaList*);
using Begin = PrintStream* (APS5_VABI*)(PrintStream*);
using End = void (APS5_VABI*)(PrintStream*);

void* APS5_VABI HeapAllocate(std::size_t bytes) { return std::malloc(bytes == 0 ? 1 : bytes); }
void APS5_VABI HeapFree(void* pointer) { std::free(pointer); }
void* APS5_VABI HeapCalloc(std::size_t count, std::size_t bytes) { return std::calloc(count == 0 ? 1 : count, bytes == 0 ? 1 : bytes); }
void* APS5_VABI HeapReallocate(void* pointer, std::size_t bytes) { return std::realloc(pointer, bytes == 0 ? 1 : bytes); }
void* APS5_VABI HeapAlign(std::size_t, std::size_t) { throw std::logic_error("guest_wtf_set_data_file_path: unexpected aligned allocation"); }
void* APS5_VABI HeapRealign(void*, std::size_t, std::size_t) { throw std::logic_error("guest_wtf_set_data_file_path: unexpected aligned reallocation"); }
int APS5_VABI HeapPosixAlign(void**, std::size_t, std::size_t) { throw std::logic_error("guest_wtf_set_data_file_path: unexpected posix_memalign"); }

void RegisterHeap() {
    std::array<void*, 10> api{};
    api[0] = reinterpret_cast<void*>(&HeapAllocate);
    api[1] = reinterpret_cast<void*>(&HeapFree);
    api[2] = reinterpret_cast<void*>(&HeapCalloc);
    api[3] = reinterpret_cast<void*>(&HeapReallocate);
    api[4] = reinterpret_cast<void*>(&HeapAlign);
    api[5] = reinterpret_cast<void*>(&HeapRealign);
    api[6] = reinterpret_cast<void*>(&HeapPosixAlign);
    ApplicationHeapRegister_nid_no_patch(api.data());
}

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename TFunction>
TFunction Entry(PrintStream* stream, std::size_t slot) {
    Require(stream && stream->vtable && stream->vtable[slot], "missing virtual function");
    return reinterpret_cast<TFunction>(const_cast<void*>(stream->vtable[slot]));
}

void APS5_VABI Print(PrintStream* stream, const char* format, ...) {
#ifdef _WIN32
    __builtin_sysv_va_list args;
    __builtin_sysv_va_start(args, format);
#else
    std::va_list args;
    va_start(args, format);
#endif
    Entry<Vprintf>(stream, 2)(stream, format, reinterpret_cast<VaList*>(args));
#ifdef _WIN32
    __builtin_sysv_va_end(args);
#else
    va_end(args);
#endif
}

FilePrintStream* Target() {
    PrintStream* stream = &_ZN3WTF8dataFileEv();
    Require(stream == WtfDataFile_nid_no_patch(), "dataFile is libSceJsc's stream");
    PrintStream* target = Entry<Begin>(stream, 4)(stream);
    Entry<End>(stream, 5)(stream);
    return reinterpret_cast<FilePrintStream*>(target);
}

std::string ReadAll(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void RunPid(const std::filesystem::path& directory) {
    _ZN3WTF11setDataFileEPKc("/wtf-set-data-file/log-%pid.txt");
    FilePrintStream* file = Target();
    Require(file->file != &_Stdout_nid_postfix && file->adoptionMode == 0, "an opened path is adopted as the data file");
    Print(&_ZN3WTF8dataFileEv(), "value=%d", 7);
    const auto expected = directory / ("log-" + std::to_string(getpid_nid_postfix()) + ".txt");
    Require(std::filesystem::exists(expected), "%pid is replaced by the process id");
    Require(ReadAll(expected) == "value=7", "the data file is unbuffered, so the text is written at once");

    _ZN3WTF11setDataFileEPKc("/wtf-set-data-file/plain.txt");
    Require(Target()->file != file->file, "a second path replaces the data file");
    Print(&_ZN3WTF8dataFileEv(), "second");
    Require(ReadAll(directory / "plain.txt") == "second", "a path without %pid is opened as given");
}

void RunUnopenable() {
    _ZN3WTF11setDataFileEPKc("/wtf-set-data-file/missing/directory/log.txt");
    FilePrintStream* file = Target();
    Require(file->file == &_Stdout_nid_postfix && file->adoptionMode == 1, "a path that cannot be opened falls back to borrowed stdout");
}

void RunNull() {
    _ZN3WTF11setDataFileEPKc(nullptr);
    FilePrintStream* file = Target();
    Require(file->file == &_Stdout_nid_postfix && file->adoptionMode == 1, "a null path selects borrowed stdout");
}

}

int main(int argc, char** argv) {
    RegisterHeap();
    const auto directory = std::filesystem::temp_directory_path()
        / ("anyps5-wtf-set-data-file-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    AddPathAlias_nid_no_patch("/wtf-set-data-file", directory.string().c_str());
    const std::string mode = argc > 1 ? argv[1] : "pid";
    if (mode == "pid") RunPid(directory);
    else if (mode == "unopenable") RunUnopenable();
    else if (mode == "null") RunNull();
    else throw std::invalid_argument("guest_wtf_set_data_file_path: unknown mode");
    return EXIT_SUCCESS;
}
