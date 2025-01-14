#include "error_repairing.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <list>
#include <optional>
#include <synchapi.h>
#include <vector>


std::optional<size_t> custom_utils::amount_of_redundant_bits (size_t data_size) {
    // FIXME: still not fully safe due to usage of redundant_bits
    if (data_size > UINT64_MAX - 1) return std::nullopt;

    auto redundant_bits = static_cast<size_t>(std::ceil(std::log(data_size + 1)));

    while (std::pow(2, redundant_bits) < static_cast<double>(data_size + 1 + redundant_bits)) {
        // could be optimized - maybe the redundant bits update in cycle always been called once
        // (at least consider maybe for std::log increment) - remove if statement of while cycle
        // redundant_bits = static_cast<size_t>(std::ceil(std::log(data_size + redundant_bits + 1)));
        ++redundant_bits;
    }
    return redundant_bits;
}


bool custom_utils::encode_repair_data (std::list<bool>& data) {
    std::optional<size_t> redundant_amount = amount_of_redundant_bits(std::size(data));
    if (not redundant_amount.has_value()) return false;

    std::vector<std::list<bool>::iterator> ref_parity_bits(redundant_amount.value());
    bool is_global_even = true;
    size_t result = 0;

    // evaluting cycle
    size_t i = 1; // assume zero bit
    size_t power_of_two = 1;
    size_t parity_i = 0;
    for (auto x = data.begin(); x != data.end(); ++x, ++i) {
        if (i == power_of_two) {
            x = data.insert(x, false);
            ref_parity_bits[parity_i++] = x;
            power_of_two <<= 1;
            continue;
        }

        if (not *x) continue;
        is_global_even = not is_global_even;
        result ^= i;
    }

    // adding cycle
    for (auto x : ref_parity_bits) {
        bool parity_value = result & 1;
        *x = parity_value;
        result >>= 1;
        if (parity_value) is_global_even = not is_global_even;
    }
    // add global parity check
    data.push_front(is_global_even);
    return true;
}

bool custom_utils::decode_repair_data (std::list<bool>& data) {
    bool is_global_even = true;
    size_t result = 0;

    // evaluting cycle
    auto x   = data.begin();
    bool expect_global_even = *x;
    x = data.erase(x);

    size_t i            = 1; // assume zero bit
    size_t power_of_two = 1;
    for (; x != data.end(); ++i) {
        if (*x) {
            is_global_even = not is_global_even;
            result ^= i;
        }
       // erase or iterate to next
        if (i == power_of_two) {
            x = data.erase(x);
            power_of_two <<= 1;
        } else {
            ++x;
        }
    }

    // ---------------------
    // result processing
    // ---------------------

    // if found error, but global parity is correct
    if (result != 0 and is_global_even == expect_global_even) {
        return false;
    }

    // remove complition bits - which were added due to adding of error correction bits
    // which don't assure that resulted amount of bits is divisable by 8
    size_t complition_bits_amount = data.size() % 8;
    data.resize(data.size() - complition_bits_amount);

    // no error
    if (result == 0) {
        return true;
    }

    // error found and global parity is incorrect
    x = data.begin();
    power_of_two = 1;
    for (i = 1; i != result; ++i, ++x) { // i = 1 - because we already removed first element
        // skip removed
        while (i == power_of_two) {
            power_of_two <<= 1;
            ++i;
        }
    }
    *x = not *x; // flip bit - error repair

    return true;
}

std::vector<uint8_t> custom_utils::binary_list_to_byte_array (const std::list<bool>& data) {
    std::vector<uint8_t> result;
    result.reserve(size_t(std::ceil(double(data.size()) / 8.0)));

    uint8_t byte = 0;
    short i = 0;
    for (bool bit : data) {
        if (bit) byte |= 1;
        if (++i == 8) {
            result.push_back(byte);
            byte = 0;
            i    = 0;
            continue;
        }
        byte <<= 1;
    }

    // if it wasn't full bytes
    if (i != 0) result.push_back(static_cast<uint8_t>(byte << (7 - i))); // 7 = 8 - 1

    return result;
}

std::list<bool> custom_utils::byte_array_to_binary_list (const std::vector<uint8_t>& data) {
    std::list<bool> result;
    for (uint8_t byte : data) {
        for (short i = 0; i < 8; ++i) {
            result.push_back(static_cast<bool>(byte & 0x80));
            byte <<= 1;
        }
    }
    return result;
}

// ---------------------------------------------------

consteval std::array<uint32_t, UINT8_MAX + 1> make_table () {
    std::array<uint32_t, UINT8_MAX + 1> result;
    const uint32_t C = 0x04C11DB7; // 1 1000 0000 0000 0101 -> 1000 0000 0000 0101 = 04C11DB7

    for (uint16_t control_byte_overflow = 0; control_byte_overflow <= UINT8_MAX; ++control_byte_overflow) {
        auto control_byte = static_cast<uint8_t>(control_byte_overflow); // using overflow cause iterator has to be > than UINT8_MAX
        auto xor_result = static_cast<uint32_t>(control_byte << 24); // 32 - 8 = 24

        for (short bit = 0; bit < 8; ++bit) {
            if ((xor_result & 0x80000000) == 0) {
                xor_result <<= 1;
                continue;
            }
            xor_result = (xor_result << 1) ^ C;
        }

        result[control_byte] = xor_result;
    }

    return result;
}

const std::array<uint32_t, UINT8_MAX + 1>& custom_utils::get_table () {
    static const auto table = make_table();
    return table;
}

// ---------------------------------------------------

uint32_t custom_utils::calculate_checksum_slow (const std::vector<uint8_t>& data) {
    const uint32_t C = 0x04C11DB7;
    uint32_t result = 0xFFFFFFFF;
    bool* a = reinterpret_cast<bool*>(const_cast<unsigned char*>(data.data()));

    for (uint32_t i = 0; i < data.size() * 8; ++i) {
        bool to_xor = (result & 0x80000000) != 0;
        bool x = a[i / 8];
        x = x & (0x80 >> (i % 8));
        result = (result << 1) | x;
        if (to_xor) result ^= C;
    }
    for (uint32_t i = 0; i < 32; ++i) {
        bool to_xor = (result & 0x80000000) != 0;
        result = (result << 1);
        if (to_xor) result ^= C;

    }
    return result;

}

uint32_t custom_utils::calculate_checksum (const std::vector<uint8_t>& data) {
    uint32_t result = 0xFFFFFFFF;
    const auto& table = get_table();
    for (uint8_t byte : data) {
        result = (result << 8) ^ table[byte ^ uint8_t(result >> 24)]; // 24 = 32 - 8, uint32_t >> 24 = uint8_t
    }
    return result;
}

// ---------------------------------------------------

bool custom_utils::encode_package (std::vector<uint8_t>& package) {
    // part I: error check_sum - to check after reparing
    uint32_t checksum = calculate_checksum(package);
    auto* array_checksum = reinterpret_cast<uint8_t*>(&checksum);
    if (INT64_MAX - package.size() < sizeof(checksum)) return false;

    for (auto* ptr = array_checksum + sizeof(checksum) - 1; ptr >= array_checksum; --ptr) {
        package.push_back(*ptr);
    }

    // part II: repair code
    auto binary_package = byte_array_to_binary_list(package);
    if (not encode_repair_data(binary_package)) return false;

    // result
    package = binary_list_to_byte_array(binary_package);
    return true;
}
// 218:  1101 1010
// 110:  0110 1110

bool custom_utils::decode_package (std::vector<uint8_t>& package) {
    // part I: repair code
    auto binary_package = byte_array_to_binary_list(package);
    bool ok = decode_repair_data(binary_package);
    if (not ok) return false;

    package = binary_list_to_byte_array(binary_package); // cause repair codes were removed

    // part II: error check_sum - check after repairing
    uint32_t checksum = calculate_checksum(package);
    if (checksum != 0) return false;

    package.resize(package.size() - sizeof(checksum)); // remove checksum from data

    // result
    return true;
}

// ---------------------------------------------------

bool c_wrapped_custom_utils::encode_package_c_wrapped (uint8_t* ptr_data, size_t size, uint8_t** result, size_t* result_size) {
    std::vector<uint8_t> package(ptr_data, ptr_data + size);

    if (not custom_utils::encode_package(package)) return false;;

    *result = reinterpret_cast<uint8_t*>(malloc(package.size() * sizeof(uint8_t)));
    *result_size = package.size();

    std::ranges::copy(package, *result);
    return true;
}

bool c_wrapped_custom_utils::decode_package_c_wrapped (uint8_t* ptr_data, size_t size, uint8_t** result, size_t* result_size) {
    std::vector<uint8_t> package(ptr_data, ptr_data + size);

    bool function_result = custom_utils::decode_package(package);
    if (not function_result) return false;

    *result      = reinterpret_cast<uint8_t*>(malloc(package.size() * sizeof(uint8_t)));
    *result_size = package.size();

    std::ranges::copy(package, *result);

    return true;
}

void c_wrapped_custom_utils::free_c_wrapped (uint8_t* ptr_data) {
    free(ptr_data);
}