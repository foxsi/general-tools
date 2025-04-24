#include "parameters.h"
#include <chrono>
#include <ctime>
#include <sys/time.h>

std::string util::get_now_string() {
    char time_format[std::size("yyyy-mm-dd_hh-mm-ss")];
    auto start_time = std::time({});

    std::strftime(std::data(time_format), std::size(time_format), "%F_%H-%M-%S", std::gmtime(&start_time));

    return std::string(time_format) + "-" + get_now_millis();
}
std::string util::get_now_millis() {
    int millisec;
    struct tm* tm_info;
    struct timeval tv;

    gettimeofday(&tv, NULL);

    millisec = lrint(tv.tv_usec/1000.0); // Round to nearest millisec
    if (millisec>=1000) { // Allow for rounding up to nearest second
        millisec -=1000;
        tv.tv_sec++;
    }

    return std::to_string(millisec);
}

uint32_t util::bytes_to_uint32_t(std::vector<uint8_t>& data) {
    uint32_t result = 0;
    // for (size_t k = 0; k < data.size(); ++k) {
    //     result <<= 8*k;
    //     result |= data[k];

    //     if (k > 3) {
    //         break;
    //     }
    // }
    memcpy(&result, data.data(), 4);

    return result;
}