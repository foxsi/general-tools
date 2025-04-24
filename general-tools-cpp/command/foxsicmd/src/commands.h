#ifndef COMMANDS_H
#define COMMANDS_H

#include <unordered_map>
#include <string>
#include <vector>
#include "ftxui/screen/color.hpp"

class TreeItem{
    public:
        uint8_t hex;
        std::string name;
        uint32_t color;
        uint32_t order;

        std::unordered_map<uint8_t, std::string> lookup_name;
        std::unordered_map<std::string, uint8_t> lookup_hex;

        TreeItem(uint8_t hex, std::string name);
        TreeItem(uint8_t hex, std::string name, uint32_t color, uint32_t order);
        TreeItem() {};

        ftxui::Color get_color();
        std::string to_string();
};

class Command: public TreeItem {
    public:
        Command(uint8_t hex, std::string name) : TreeItem(hex, name) {};
        Command(uint8_t hex, std::string name, uint32_t color, uint32_t order) : TreeItem(hex, name, color, order) {};
        Command(uint8_t hex, std::string name, uint32_t color, uint32_t order, std::vector<uint8_t> write_value) : TreeItem(hex, name, color, order) {this->write_value = write_value;};
        Command() {};

        std::vector<uint8_t> write_value;
};

class System: public TreeItem {
    public: 
        System(uint8_t hex, std::string name) : TreeItem(hex, name) {};
        System(uint8_t hex, std::string name, uint32_t color, uint32_t order) : TreeItem(hex, name, color, order) {};
        System(uint8_t hex, std::string name, std::string interface) : TreeItem(hex, name) {this->interface = interface;};
        System() {};

        std::string interface;
};

class Deck {
    public:
        Deck();

        void add(uint8_t system, std::vector<Command> commands);
        
        System lookup(uint8_t system);
        System lookup(std::string system);
        Command lookup(uint8_t system, uint8_t command);
        Command lookup(std::string system, std::string command);
        std::vector<Command> lookup_commands(uint8_t system);
        std::vector<Command> lookup_commands(std::string system);
        std::vector<std::string> lookup_commands_str(uint8_t system);
        std::vector<std::string> lookup_commands_str(std::string system);
        
        std::vector<System> systems;
        std::vector<Command> commands;

        std::string to_string();

    private:
        std::unordered_map<uint8_t, std::unordered_map<uint8_t, Command>> lookup_map;
};

#endif