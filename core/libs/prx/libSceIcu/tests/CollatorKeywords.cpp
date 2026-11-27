#include "prx/libSceIcu/libSceIcu.hpp"
#include "prx/libc/include/ApplicationHeap.hpp"
#include "unicode/utypes.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Require(bool value, const std::string& message) {
    if (!value) throw std::runtime_error("icu collator keywords: " + message);
}

void* APS5_VABI Allocate(std::size_t size) { return std::malloc(size); }
void APS5_VABI Free(void* pointer) { std::free(pointer); }
void* APS5_VABI Calloc(std::size_t count, std::size_t size) { return std::calloc(count, size); }
void* APS5_VABI Reallocate(void* pointer, std::size_t size) { return std::realloc(pointer, size); }
void* APS5_VABI Align(std::size_t alignment, std::size_t size) {
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    void* result = nullptr;
    return posix_memalign(&result, alignment, size) == 0 ? result : nullptr;
#endif
}
void* APS5_VABI Realign(void* pointer, std::size_t size, std::size_t alignment) {
#ifdef _WIN32
    return _aligned_realloc(pointer, size, alignment);
#else
    void* result = Align(alignment, size);
    std::free(pointer);
    return result;
#endif
}
int APS5_VABI PosixAlign(void** pointer, std::size_t alignment, std::size_t size) {
#ifdef _WIN32
    *pointer = _aligned_malloc(size, alignment);
    return *pointer ? 0 : 12;
#else
    return posix_memalign(pointer, alignment, size);
#endif
}

constexpr std::size_t HeaderSize = 32;
constexpr std::uint32_t TableType = 2;
constexpr std::uint32_t TypeShift = 28;

void Put16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value) {
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}

void Put32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}

void PutHeader(std::vector<std::uint8_t>& bytes, std::size_t offset, std::string_view format) {
    Put16(bytes, offset, HeaderSize);
    bytes[offset + 2] = 0xda;
    bytes[offset + 3] = 0x27;
    Put16(bytes, offset + 4, 20);
    bytes[offset + 8] = U_IS_BIG_ENDIAN;
    bytes[offset + 9] = U_CHARSET_FAMILY;
    bytes[offset + 10] = U_SIZEOF_UCHAR;
    std::memcpy(bytes.data() + offset + 12, format.data(), 4);
    bytes[offset + 16] = 1;
}

class Bundle {
public:
    Bundle() : body(4) {}

    std::uint16_t Key(std::string_view key) {
        const auto offset = body.size();
        body.insert(body.end(), key.begin(), key.end());
        body.push_back(0);
        return static_cast<std::uint16_t>(offset);
    }

    std::uint32_t String(std::u16string_view text) {
        Align();
        const auto offset = body.size();
        body.resize(offset + 4 + (text.size() + 1) * 2);
        Put32(body, offset, static_cast<std::uint32_t>(text.size()));
        std::memcpy(body.data() + offset + 4, text.data(), text.size() * 2);
        return static_cast<std::uint32_t>(offset / 4);
    }

    std::uint32_t Table(const std::vector<std::pair<std::uint16_t, std::uint32_t>>& items) {
        if (items.empty()) return TableType << TypeShift;
        Align();
        const auto offset = body.size();
        body.resize(offset + 2 + 2 * items.size());
        Put16(body, offset, static_cast<std::uint16_t>(items.size()));
        for (std::size_t index = 0; index != items.size(); ++index)
            Put16(body, offset + 2 + 2 * index, items[index].first);
        Align();
        for (const auto& item : items) {
            const auto position = body.size();
            body.resize(position + 4);
            Put32(body, position, item.second);
        }
        return (TableType << TypeShift) | static_cast<std::uint32_t>(offset / 4);
    }

    std::vector<std::uint8_t> Finish(std::uint32_t root) {
        Align();
        Put32(body, 0, root);
        std::vector<std::uint8_t> item(HeaderSize);
        PutHeader(item, 0, "ResB");
        item.insert(item.end(), body.begin(), body.end());
        return item;
    }

private:
    void Align() {
        while (body.size() % 4) body.push_back(0);
    }

    std::vector<std::uint8_t> body;
};

std::vector<std::uint8_t> Index() {
    Bundle bundle;
    const auto installed = bundle.Key("InstalledLocales");
    const auto de = bundle.Key("de");
    const auto root = bundle.Key("root");
    const auto locales = bundle.Table({{de, 0}, {root, 0}});
    return bundle.Finish(bundle.Table({{installed, locales}}));
}

std::vector<std::uint8_t> Locale(std::u16string_view defaultName, const std::vector<std::string_view>& keys) {
    Bundle bundle;
    const auto collations = bundle.Key("collations");
    std::vector<std::pair<std::uint16_t, std::uint32_t>> items;
    for (const auto key : keys) items.emplace_back(bundle.Key(key), TableType << TypeShift);
    const auto name = bundle.String(defaultName);
    for (std::size_t index = 0; index != keys.size(); ++index)
        if (keys[index] == "default") items[index].second = name;
    const auto table = bundle.Table(items);
    return bundle.Finish(bundle.Table({{collations, table}}));
}

std::vector<std::uint8_t> Package() {
    const std::string prefix = U_ICUDATA_NAME "/coll/";
    const std::vector<std::pair<std::string, std::vector<std::uint8_t>>> items{
        {prefix + "de.res", Locale(u"phonebook", {"default", "phonebook", "private-unihan", "search"})},
        {prefix + "res_index.res", Index()},
        {prefix + "root.res", Locale(u"standard", {"default", "search", "standard"})}};
    std::vector<std::uint8_t> bytes(HeaderSize);
    PutHeader(bytes, 0, "CmnD");
    const std::size_t toc = HeaderSize;
    std::size_t names = toc + 4 + 8 * items.size();
    bytes.resize(names);
    Put32(bytes, toc, static_cast<std::uint32_t>(items.size()));
    std::vector<std::size_t> nameOffsets;
    for (const auto& item : items) {
        nameOffsets.push_back(bytes.size() - toc);
        bytes.insert(bytes.end(), item.first.begin(), item.first.end());
        bytes.push_back(0);
    }
    for (std::size_t index = 0; index != items.size(); ++index) {
        bytes.resize((bytes.size() + 15) / 16 * 16);
        Put32(bytes, toc + 4 + 8 * index, static_cast<std::uint32_t>(nameOffsets[index]));
        Put32(bytes, toc + 8 + 8 * index, static_cast<std::uint32_t>(bytes.size() - toc));
        bytes.insert(bytes.end(), items[index].second.begin(), items[index].second.end());
    }
    return bytes;
}

std::string Join(const std::vector<std::string>& values) {
    std::string text;
    for (const auto& value : values) text += (text.empty() ? "" : ",") + value;
    return "[" + text + "]";
}

std::vector<std::string> Drain(UEnumeration* enumeration, const std::string& context) {
    Require(enumeration != nullptr, context + " returned no enumeration");
    std::int32_t status = U_ZERO_ERROR;
    const auto count = uenum_count_nid_postfix(enumeration, &status);
    Require(status == U_ZERO_ERROR, context + " count failed with status " + std::to_string(status));
    std::vector<std::string> values;
    std::int32_t length = -1;
    while (const char* value = uenum_next_nid_postfix(enumeration, &length, &status)) {
        Require(length == static_cast<std::int32_t>(std::strlen(value)), context + " returned a wrong length for " + value);
        values.emplace_back(value);
    }
    Require(status == U_ZERO_ERROR, context + " next failed with status " + std::to_string(status));
    Require(count == static_cast<std::int32_t>(values.size()), context + " counted " + std::to_string(count) + " values for " + Join(values));
    uenum_close_nid_postfix(enumeration);
    return values;
}

void CheckValues(const char* locale, std::int32_t expectedStatus, const std::vector<std::string>& expected) {
    const std::string context = std::string("ucol_getKeywordValuesForLocale(") + (locale ? locale : "null") + ")";
    std::int32_t status = U_ZERO_ERROR;
    const auto values = Drain(ucol_getKeywordValuesForLocale_nid_postfix("collation", locale, 1, &status), context);
    Require(status == expectedStatus, context + " set status " + std::to_string(status));
    Require(values == expected, context + " returned " + Join(values) + ", expected " + Join(expected));
}

void CheckKeywordRejected() {
    for (const char* keyword : {static_cast<const char*>(nullptr), "", "collations", "Collation", "calendar"}) {
        std::int32_t status = U_USING_DEFAULT_WARNING;
        Require(ucol_getKeywordValues_nid_postfix(keyword, &status) == nullptr, "ucol_getKeywordValues accepted a keyword other than collation");
        Require(status == U_ILLEGAL_ARGUMENT_ERROR, "ucol_getKeywordValues set status " + std::to_string(status) + " for a keyword other than collation");
    }
    std::int32_t status = U_MEMORY_ALLOCATION_ERROR;
    Require(ucol_getKeywordValues_nid_postfix("collation", &status) == nullptr, "ucol_getKeywordValues ignored a failing status");
    Require(ucol_getKeywordValuesForLocale_nid_postfix("collation", "de", 0, &status) == nullptr, "ucol_getKeywordValuesForLocale ignored a failing status");
    Require(status == U_MEMORY_ALLOCATION_ERROR, "a failing status changed");
}

void CheckWithoutData() {
    CheckKeywordRejected();
    std::int32_t status = U_ZERO_ERROR;
    Require(ucol_getKeywordValues_nid_postfix("collation", &status) == nullptr, "ucol_getKeywordValues returned values without collation data");
    Require(status == U_MISSING_RESOURCE_ERROR, "ucol_getKeywordValues set status " + std::to_string(status) + " without collation data");
    status = U_ZERO_ERROR;
    Require(ucol_getKeywordValuesForLocale_nid_postfix("collation", "de", 1, &status) == nullptr, "ucol_getKeywordValuesForLocale returned values without collation data");
    Require(status == U_MISSING_RESOURCE_ERROR, "ucol_getKeywordValuesForLocale set status " + std::to_string(status) + " without collation data");
}

void CheckWithData() {
    alignas(16) static std::uint8_t storage[4096];
    const auto image = Package();
    Require(image.size() <= sizeof(storage), "the test data does not fit its buffer");
    std::memcpy(storage, image.data(), image.size());
    std::int32_t status = U_ZERO_ERROR;
    udata_setCommonData_nid_postfix(storage, &status);
    Require(status == U_ZERO_ERROR, "udata_setCommonData refused the test data with status " + std::to_string(status));
    uloc_setDefault_nid_postfix("en_US", &status);
    Require(status == U_ZERO_ERROR, "uloc_setDefault failed with status " + std::to_string(status));

    CheckKeywordRejected();
    status = U_ZERO_ERROR;
    const auto all = Drain(ucol_getKeywordValues_nid_postfix("collation", &status), "ucol_getKeywordValues");
    Require(status == U_ZERO_ERROR, "ucol_getKeywordValues set status " + std::to_string(status));
    Require(all == std::vector<std::string>{"phonebook", "search", "standard"}, "ucol_getKeywordValues returned " + Join(all));

    CheckValues("de", U_ZERO_ERROR, {"phonebook", "search", "standard"});
    CheckValues("de_AT", U_USING_FALLBACK_WARNING, {"phonebook", "search", "standard"});
    CheckValues("root", U_ZERO_ERROR, {"standard", "search"});
    CheckValues("fr", U_USING_DEFAULT_WARNING, {"standard", "search"});
    CheckValues(nullptr, U_USING_DEFAULT_WARNING, {"standard", "search"});

    bool thrown = false;
    try {
        status = U_ZERO_ERROR;
        ucol_getKeywordValues_nid_postfix(reinterpret_cast<const char*>(0x10), &status);
    } catch (const std::exception&) {
        thrown = true;
    }
    Require(thrown && status == U_ZERO_ERROR, "ucol_getKeywordValues read an unmapped keyword");
}

}

int main(int argumentCount, char** arguments) {
    Require(argumentCount == 2, "expected one argument: empty or data");
    std::array<void*, 10> api{
        reinterpret_cast<void*>(Allocate), reinterpret_cast<void*>(Free), reinterpret_cast<void*>(Calloc),
        reinterpret_cast<void*>(Reallocate), reinterpret_cast<void*>(Align), reinterpret_cast<void*>(Realign),
        reinterpret_cast<void*>(PosixAlign)};
    ApplicationHeapRegister_nid_no_patch(api.data());
    const std::string_view mode = arguments[1];
    if (mode == "empty") CheckWithoutData();
    else if (mode == "data") CheckWithData();
    else Require(false, "unknown mode " + std::string(mode));
    return 0;
}
