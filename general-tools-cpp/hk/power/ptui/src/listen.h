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
 * @brief A class to query the Housekeeping board's ADC subsystem.
 * 
 * This will set up a synchronous TCP connection to the Housekeeping board. The class also includes functionality for querying the board's power subsystem for voltage and current data and formatting those responses. Things are set up to return data to the FTXUI visualization which uses this class. 
 */
class HKADCNode {
    public:
        /**
         * @brief Construct a new Node object
         * 
         * @param local the local endpoint to bind to.
         * @param io_context the `boost::asio` `io_context` used to manage communication.
         */
        HKADCNode(boost::asio::ip::tcp::endpoint &local, boost::asio::io_context& io_context);

        // socket should already be bound by the time these are called:

        /**
         * @brief Synchronously read data from remote.
         * 
         * @param data a buffer to store read data in.
         * @return size_t the amount of data (in bytes) read from the socket.
         */
        size_t sync_read(std::vector<uint8_t> &data);

        /**
         * @brief Asynchronously read data from remote.
         * 
         * The read will retry after `HKADCNode::io_timeout` elapses with no response, and do so `HKADCNode::max_retry_count` times. 
         * 
         * Note that failure to read will interrupt the main `io_context`. Take care to manage your contexts if that will disrupt other timers or communication.
         * 
         * @param data a buffer to fill with read data. Should be sized for the amount of data you want to read.
         * @return size_t the amount of data (in bytes) read from the socket.
         */
        size_t async_read(std::vector<uint8_t> &data);
        
        /**
         * @brief Underlying method for `::async_read`. Don't use directly.
         * 
         * @param receive_size amount of data to expect, in bytes.
         * @return std::vector<uint8_t> buffer of data read from socket.
         */
        std::vector<uint8_t> sync_read(size_t receive_size);

        /**
         * @brief Synchronously write `data` to the remote connected socket. 
         * 
         * @param data the data to write.
         */
        void sync_write(std::vector<uint8_t> data);

        /**
         * @brief Synchronously write data to an output file.
         * 
         * @param sink the file descriptor to write to.
         * @param data the data to write.
         */
        void sync_write(std::ofstream &sink, std::vector<uint8_t> data);

        /**
         * @brief Synchronously write data to `::csv_file`. 
         * 
         * This will write the buffer currently stored in `::displayable_reading` (recently obtained Housekeeping board power readings) to the `::csv_file` for storage. A time tag will be added in front with millisecond precision. 
         * 
         */
        void csv_write();

        /**
         * @brief Internal function for ADC responses.
         * 
         * @param err error code for `boost::asio` communication.
         * @param reply_size size of data (in bytes) received.
         */
        void handle_adc_reply(const boost::system::error_code& err, std::size_t reply_size);
        /**
         * @brief Internal function for ADC commands.
         */
        void handle_adc_write();
        /**
         * @brief Method to poll ADC for new power data, save it to CSV, and provide inputs for data display.
         * 
         * This function polls the Housekeeping board for new power readings, stores those readings in the CSV and raw data files, and populates other fields for the FTXUI display to use.
         * 
         * This can be called on a timer in a background thread to continuously update the display or continuously store power readings.
         */
        void poll_adc();

        /**
         * @brief Parse a received 32-byte message from the Housekeeping board into a list of voltage/current values.
         * 
         * @param data the raw 32-byte message received from the power board.
         * @return std::vector<double> parsed current and voltage values, check `config::adc_ch_names` for description of each index.
         */
        std::vector<double> adc_table(std::vector<uint8_t>& data);

        /**
         * @brief The last parsed measurement taken from the Housekeeping board.
         */
        std::vector<double> last_reading;
        /**
         * @brief A formatted version of `::last_reading` for the FTXUI display to ingest.
         */
        std::vector<std::vector<std::string>> displayable_reading;
        /**
         * @brief A formatted version of `::last_reading` for the FTXUI display to ingest.
         */
        std::vector<std::vector<std::string>> format_table;

        /**
         * @brief Convert a pair of bytes in the raw Housekeeping ADC message to `uint16_t`.
         * 
         * @param data the raw Housekeeping ADC message.
         * @param start the start index in the message to convert. `start` and `start+1` will convert, little-endian, to `uint16_t`.
         * @return uint16_t 
         */
        uint16_t adc_range_to_uint16_t(std::vector<uint8_t>& data, size_t start);

        /**
         * @brief An optional message for debugging with the FTXUI interface.
         */
        std::string debug_msg;

        /**
         * @brief Set the up the local socket and connect to `target`.
         * 
         * @param target the remote endpoint to connect to.
         * @return true if the connection was successful.
         * @return false if the connection was not successful.
         */
        bool setup_socket(boost::asio::ip::tcp::endpoint &target);
        
        /**
         * @brief Flag that this is the first entry in the CSV file, so header can be included.
         */
        bool csv_first;
        /**
         * @brief Flag that continuous polling of the Housekeeping ADC has started.
         */
        bool poll_started;
        /**
         * @brief Count the number of readings taken from the Housekeeping board.
         */
        size_t linecounter;

        /**
         * @brief Thread context for communications.
         */
        boost::asio::io_context& context;

        /**
         * @brief TCP endpoint on local machine.
         */
        boost::asio::ip::tcp::endpoint local_endpoint;
        /**
         * @brief TCP socket on local machine.
         */
        boost::asio::ip::tcp::socket socket;

    private:
        /**
         * @brief Internal method for managing read timeouts/retries.
         * 
         * @return true 
         * @return false 
         */
        bool run_context_until();
        /**
         * @brief Internal method for managing read timeouts/retries.
         * 
         * @param ec 
         * @param length 
         * @param out_ec 
         * @param out_length 
         */
        void async_read_handler(const boost::system::error_code &ec, std::size_t length, boost::system::error_code *out_ec, std::size_t *out_length);

        /**
         * @brief Duration to wait for response (in `::async_read`) before trying again or abandoning.
         */
        std::chrono::milliseconds io_timeout;
        /**
         * @brief Number of times (in `::async_read`) to try reading again before abandoning.
         */
        uint8_t max_retry_count;

        std::ofstream raw_file;
        std::ofstream csv_file;

        std::vector<uint8_t> _swap;
};

#endif