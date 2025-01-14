#include <array>
#include <cstddef>
#include <cstdint>
#include <list>
#include <optional>
#include <vector>

namespace custom_utils {
    /**
     * @brief amount of bits that will be added after encoding (only amount of bits that will be added)
     * @return false if data_size is too big to be encoded (become bigger)
     */
    std::optional<size_t> amount_of_redundant_bits (size_t data_size);
    bool encode_repair_data (std::list<bool>& data);
    /**
     * @brief using hamming_codes in this implementation
     * @return true when no error (or repaired error), and no global parity error
               (no second error was FOUND - it still can possibly be there)
     * @return false otherwise
     */
    bool decode_repair_data (std::list<bool>& data);


    // ----------------------------------

    std::vector<uint8_t> binary_list_to_byte_array (const std::list<bool>& data);
    std::list<bool>      byte_array_to_binary_list (const std::vector<uint8_t>& data);

    // ----------------------------------

    const std::array<uint32_t, UINT8_MAX + 1>& get_table();
    uint32_t calculate_checksum      (const std::vector<uint8_t>& data);
    uint32_t calculate_checksum_slow (const std::vector<uint8_t>& data);

    // ----------------------------------

    bool encode_package (std::vector<uint8_t>& package);
    bool decode_package (std::vector<uint8_t>& package);

}

namespace c_wrapped_custom_utils {
    extern "C" bool encode_package_c_wrapped (uint8_t* ptr_data, size_t size, uint8_t** result, size_t* result_size);
    extern "C" bool decode_package_c_wrapped (uint8_t* ptr_data, size_t size, uint8_t** result, size_t* result_size);
    extern "C" void free_c_wrapped (uint8_t* ptr_data);
}