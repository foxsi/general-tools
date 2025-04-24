#ifndef LINE_H
#define LINE_H

#include "uart.h"
#include "commands.h"
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <string>


class Line {
    public:
        Line(int argc, char** argv, boost::asio::io_context& context);
        boost::asio::serial_port port;

        UARTInfo interface;
        Deck deck;

        void start_port();
        void pull_uplink(std::string path, int timepix);
        void pull_deck(std::string path);

    private:
        boost::program_options::options_description options;
        boost::program_options::variables_map vm;

};

#endif