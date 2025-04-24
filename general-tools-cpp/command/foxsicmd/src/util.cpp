#include "util.h"

#include <sstream>
#include <iomanip>

namespace util{
    std::vector<uint8_t> string_to_bytes(std::string hex_str) {
        // will slice 0x or 0X prefix off hex string

        std::vector<uint8_t> bytes;
        // prepend to bytes to make it even-length
        
        if(hex_str.substr(0,2).compare("0x") == 0 || hex_str.substr(0,2).compare("0X") == 0) {
            hex_str.erase(0,2);
        }
        if((hex_str.length() % 2) != 0) {
            hex_str.insert(0, "0");
        }

        for (unsigned int i = 0; i < hex_str.length(); i += 2) {
            std::string byte_str = hex_str.substr(i, 2);
            uint8_t byte = (uint8_t) strtol(byte_str.c_str(), NULL, 16);
            bytes.push_back(byte);
        }
        return bytes;
    }

    std::string bytes_to_string(std::vector<uint8_t> hex) {
        std::stringstream res;

        res << "0x"; 
        for (auto& val: hex) {
            res << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(val);
        }
        return res.str();
    }

    std::string byte_to_string(uint8_t hex) {
        std::stringstream res;
        res << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(hex);
        return res.str();
    }
}