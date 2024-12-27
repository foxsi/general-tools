#include "parameters.h"
#include "listen.h"
#include <sstream>
#include <boost/bind.hpp>

HKADCNode::HKADCNode(boost::asio::ip::tcp::endpoint &local, boost::asio::io_context &io_context): 
        context(io_context), 
        socket(io_context)
{
    local_endpoint = local;
    socket.open(boost::asio::ip::tcp::v4());
    boost::asio::socket_base::reuse_address reuse_addr_option(true);
    socket.set_option(reuse_addr_option);
    socket.bind(local_endpoint);

    io_timeout = std::chrono::milliseconds(250);
    max_retry_count = 4;
    linecounter = 0;

    // open log files
    raw_file.open("log/raw_" + util::get_now_string() + ".log", std::ios::binary | std::ios::out | std::ios::app);
    csv_file.open("log/parse_" + util::get_now_string() + ".csv", std::ios::out | std::ios::app);
    csv_first = true;
    poll_started = false;
}

bool HKADCNode::setup_socket(boost::asio::ip::tcp::endpoint &target) {
    if (socket.is_open()) {
        try {
            socket.connect(target);
            return true;
        } catch (std::exception &e) {
            std::cout << "connect error: " << e.what() << "\n";
            return false;
        }
    } else {
        socket.open(boost::asio::ip::tcp::v4());
        boost::asio::socket_base::reuse_address reuse_addr_option(true);
        socket.set_option(reuse_addr_option);
        socket.bind(local_endpoint);
        return setup_socket(target);
    }
}

void HKADCNode::sync_write(std::vector<uint8_t> data) {
    try {
        socket.async_send(
            boost::asio::buffer(data),
            boost::bind(
                &HKADCNode::handle_adc_write,
                this
            )
        );
        // socket.send(boost::asio::buffer(data));
    } catch (std::exception& e) {
        std::cout << "send error: " << e.what() << "\n";
    }
}

void HKADCNode::sync_write(std::ofstream &sink, std::vector<uint8_t> data) {
    if (sink.is_open()) {
        char* writable = new char[data.size()];
        std::copy(data.begin(),data.end(),writable);
        sink.write(writable, data.size());
        _swap.resize(0);
    } else {
        std::cout << "file is not open!\n";
    }
}

size_t HKADCNode::sync_read(std::vector<uint8_t> &data) {
    if (data.size() > 0) {
        return socket.receive(boost::asio::buffer(data));
    } else {
        return 0;
    }
}

size_t HKADCNode::async_read(std::vector<uint8_t> &data) {
    size_t retry_count = 0;
    bool did_read = false;
    while (retry_count < max_retry_count && !did_read) {
        std::vector<uint8_t> reply = HKADCNode::sync_read(data.size());
        if (reply.size() == 0) {
            std::cout << "HKADCNode::async_read() attempt "  + std::to_string(retry_count) + " failed.\n";
        } else {
            data = reply;
            return data.size();
        }
        ++retry_count;
    }
    std::cout << "All HKADCNode::async_read() attempts failed!\n";
    return 0;
}

std::vector<uint8_t> HKADCNode::sync_read(size_t receive_size) {
    boost::system::error_code err;
    _swap.resize(receive_size);
    std::vector<uint8_t> receive_data(receive_size);

    boost::asio::async_read(
        socket,
        // boost::asio::buffer(tcp_local_receive_swap),
        boost::asio::buffer(receive_data),
        boost::bind(
            &HKADCNode::async_read_handler,
            this,
            boost::placeholders::_1, 
            boost::placeholders::_2, 
            &err,
            &receive_size
        )
    );
    bool timed_out = HKADCNode::run_context_until();

    if (timed_out) {
        return {};
    } else {
        receive_data.resize(receive_size);
        return receive_data;
    }
}

void HKADCNode::async_read_handler(const boost::system::error_code &ec, std::size_t length, boost::system::error_code *out_ec, std::size_t *out_length) {
    *out_ec = ec;
    *out_length = length;
}

bool HKADCNode::run_context_until() {
    context.restart();
    context.run_for(io_timeout);
    if (!context.stopped()) {
        socket.cancel();
        context.run();
        return true;
    }
    return false;
}

void HKADCNode::poll_adc() {
    if (!poll_started) {
        return;
    }
    socket.send(boost::asio::buffer(config::request_adc));
    std::vector<uint8_t> reply;
    size_t target_size = config::REPLY_SIZE;
    reply.resize(target_size);
    size_t reply_size = socket.receive(boost::asio::buffer(reply));
    
    if (reply_size == target_size) {
        reply.resize(reply_size);
        sync_write(raw_file, reply);
        raw_file.flush();

        ++linecounter;

        last_reading = adc_table(reply);
        displayable_reading.clear();
        format_table.clear();
        format_table.push_back({"System", "Voltage", "Current"});
        for (size_t k = 0; k < 16; ++k) {
            if (k < 12) {
                std::stringstream v_stream;
                v_stream << std::fixed << std::setprecision(3) << last_reading[config::v_map[k]];
                std::stringstream i_stream;
                i_stream << std::fixed << std::setprecision(3) << last_reading[config::i_map[k]];
                format_table.push_back({config::measure_names[k], v_stream.str(), i_stream.str()});
            }
            std::stringstream stream;
            stream << std::fixed << std::setprecision(3) << last_reading[k];
            displayable_reading.push_back({config::adc_ch_names[k], stream.str()});
            debug_msg = config::adc_ch_names[k];
        }
        // call this after setting displayable_reading, to avoid missing last packet before quit.
        csv_write();
    } else {
        std::cout << "got reply size: " << std::to_string(reply_size);
    }
}

void HKADCNode::handle_adc_write() {
    
    size_t target_size = config::REPLY_SIZE;
    _swap.resize(target_size);

    boost::asio::async_read(
        socket, 
        boost::asio::buffer(_swap),
        boost::bind(
            &HKADCNode::handle_adc_reply, 
            this,
            boost::asio::placeholders::error, 
            boost::asio::placeholders::bytes_transferred
        )
    );
}

void HKADCNode::handle_adc_reply(const boost::system::error_code& err, std::size_t reply_size) {
    if (reply_size == config::REPLY_SIZE) {
        _swap.resize(reply_size);
        sync_write(raw_file, _swap);
        raw_file.flush();
    } else {
        std::cout << "got reply size: " << std::to_string(reply_size);
    }
}

void HKADCNode::csv_write() {
    if (displayable_reading.size() != 16) {
        return;
    }

    // linecounter = 0;
    std::string now_line = util::get_now_string();
    std::string header = "";
    
    if (csv_first) {
        header += "Time";
        for (auto& s: displayable_reading) {
            header += "," + s[0];
        }
        header += "\n";
        csv_first = false;
    }
    header += now_line;
    for (auto& s: displayable_reading) {
        header += "," + s[1];
    }
    header += "\n";
    
    char* writable = new char[header.size()];
    std::copy(header.begin(),header.end(),writable);
    csv_file.write(writable, header.size());
    csv_file.flush();
}

std::vector<double> HKADCNode::adc_table(std::vector<uint8_t>& data) {
    if (data.size() != config::REPLY_SIZE) {
        debug_msg = "adc wrong packet size!";
        return {};
    }

    uint16_t raw_5v_src = adc_range_to_uint16_t(data, 6);
    uint16_t ch_5v = raw_5v_src >> 12;
    if (ch_5v != 0x03) {
        // error, read the wrong channel.
        debug_msg = "adc 5v ch error!";
        return {};
    }

    double ref_5v = 5.0;
    double current_gain = 0.2;
    std::vector<double> v_divider_coefficients = {9.2, 2.0, 4.0, 1.68};
    double measured_5v = v_divider_coefficients[3] * ref_5v * (raw_5v_src & 0x0fff) / 0x0fff;

    std::vector<double> result(16);
    for (size_t i = 0; i < config::REPLY_SIZE; i += 2) {
        uint16_t raw = adc_range_to_uint16_t(data, i);
        uint16_t ch = raw >> 12;

        double this_ratiometric = ref_5v * (raw & 0x0fff) / 0x0fff;
        if (ch < 4){
            if (ch == 0x03) {
                // result.insert(std::make_pair(ch_names[ch], measured_5v));
                result[ch] = measured_5v;
            } else {
                // result.insert(std::make_pair(ch_names[ch], v_divider_coefficients[ch] * this_ratiometric));
                result[ch] = v_divider_coefficients[ch] * this_ratiometric;
            }
        } else {
            // result.insert(std::make_pair(ch_names[ch], (this_ratiometric - measured_5v/2) / current_gain));
            result[ch] = (this_ratiometric - measured_5v/2) / current_gain;
        }
    }
    return result;
}

uint16_t HKADCNode::adc_range_to_uint16_t(std::vector<uint8_t>& data, size_t start) {
    uint16_t result = 0;
    for (size_t i = start; i < start + 2; ++i) {
        result <<= 8;
        result |= data[i];
    }
    return result;
}