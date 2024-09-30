#pragma once
#ifndef LISTEN_H
#define LISTEN_H

#include <boost/asio.hpp>
#include <vector>
#include <map>
#include <fstream>
#include <chrono>
#include <queue>
#include <iostream>
#include <ctime>                // for timestamping
#include "parameters.h"

/**
 * @brief A basic synchronous client socket for pulling power usage data from the housekeeping board.
 */
class HKADCNode {
    public:
        /**
         * @brief Construct a new Node object
         * 
         * @param local the local endpoint to bind to.
         * @param io_context 
         */
        HKADCNode(boost::asio::ip::tcp::endpoint &local, boost::asio::io_context& io_context);

        // socket should already be bound by the time these are called:
        size_t sync_read(std::vector<uint8_t> &data);
        size_t async_read(std::vector<uint8_t> &data);
        std::vector<uint8_t> sync_read(size_t receive_size);
        void sync_write(std::vector<uint8_t> data);
        void sync_write(std::ofstream &sink, std::vector<uint8_t> data);
        void csv_write();

        void handle_adc_reply(const boost::system::error_code& err, std::size_t reply_size);
        void handle_adc_init();
        void handle_adc_write();
        void poll_adc();

        std::vector<double> adc_table(std::vector<uint8_t>& data);
        std::vector<double> last_reading;
        std::vector<std::vector<std::string>> displayable_reading;
        std::vector<std::vector<std::string>> format_table;
        uint16_t adc_range_to_uint16_t(std::vector<uint8_t>& data, size_t start);

        std::string debug_msg;
        bool setup_socket(boost::asio::ip::tcp::endpoint &target);
        
        bool csv_first;
        bool poll_started;
        size_t linecounter;

        boost::asio::io_context& context;

        boost::asio::ip::tcp::endpoint local_endpoint;
        boost::asio::ip::tcp::socket socket;

    private:
        bool run_context_until();
        void async_read_handler(const boost::system::error_code &ec, std::size_t length, boost::system::error_code *out_ec, std::size_t *out_length);

        std::chrono::milliseconds io_timeout;
        uint8_t max_retry_count;

        std::ofstream raw_file;
        std::ofstream csv_file;

        std::vector<uint8_t> _swap;
};

#endif