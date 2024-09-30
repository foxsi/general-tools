#pragma once
#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>

namespace config {
// power board ADC reply message size
static const size_t REPLY_SIZE = 0x20;

// lookup table for power board switch channel by name
static const std::unordered_map<std::string, uint8_t> token_lookup = {
    {"de",          0x00},
    {"timepix",     0x01},
    {"saas",        0x02},
    {"cdte1",       0x03},
    {"cdte3",       0x04},
    {"cdte2",       0x05},
    {"cdte4",       0x06},
    {"cmos2",       0x07},
    {"cmos1",       0x08},
    {"formatter",   0x09}
};

// commands to setup ADC and requrest data
static const std::vector<uint8_t> setup_adc = {0x04, 0xff, 0x00};
static const std::vector<uint8_t> request_adc = {0x04, 0x20, 0x00};

static const std::vector<std::string> names = {
    // "formatter",
    "de",
    "cdte1",
    "cdte2",
    "cdte3",
    "cdte4",
    "cmos1",
    "cmos2",
    "timepix",
    "saas"
};
// labels for ADC channels on power board
static const std::vector<std::string> adc_ch_names = {
    "28 V",
    "5.5 V",
    "12 V",
    "5 V",
    "Regulators (A)",
    "SAAS 12 V",
    "SAAS 5 V",
    "Timepix 12 V",
    "Timepix 5 V",
    "CdTe DE",
    "CdTe 1",
    "CdTe 2",
    "CdTe 3",
    "CdTe 4",
    "CMOS 1",
    "CMOS 2"
};

// maps system name indices onto ADC voltage channels
static const std::vector<size_t> v_map = {
    1,
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    3, 
    2, 
    3, 
    2, 
    0 
}; // last channel is aggregate regulators
// maps system name indices onto ADC current channels (CMOS 1 and 2 are swapped as-built)
static const std::vector<size_t> i_map = {
    9, 
    10, 
    11, 
    12, 
    13, 
    14, 
    15, 
    8, 
    7, 
    6, 
    5, 
    4
};
}; // namespace config

namespace util {
    std::string get_now_string();
    std::string get_now_millis();
};

#endif
