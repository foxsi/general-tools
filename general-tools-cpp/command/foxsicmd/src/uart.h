#ifndef UART_H
#define UART_H

#include <string>

class UARTInfo {
    
    public:
        UARTInfo() = default;
        UARTInfo(std::string port_name, uint32_t baud_rate, uint8_t data_bits, uint8_t stop_bits, uint8_t parity);

        std::string port_name;
        uint32_t baud_rate;
        uint8_t data_bits;
        uint8_t stop_bits;
        uint8_t parity;

        std::string to_string();
};

#endif