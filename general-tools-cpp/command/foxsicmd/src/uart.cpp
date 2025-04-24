#include "uart.h"
#include <sstream>

UARTInfo::UARTInfo(std::string port_name, uint32_t baud_rate, uint8_t data_bits, uint8_t stop_bits, uint8_t parity): port_name(port_name), baud_rate(baud_rate), data_bits(data_bits), stop_bits(stop_bits), parity(parity) {};

std::string UARTInfo::to_string() {
    std::stringstream result;
    result << "UARTInfo::";

    result << "\n\t" << "port_name:\t" << port_name;
    result << "\n\t" << "baud_rate:\t" << std::to_string(baud_rate);
    result << "\n\t" << "data_bits:\t" << std::to_string(data_bits);;
    result << "\n\t" << "stop_bits:\t" << std::to_string(stop_bits);
    result << "\n\t" << "parity:\t\t" << std::to_string(parity);
    result << "\n";

    return result.str();
}