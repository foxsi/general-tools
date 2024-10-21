#include "listen.h"
#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>

/**
 * @brief A sample TCP client (like the Formatter) for debugging use.
 */
int main(int argc, char** argv) {
    std::cout << argc << "\n";
    if (argc != 5) {
        std::cout << "use like this:\n\t> ./debug_server ip.address portnum remote.ip.address remoteportnum\n";
        return 1;
    }

    boost::asio::io_context context;
    
    boost::asio::ip::tcp::endpoint local_endpoint(boost::asio::ip::make_address_v4(argv[1]), strtoul(argv[2], nullptr, 10));
    boost::asio::ip::tcp::endpoint remote_endpoint(boost::asio::ip::make_address_v4(argv[3]), strtoul(argv[4], nullptr, 10));
    HKADCNode node(local_endpoint, context);
    
    bool connected = node.setup_socket(remote_endpoint);
    node.poll_started = connected;

    context.run();
    while(true) {
        node.poll_adc();
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    return 0;
}
