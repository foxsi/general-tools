#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>

// test reply messages, sent in response to Formatter requests
const std::vector<uint8_t> adc_reply1 = {
    0x09, 0xB8, 0x14, 0xA0, 0x2A, 0x25, 0x3A, 0x33, 0x48, 0xC0, 0x58, 0x8B, 0x68, 0x93, 0x78, 0x94, 
    0x88, 0x95, 0x98, 0x8C, 0xA8, 0x97, 0xB8, 0x8C, 0xC8, 0x99, 0xD8, 0x90, 0xE8, 0xA3, 0xF8, 0x94
};
const std::vector<uint8_t> adc_reply2 = {
    0x09, 0xff, 0x14, 0xff, 0x2A, 0xff, 0x3A, 0xff, 0x48, 0xff, 0x58, 0xff, 0x68, 0xff, 0x78, 0xff, 
    0x88, 0xff, 0x98, 0xff, 0xA8, 0xff, 0xB8, 0xff, 0xC8, 0xff, 0xD8, 0xff, 0xE8, 0xff, 0xF8, 0xff
};

/**
 * @brief A sample TCP server (like the Housekeeping board) for debugging use.
 */
int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "use like this:\n\t> ./debug_server ip.address portnum \n";
        return 1;
    }

    boost::asio::io_context context;
    context.run();
    boost::asio::ip::tcp::acceptor acceptor(context, boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address_v4(argv[1]), strtoul(argv[2], nullptr, 10)));

    while (true) {
        std::cout << "waiting for accept... ";
        boost::asio::ip::tcp::socket sock(context);
        acceptor.accept(sock);
        
        std::cout << "accepted!\n";
        bool flop = true;
        while (sock.is_open()) {
            std::cout << "waiting for message...\n\t";
            std::vector<uint8_t> msg;
            msg.resize(0xff);
            try {
                size_t replylen = sock.read_some(boost::asio::buffer(msg));
                msg.resize(replylen);

                for (auto &c: msg) {
                    std::cout << std::setw(2) << std::setfill('0') << std::hex << (int)c << " ";
                }
                // handle hk requests, reply with correct-length garbage packets:
                bool is_req = true;
                for (size_t i = 0; i < msg.size(); ++i) {
                    if (i > 2) {
                        is_req = false;
                        break; 
                    }
                    if (i == 0) {
                        if (msg[i] != 0x04) {
                            is_req = false;
                            break;
                        }
                    }
                    if (i == 1) {
                        if (msg[i] != 0x20) {
                            is_req = false;
                            break;
                        }
                    }
                }
                if (is_req) {
                    // alternate sending reply1 and reply2
                    if (flop) {
                        sock.send(boost::asio::buffer(adc_reply1));
                    } else {
                        sock.send(boost::asio::buffer(adc_reply2));
                    }
                    flop = !flop;
                }
            } catch (std::exception &e) {
                std::cout << "exchange error: " << e.what() << "\n";
                sock.cancel();
                sock.close();
            }
            std::cout << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    return 0;
}
