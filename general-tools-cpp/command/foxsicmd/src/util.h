#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>

namespace util{
    std::vector<uint8_t> string_to_bytes(std::string hex_str);
    std::string bytes_to_string(std::vector<uint8_t> hex);
    std::string byte_to_string(uint8_t hex);
}
#endif