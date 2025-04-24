#include "line.h"
#include "json.hpp"
#include "uart.h"
#include "util.h"

#include <boost/filesystem.hpp>

#include <exception>
#include <iostream>
#include <fstream>
#include <string>


Line::Line(int argc, char** argv, boost::asio::io_context& context): options("options"), port(context) {
    options.add_options()
        ("help,h",                                                             "output help message")
        ("timepix,t",                                                           "use timepix UART config")
        ("config,c",    boost::program_options::value<std::string>(),       "config file with options")
        ("port,p",      boost::program_options::value<std::string>(),       "serial port")
    ;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(options).run(), vm);
    boost::program_options::notify(vm);
    // long help message (move this elsewhere? TODO:write it)
    std::string help_msg = R"(usage: foxsicmd [options]
    
    Options:
        --help,         -h                  Display help message.
        --config,       -c                  Provide a systems configuration JSON file.
                                            By default, the serial device defined under the
                                            gse system and the serial settings defined under 
                                            the uplink system will be used to try to open a
                                            serial port.
        --port,         -p                  Override the serial device definition in the 
                                            config file.
        --timepix,t                         Use the serial port definition for the timepix
                                            system in the config file instead of gse + uplink.
    )";

    // handle all the options:
    if(vm.count("help")) {
        std::cout << help_msg << "\n";
        exit(0);
    }

    // set up a serial port and commands using config file:
    if(vm.count("config")) {
        std::cout << "loading config from " << vm["config"].as<std::string>() << "\n";
        try {
            std::string path = vm["config"].as<std::string>();
            this->pull_uplink(path, vm.count("timepix"));
        } catch(std::exception& e) {
            std::cerr << "failed to define a UART interface: " << e.what() << "\n";
            exit(1);
        }

        try {
            this->pull_deck(vm["config"].as<std::string>());
        } catch(std::exception& e) {
            std::cerr << "failed to create command deck: " << e.what() << "\n";
            exit(1);
        }
    } else {
        std::cerr << "--config option is required!\n";
        exit(1);
    }

    // if the serial port is explicitly assigned, overwrite the one we found in the file.
    if(vm.count("port")) {
        if (vm["port"].as<std::string>() != "") {
            this->interface.port_name = vm["port"].as<std::string>();
            std::cout << "overwrote config file UART interface to: " << this->interface.to_string() << "\n";
        }
    } else {
        std::cout << "using config file UART interface: " << this->interface.to_string() << "\n";
    }

    this->start_port();
}

void Line::pull_uplink(std::string path, int timepix) {
    this->interface = UARTInfo();

    std::ifstream sys_file;
    sys_file.open(path);
    if(!sys_file.is_open()) {
        std::cerr << "couldn't open settings file.\n";
        exit(1);
    }

    // parse the systems file:
    nlohmann::json systems_data = nlohmann::json::parse(sys_file);
    sys_file.close();
    
    for(auto& this_entry: systems_data.items()) {
        std::string sys_name = this_entry.value()["name"];
        std::string sys_hex_str = this_entry.value()["hex"];
        if (timepix) {
            if (sys_name == "timepix") {
                try {
                    this->interface.port_name = this_entry.value()["uart_interface"]["tty_path"];
                    this->interface.baud_rate = this_entry.value()["uart_interface"]["baud_rate"];
                    this->interface.data_bits = this_entry.value()["uart_interface"]["data_bits"];
                    this->interface.stop_bits = this_entry.value()["uart_interface"]["stop_bits"];
                    this->interface.parity = this_entry.value()["uart_interface"]["parity_bits"];
                    break;
                } catch(std::exception& e) {
                    std::cerr << "failed to find uplink UART configuration in config file.\n";
                    exit(1);
                }
            }
        } else {
            if (sys_name == "gse") {
                try {
                    this->interface.port_name = this_entry.value()["logger_interface"]["uplink_device"];
                } catch(std::exception& e) {
                    std::cerr << "couldn't find uplink device in config file.\n";
                    exit(1);
                }
            }
            if (sys_name == "uplink") {
                try {
                    this->interface.baud_rate = this_entry.value()["uart_interface"]["baud_rate"];
                    this->interface.data_bits = this_entry.value()["uart_interface"]["data_bits"];
                    this->interface.stop_bits = this_entry.value()["uart_interface"]["stop_bits"];
                    this->interface.parity = this_entry.value()["uart_interface"]["parity_bits"];
                } catch(std::exception& e) {
                    std::cerr << "failed to find uplink UART configuration in config file.\n";
                }
            }
        }
    }
}

void Line::pull_deck(std::string path) {
    this->deck = Deck();

    std::ifstream sys_file;

    // try to open the systems file
    sys_file.open(path);
    if(!sys_file.is_open()) {
        std::cerr << "couldn't open settings file.\n";
        exit(1);
    }

    // parse the systems file:
    nlohmann::json systems_data = nlohmann::json::parse(sys_file);
    sys_file.close();

    boost::filesystem::path source(path);

    for(auto& this_entry: systems_data.items()) {
        std::string sys_name = this_entry.value()["name"];
        std::string sys_hex_str = this_entry.value()["hex"];
        uint8_t sys_hex = strtol(sys_hex_str.c_str(), NULL, 16);

        try{
            std::string this_command_path = this_entry.value().at("commands");
            std::string this_command_type = this_entry.value().at("command_type");
            
            boost::filesystem::path cmd_path = boost::filesystem::canonical(source.parent_path() / ".." / this_command_path);
            std::string cmd_fname = cmd_path.string();
            // std::cout << "reading commands from " << cmd_fname << "\n";
            
            std::ifstream cmd_file;
            cmd_file.open(cmd_fname);
            if(!cmd_file.is_open()) {
                std::cerr << "couldn't open command file for " << sys_name << "\n";
                continue;
            }
            
            // save data for this system to the command deck:
            this->deck.systems.push_back(System(sys_hex, sys_name, this_command_type));
            
            nlohmann::json commands_data = nlohmann::json::parse(cmd_file);
            cmd_file.close();

            std::vector<Command> commands;

            // load each command for this system:
            for(auto& cmd_entry: commands_data.items()) {
                std::string cmd_name = cmd_entry.value()["name"];
                std::string cmd_hex_str = cmd_entry.value()["hex"];
                std::string write_value_str = cmd_entry.value()["write_value"];
                std::string order_str = cmd_entry.value()["order"];
                std::string color_str = cmd_entry.value()["color"];

                uint8_t cmd_hex = strtol(cmd_hex_str.c_str(), NULL, 16);
                auto write_value = util::string_to_bytes(write_value_str);
                uint32_t color = strtoul(color_str.c_str(), NULL, 16);
                uint32_t order = strtoul(order_str.c_str(), NULL, 10);
                
                Command this_cmd(cmd_hex, cmd_name, color, order, write_value);

                commands.push_back((this_cmd));
            }

            this->deck.add(sys_hex, commands);

        } catch(std::exception &e) {}
    }

    // std::cout << "loaded systems: " << deck.to_string();
}

void Line::start_port() {
    // open port:
    try {
        this->port.open(this->interface.port_name);
    } catch(std::exception& e) {
        std::cerr << "failed to open serial port at " << this->interface.port_name << ".\n";
        exit(1);
    }
    
    // set options:
    this->port.set_option(boost::asio::serial_port_base::baud_rate(this->interface.baud_rate));
    this->port.set_option(boost::asio::serial_port_base::character_size(this->interface.data_bits));

    switch(this->interface.parity) {
        case 0:
            this->port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
            break;
        case 1:
            this->port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::odd));
            break;
        case 2:
            this->port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::even));
            break;
        default:
            std::cerr << "unhandled UARTInfo::parity_bits value: " << std::to_string(this->interface.parity) << ".\n";
            exit(1);
    }
    switch(this->interface.stop_bits) {
        case 1:
            this->port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
            break;
        case 2:
            this->port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::two));
            break;
        default:
            std::cerr << "unhandled UARTInfo::stop_bits value: " << std::to_string(this->interface.stop_bits) << ".\n";
            exit(1);
    }
}