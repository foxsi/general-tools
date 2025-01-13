#pragma once
#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>

namespace config {
// power board ADC reply message size
static const size_t REPLY_SIZE = 36;
// commands to setup ADC and request data
static const std::vector<uint8_t> setup_rtd1 = {0x01, 0xff, 0x00};
static const std::vector<uint8_t> setup_rtd2 = {0x02, 0xff, 0x00};
static const std::vector<uint8_t> convert_rtd1 = {0x01, 0xf0, 0x00};
static const std::vector<uint8_t> convert_rtd2 = {0x02, 0xf0, 0x00};
static const std::vector<uint8_t> read_rtd1 = {0x01, 0xf2, 0x00};
static const std::vector<uint8_t> read_rtd2 = {0x02, 0xf2, 0x00};

static const std::vector<uint8_t> rtd_ids = {0x01, 0x02};
static const uint8_t setup = 0xff;
static const uint8_t convert = 0xf0;
static const uint8_t read = 0xf2;

}; // namespace config
namespace util {
    // get current time as string
    std::string get_now_string();
    // get current time as string, including milliseconds
    std::string get_now_millis();
    // convert four bytes to a uint32_t type
    uint32_t bytes_to_uint32_t(std::vector<uint8_t>& data);
};

#endif
