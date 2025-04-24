#include "ftxui/component/captured_mouse.hpp"      // for ftxui
#include "ftxui/component/component.hpp"           // for Radiobox, Renderer
#include "ftxui/component/component_base.hpp"      // for ComponentBase
#include "ftxui/component/screen_interactive.hpp"  // for ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for operator|, Element, size, border, frame, HEIGHT, LESS_THAN
 
#include "line.h"
#include "util.h"
#include <boost/asio.hpp>
#include <chrono>
#include <ftxui/component/component_options.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <string>

// Define a special style for some menu entry.
ftxui::MenuEntryOption Colored(ftxui::Color c) {
    ftxui::MenuEntryOption option;
    option.transform = [c](ftxui::EntryState state) {
    state.label = (state.active ? "> " : "  ") + state.label;
    ftxui::Element e = ftxui::text(state.label) | ftxui::color(c);
    if (state.focused)
        e = e | ftxui::inverted;
    if (state.active)
        e = e | ftxui::bold;
    return e;
    };
    return option;
}

int main(int argc, char** argv) {
    boost::asio::io_context context;
    // digest CLI args and make app objects
    Line lf = Line(argc, argv, context);

    // assemble display names for systems:
    std::vector<std::string> system_names;
    std::vector<uint8_t> system_hexes;
    for (auto& s: lf.deck.systems) {
        // filter only sytems which can be commanded:
        if (lf.deck.lookup_commands(s.hex).size()) {
            system_names.push_back(s.name);
            system_hexes.push_back(s.hex);
        }
    }
    
    // index the system which has been chosen in the UI
    int sys_selected = 0;
    // a bunch of containers for UI data.
    //      for context: to make the command menu change based on the selection in the
    //      system menu in FTXUI, an array of each command (std::vector<std::string>) 
    //      name needs to exist in scope for the UI. 
    std::vector<size_t> sort_index;
    std::vector<ftxui::Component> all_command_menu(system_names.size());
    std::vector<std::vector<ftxui::Component>> all_command_menu_entries(system_names.size());
    std::vector<int> all_command_selection(system_names.size());
    std::vector<std::vector<Command>> all_commands(system_names.size());
    std::vector<std::vector<std::string>> all_command_str(system_names.size());
    std::vector<std::vector<size_t>> all_command_order(system_names.size());
    for (size_t k = 0; k < system_names.size(); ++k) {
        // get all command objects for this system
        auto all_commands_temp = lf.deck.lookup_commands(lf.deck.lookup(system_names[k]).hex);
        sort_index.resize(all_commands_temp.size());
        //      (temp commands used here for sorting, then copied to outer scope command list for UI once sorted).
        //      (makes the indexing easier later on.)

        // get sorting order for these commands:
        all_command_order[k].resize(all_commands_temp.size());
        for (size_t p = 0; p < all_commands_temp.size(); ++p) {
            all_command_order[k][p] = all_commands_temp[p].order;
            sort_index[p] = p;
        }

        // sort the commands by order:
        std::sort(sort_index.begin(), sort_index.end(), 
            [&](const size_t& a, const size_t& b) {
                return (all_command_order[k][a] < all_command_order[k][b]);
            }
        );
        all_commands[k].resize(all_commands_temp.size());
        for (size_t p = 0; p < all_commands_temp.size(); ++p) {
            all_commands[k][p] = all_commands_temp[sort_index[p]];
        }

        // populate the command string values for display
        all_command_str[k].resize(all_commands[k].size());
        all_command_menu_entries[k].resize(all_commands[k].size());
        for (size_t p = 0; p < all_commands[k].size(); ++p) {
            all_command_str[k][p] = all_commands[k][p].name;
            all_command_menu_entries[k][p] = ftxui::MenuEntry(all_commands[k][p].name, Colored(all_commands[k][p].get_color()));
        }

        // store the index for each command
        all_command_selection[k] = 0;
        all_command_menu[k] = ftxui::Container::Vertical(all_command_menu_entries[k], &all_command_selection[k]);
        // all_command_menu[k] = ftxui::Menu(&all_command_str[k], &all_command_selection[k]);
    }

    // track whether user has selected to bypass Formatter
    bool bypass_state = false;
    // UI container for the system selection menu
    auto system_box = ftxui::Menu(&system_names, &sys_selected);
    
    // a string to display debug info on the UI
    std::string debug_note;
    std::string debug_hex;
    std::string selection_note;
    auto put_debug_str = [&]() {
        return debug_note;
    };
    auto put_debug_hex = [&]() {
        return debug_hex;
    };
    auto put_selection_note = [&]() {
        auto sys_name = system_names[sys_selected];
        auto cmd = all_commands[sys_selected][all_command_selection[sys_selected]];
        return sys_name + " > " + cmd.name;
    };

    // lambda to print last transmitted command
    std::string last_transmit = "";
    auto last_send = [&]() {

    };
    
    // lambda to call when the "send" button is pressed
    auto send = [&]() {
        // get system and command selection from UI
        auto sys_name = system_names[sys_selected];
        auto sys_hex = system_hexes[sys_selected];
        auto cmd = all_commands[sys_selected][all_command_selection[sys_selected]];
        
        debug_note = sys_name + " > " + cmd.name;
        debug_hex = util::bytes_to_string({sys_hex, cmd.hex});

        if (bypass_state) { // bypass Formatter, and attempt to send onboard system raw command
            // note: this fails immediately because system_names is not yet populated.
            if (sys_name == "timepix") {
                // send to timepix based on write_value
                std::vector<uint8_t> write_value = cmd.write_value;
                // debug_note = "sending " + util::byte_to_string(write_value[0]);
                lf.port.write_some(boost::asio::buffer(write_value));

                return;
            } else {
                debug_note = "can only bypass formatter for Timepix commands!";
                debug_hex = "";
            }
        } else { // send command as if GSE to Formatter:
            std::vector<uint8_t> write_value = {sys_hex, cmd.hex};
            // debug_note = "sending " + util::bytes_to_string(write_value);
            lf.port.write_some(boost::asio::buffer(write_value));
        }
    };

    // UI layout for the bypass button and send button
    auto send_container = ftxui::Container::Vertical({
        ftxui::Checkbox("Bypass Formatter", &bypass_state, ftxui::CheckboxOption::Simple()),
        ftxui::Button("Send", send, ftxui::ButtonOption::Simple())
    });

    // UI layout for the system and command selection menus
    auto tab_container = ftxui::Container::Tab(
        all_command_menu,
        &sys_selected
    );
   
    // UI layout for all sublayouts
    auto container = ftxui::Container::Horizontal({
        system_box,
        tab_container,
        send_container
    });

    auto renderer = ftxui::Renderer(container, [&] {
        return ftxui::vbox({
            ftxui::separator(),
            ftxui::text("selection: " + put_selection_note()),
            // ftxui::separator(),
            ftxui::hbox({
                system_box->Render() | ftxui::vscroll_indicator | ftxui::frame |
                size(ftxui::HEIGHT, ftxui::LESS_THAN, 16) | ftxui::border,
                tab_container->Render() | ftxui::vscroll_indicator | ftxui::frame |
                size(ftxui::HEIGHT, ftxui::LESS_THAN, 16) | size(ftxui::WIDTH, ftxui::GREATER_THAN, 52) | ftxui::border,
                send_container->Render() | ftxui::frame | ftxui::border
            }),
            ftxui::text("previous command: " + put_debug_str()),
            ftxui::text("                  " + put_debug_hex()),
            ftxui::separator()
        });
    });

    auto screen = ftxui::ScreenInteractive::TerminalOutput();

    // render the screen using the explicit thread/loop.
    //      this is used *in case* blocking port read operations are ever used 
    //      in this tool. In that case, the UI update thread can continue while
    //      the port/socket polling thread is blocked.
    std::atomic<bool> refresh_ui_continue = true;
    std::thread refresh_ui([&] {
        while (refresh_ui_continue) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(20ms);
            // `screen.Post(task)` will execute the update on the thread
            //  where |screen| lives (e.g. the main thread). Using 
            // `screen.Post(task)` is threadsafe.

            screen.Post([&] {  });

            // After updating the state, request a new frame to be drawn. This is done
            // by simulating a new "custom" event to be handled.
            screen.Post(ftxui::Event::Custom);
        }
    });

    screen.Loop(renderer);
    refresh_ui_continue = false;
    refresh_ui.join();

    return 0;
}