#include <ftxui/dom/elements.hpp>                   // graph
#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"       // for Input, Renderer, Vertical
#include "ftxui/component/component_base.hpp"  // for ComponentBase
#include "ftxui/component/screen_interactive.hpp"  // for Component, ScreenInteractive
#include <ftxui/screen/color.hpp>
#include <ftxui/dom/table.hpp>

#include <atomic>
#include <thread>
#include <boost/asio.hpp>
#include <iostream>
#include <algorithm>
#include <functional>
#include "src/listen.h"

int main(int argc, char* argv[]) {
    // handle CLI arguments: 
    if (argc < 3) {
        std::cout << "use like this:\n\t> ./gsetui ip.address portnum \n";
        return 1;
    }
    // create io context manager and local TCP endpoint from CLI arguments
    boost::asio::io_context context;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address_v4(argv[1]), strtoul(argv[2], nullptr, 10));
    HKADCNode node(endpoint, context);

    // number of systems used
    const std::size_t n_sys = 9;
    // track on/off state of each system
    std::array<bool, n_sys> states;
    
    auto system_list = ftxui::Container::Vertical({});
    
    // some mutable global strings to display
    std::string raw_address;
    std::string raw_port;
    std::string debug_info;

    std::string connect_label = "Connect...";
    bool connected = false;
    int selected = 0;
    states[selected] = true;

    auto assemble_endpoint = [&] {
        auto addr = boost::asio::ip::make_address(raw_address);
        auto port = strtoul(raw_port.c_str(), nullptr, 10);
        return boost::asio::ip::tcp::endpoint(addr, port);
    };

    // callback when user presses "connect" button
    auto on_connect = [&] {
        try{
            // try to connect to power board
            if (!connected) {
                // make the power board endpoint out of args user has provided in IP and port fields
                auto ept = assemble_endpoint();
                connected = node.setup_socket(ept);
                node.poll_started = connected;
            } else {
                node.socket.cancel();
                node.socket.close();
                node.poll_started = false;
                connected = false;
            }

        } catch(std::exception& e) {
            node.socket.close();
            connected = false;
            node.poll_started = false;
        }

        if (connected) {
            connect_label = "Disconnect";
        } else {
            connect_label = "Connect...";
        }
            
        return connected;
    };

    // callback for user pressing "on" button
    auto on_on = [&] {
        if (connected) {
            // build transmit message (to power board) from the power board's channel ID specific to the currently selected system
            std::vector<uint8_t> out = {0x03, config::token_lookup.at(config::names[selected]), 0x00};
            try{
                node.sync_write(out);
            } catch (std::exception &e) {
                // close the socket if writing command failed
                connected = false;
                node.socket.close();
            }
        }
        return true;
    };
    
    // callback for user pressing "off" button
    auto on_off = [&] {
        if (connected) {
            // build transmit message (to power board) from the power board's channel ID specific to the currently selected system
            std::vector<uint8_t> out = {0x03, config::token_lookup.at(config::names[selected]), 0x01};
            try{
                node.sync_write(out);
            } catch (std::exception &e) {
                // close the socket if writing command failed
                connected = false;
                node.socket.close();
            }
        }
        return true;
    };

    // callback for user selecting a system to control
    auto on_system_select = [&] {
        ftxui::Elements response_field;
        response_field.push_back(ftxui::text(config::names[selected]) | ftxui::bold);
        // std::cout << std::to_string(selected) << "\n";
        return response_field;
    };
    
    // major ui elements:
    auto address_field = ftxui::Input(&raw_address, "address", ftxui::InputOption::Default());
    auto port_field = ftxui::Input(&raw_port, "port", ftxui::InputOption::Default());
    auto connect_button = ftxui::Button(&connect_label, on_connect, ftxui::ButtonOption::Ascii());
    auto ip_layout = ftxui::Container::Horizontal({address_field, port_field, connect_button});
    auto on_button = ftxui::Button("ON ", on_on, ftxui::ButtonOption::Simple());
    auto off_button = ftxui::Button("OFF", on_off, ftxui::ButtonOption::Simple());
    auto radio_list = ftxui::Radiobox(&config::names, &selected);
    system_list->Add(radio_list);   // add all systems to the radio button list
    
    auto onoff_layout = ftxui::Container::Horizontal({on_button, off_button});
    
    // render the ip address/port number entry fields
    auto ip_entry = ftxui::Renderer(ip_layout, [&] {
        return ftxui::vbox({
            ftxui::text("remote endpoint:"),
            ip_layout->Render() | ftxui::frame | size(ftxui::WIDTH, ftxui::GREATER_THAN, 32)
        });
    });
    // render the system selection radiobutton group
    auto system_selector = ftxui::Renderer(system_list, [&] {
        return ftxui::vbox({
                    ftxui::text("systems"),
                    ftxui::separator(), 
                    ftxui::text(" "),
                    ftxui::text(" "),
                    system_list->Render()}) | ftxui::vscroll_indicator | ftxui::frame | size(ftxui::HEIGHT, ftxui::LESS_THAN, 20) | ftxui::border;
    });
    // get the current connected status label
    auto status_label = [&] {
        if (connected) {
            return "connected";
        } else {
            return "unconnected";
        }
    };
    // get the current polling status label
    auto poll_label = [&] {
        std::string label = "";
        if (node.poll_started) {
            return label + "polling " + std::to_string(node.linecounter);
        } else {
            return label + "waiting";
        }
    };
    // get the current debug message set by Node::debug_msg
    auto debug_label = [&] {
        return node.debug_msg;
    };

    auto detail_readout_table = [&] {
        std::vector<std::vector<std::string>> result = node.format_table;
        auto tab = ftxui::Table(result);
        if (node.format_table.size() > 0) {
            std::vector<ftxui::Color::Palette256> colortab = {ftxui::Color::Blue1, ftxui::Color::Orange1, ftxui::Color::Purple, ftxui::Color::Red1};
            for (size_t k = 1; k <= config::v_map.size(); ++k) {
                tab.SelectCell(1, k).Decorate(ftxui::color(colortab[config::v_map[k - 1]]));
            }
            tab.SelectColumn(1).BorderRight(ftxui::LIGHT);
            tab.SelectColumn(1).BorderLeft(ftxui::LIGHT);
            tab.SelectRow(0).Decorate(ftxui::bold);
        }
        return tab.Render();
    };
    auto toggle_context = ftxui::Renderer(onoff_layout, [&] {
        return ftxui::vbox({
            ftxui::text(status_label()) | ftxui::blink | ftxui::center, 
            // ftxui::separator(),
            // ftxui::text(poll_label()) | ftxui::blink | ftxui::center, 
            ftxui::separator(),
            ftxui::hflow({
                on_system_select()
            }) | ftxui::flex_grow | ftxui::center,
            ftxui::separator(),
            onoff_layout->Render(),
        }) | ftxui::border;
    });

    auto table_context = ftxui::Renderer([&] {
        return ftxui::vbox({
            // ftxui::text(debug_label()),
            ftxui::text("measurement"),
            ftxui::separator(),
            ftxui::vbox({
                detail_readout_table()
            }) | size(ftxui::HEIGHT, ftxui::GREATER_THAN, 17) | size(ftxui::WIDTH, ftxui::GREATER_THAN, 20)
        }) | ftxui::border;
    });
    
    // layout out the main areas of the screen:
    auto system_context = ftxui::Container::Horizontal({system_selector, table_context, toggle_context});
    auto global_layout = ftxui::Container::Vertical({ip_entry, system_context});
    auto screen = ftxui::ScreenInteractive::FitComponent();
    
    std::cout << "\n";

    // add work to rendering loop to poll the power board.
    std::atomic<bool> refresh_ui_continue = true;
    std::thread refresh_ui([&] {
        while (refresh_ui_continue) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(100ms);
            // `screen.Post(task)` will execute the update on the thread
            //  where |screen| lives (e.g. the main thread). Using 
            // `screen.Post(task)` is threadsafe.
            screen.Post([&] { node.poll_adc(); });

            // After updating the state, request a new frame to be drawn. This is done
            // by simulating a new "custom" event to be handled.
            screen.Post(ftxui::Event::Custom);
        }
    });

    screen.Loop(global_layout);
    refresh_ui_continue = false;
    refresh_ui.join();

    return 0;
}