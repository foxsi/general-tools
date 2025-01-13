#include "parameters.h"
#include "listen.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <boost/bind.hpp>
#include <unordered_map>

HKRTDNode::HKRTDNode(boost::asio::ip::tcp::endpoint &local, boost::asio::io_context &io_context): 
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

bool HKRTDNode::setup_socket(boost::asio::ip::tcp::endpoint &target) {
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

void HKRTDNode::sync_write(std::vector<uint8_t> data) {
    try {
        socket.async_send(
            boost::asio::buffer(data),
            boost::bind(
                &HKRTDNode::handle_adc_write,
                this
            )
        );
        // socket.send(boost::asio::buffer(data));
    } catch (std::exception& e) {
        std::cout << "send error: " << e.what() << "\n";
    }
}

void HKRTDNode::sync_write(std::ofstream &sink, std::vector<uint8_t> data) {
    if (sink.is_open()) {
        char* writable = new char[data.size()];
        std::copy(data.begin(),data.end(),writable);
        sink.write(writable, data.size());
        _swap.resize(0);
    } else {
        std::cout << "file is not open!\n";
    }
}

size_t HKRTDNode::sync_read(std::vector<uint8_t> &data) {
    if (data.size() > 0) {
        return socket.receive(boost::asio::buffer(data));
    } else {
        return 0;
    }
}

size_t HKRTDNode::async_read(std::vector<uint8_t> &data) {
    size_t retry_count = 0;
    bool did_read = false;
    while (retry_count < max_retry_count && !did_read) {
        std::vector<uint8_t> reply = HKRTDNode::sync_read(data.size());
        if (reply.size() == 0) {
            std::cout << "HKRTDNode::async_read() attempt "  + std::to_string(retry_count) + " failed.\n";
        } else {
            data = reply;
            return data.size();
        }
        ++retry_count;
    }
    std::cout << "All HKRTDNode::async_read() attempts failed!\n";
    return 0;
}

std::vector<uint8_t> HKRTDNode::sync_read(size_t receive_size) {
    boost::system::error_code err;
    _swap.resize(receive_size);
    std::vector<uint8_t> receive_data(receive_size);

    boost::asio::async_read(
        socket,
        // boost::asio::buffer(tcp_local_receive_swap),
        boost::asio::buffer(receive_data),
        boost::bind(
            &HKRTDNode::async_read_handler,
            this,
            boost::placeholders::_1, 
            boost::placeholders::_2, 
            &err,
            &receive_size
        )
    );
    bool timed_out = HKRTDNode::run_context_until();

    if (timed_out) {
        return {};
    } else {
        receive_data.resize(receive_size);
        return receive_data;
    }
}

void HKRTDNode::async_read_handler(const boost::system::error_code &ec, std::size_t length, boost::system::error_code *out_ec, std::size_t *out_length) {
    *out_ec = ec;
    *out_length = length;
}

bool HKRTDNode::run_context_until() {
    context.restart();
    context.run_for(io_timeout);
    if (!context.stopped()) {
        socket.cancel();
        context.run();
        return true;
    }
    return false;
}

void HKRTDNode::handle_adc_write() {
    
    size_t target_size = config::REPLY_SIZE;
    _swap.resize(target_size);

    boost::asio::async_read(
        socket, 
        boost::asio::buffer(_swap),
        boost::bind(
            &HKRTDNode::handle_adc_reply, 
            this,
            boost::asio::placeholders::error, 
            boost::asio::placeholders::bytes_transferred
        )
    );
}

void HKRTDNode::handle_adc_reply(const boost::system::error_code& err, std::size_t reply_size) {
    if (reply_size == config::REPLY_SIZE) {
        _swap.resize(reply_size);
        sync_write(raw_file, _swap);
        raw_file.flush();
    } else {
        std::cout << "got reply size: " << std::to_string(reply_size);
    }
}

void HKRTDNode::poll_rtd() {
    if (!poll_started) {
        return;
    }

    if (linecounter == 0) {
        // send setup
        for (uint8_t id: config::rtd_ids) {
            socket.send(boost::asio::buffer({id, config::setup}));
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            socket.send(boost::asio::buffer({id, config::convert}));
        }
        accumulate_error.clear();
    }
    
    // std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    format_table.clear();
    format_table.push_back({"RTD", "fault", "temp ºC", "error rate"});

    for (uint8_t id: config::rtd_ids) {
        // read command:
        socket.send(boost::asio::buffer({id, config::read}));
        std::vector<uint8_t> reply;
        size_t target_size = config::REPLY_SIZE;
        reply.resize(target_size);
        size_t reply_size = socket.receive(boost::asio::buffer(reply));
    
        if (reply_size == target_size) {
            reply.resize(reply_size);

            // start the next conversion:
            socket.send(boost::asio::buffer({id, config::convert}));
            
            sync_write(raw_file, reply);
            raw_file.flush();

            ++linecounter;

            last_data[id] = parse_rtd(reply);
            for (auto row: last_data[id]) {
                std::stringstream temp_val;
                int rtd_num = row.first;
                if (id == 0x01) {
                    rtd_num = 9 - rtd_num;
                } else if  (id == 0x02) {
                    rtd_num = 20 - rtd_num;
                }

                temp_val << std::fixed << std::setprecision(3) << std::to_string(row.second.second);

                if (row.second.first != 1) { // if any error flag is set, add this to the error total for this channel.
                    accumulate_error[id][row.first].first += 1;
                }
                // count this measurement for this channel total
                accumulate_error[id][row.first].second += 1;
                double error_rate_d = static_cast<double>(accumulate_error[id][row.first].first) / accumulate_error[id][row.first].second;
                std::stringstream error_rate;
                error_rate << std::fixed << std::setprecision(3) << std::to_string(error_rate_d);

                format_table.push_back({std::to_string(rtd_num), std::to_string(row.second.first), temp_val.str(), error_rate.str()});
            }
        
            // last_reading = adc_table(reply);
            // displayable_reading.clear();
            
            // for (size_t k = 0; k < 16; ++k) {
            //     if (k < 12) {
            //         std::stringstream v_stream;
            //         format_table.push_back({config::measure_names[k], v_stream.str(), i_stream.str()});
            //     }
            // }
            // // call this after setting displayable_reading, to avoid missing last packet before quit.
            // csv_write();
        } else {
            std::cout << "got reply size: " << std::to_string(reply_size);
        }
    }
}

void HKRTDNode::csv_write() {
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

std::unordered_map<uint8_t, std::pair<uint8_t, double>> HKRTDNode::parse_rtd(std::vector<uint8_t>& data) {

    std::unordered_map<uint8_t, std::pair<uint8_t, double>> result;
    for (uint16_t k = 0; k < data.size(); k += 4) {
        std::vector<uint8_t> this_data(data.begin() + k, data.begin() + k + 4);
        uint8_t flag = this_data[0];
        std::vector<uint8_t> tail(this_data.begin() + 1, this_data.begin() + 4);
        double value = static_cast<double>(util::bytes_to_uint32_t(tail)) / 1024.0;
        uint8_t index = k/4;
        result[index] = std::make_pair(flag, value);
    }
    return result;
}