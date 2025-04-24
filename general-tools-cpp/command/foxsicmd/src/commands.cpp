#include "commands.h"
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <iomanip>

TreeItem::TreeItem(uint8_t hex, std::string name) {
    this->hex = hex;
    this->name = name;
}

TreeItem::TreeItem(uint8_t hex, std::string name, uint32_t color, uint32_t order) {
    this->hex = hex;
    this->name = name;
    this->color = color;
    this->order = order;
}

std::string TreeItem::to_string() {
    return std::string("name: " + this->name + "\n" + "hex: " + std::to_string(this->hex));
}

ftxui::Color TreeItem::get_color() {
    uint8_t r = (this->color >> 16) & 0xff;
    uint8_t g = (this->color >> 8)  & 0xff;
    uint8_t b = (this->color >> 0)  & 0xff;

    return ftxui::Color::RGB(r, g, b);
}

Deck::Deck() {
    this->systems = std::vector<System>();
    this->commands = std::vector<Command>();
}

void Deck::add(uint8_t system, std::vector<Command> commands) { 
    std::unordered_map<uint8_t, Command> inner;
    for (auto c: commands) {
        inner[c.hex] = c;
    }
    this->lookup_map[system] = inner;
}

System Deck::lookup(uint8_t system) {
    for (auto s : this->systems) {
        if (s.hex == system) {
            return s;
        }
    }
    return System();
}
System Deck::lookup(std::string system) {
    for (auto s : this->systems) {
        if (s.name == system) {
            return s;
        }
    }
    return System();
}
Command Deck::lookup(uint8_t system, uint8_t command) {
    for (auto s : this->systems) {
        if (s.hex == system) {
            for (auto c : this->commands) {
                if (c.hex == command) {
                    return c;
                }
            }
        }
    }
    return Command();
}

Command Deck::lookup(std::string system, std::string command) {
    for (auto s : this->systems) {
        if (s.name == system) {
            for (auto c : this->commands) {
                if (c.name == command) {
                    return c;
                }
            }
        }
    }
    return Command();
}

std::vector<Command> Deck::lookup_commands(uint8_t system) {
    std::vector<Command> result;
    for (auto& c: this->lookup_map[system]) {
        result.push_back(c.second);
    }
    return result;
}

std::vector<Command> Deck::lookup_commands(std::string system) {
    std::vector<Command> result;
    for (auto& c: this->lookup_map[this->lookup(system).hex]) {
        result.push_back(c.second);
    }
    return result;
}

std::vector<std::string> Deck::lookup_commands_str(uint8_t system) {
    std::vector<std::string> result;
    for (auto& c: this->lookup_map[system]) {
        result.push_back(c.second.name);
    }
    return result;
}
std::vector<std::string> Deck::lookup_commands_str(std::string system) {
    std::vector<std::string> result;
    for (auto& c: this->lookup_map[this->lookup(system).hex]) {
        result.push_back(c.second.name);
    }
    return result;
}

std::string Deck::to_string() {
    std::stringstream result;
    result << "Deck::systems:\n";

    for (auto& s: this->lookup_map) {
        result << "\t" << this->lookup(s.first).name << "\n";
        for (auto& c: this->lookup_map[s.first]) {
            result << "\t\t" << c.second.name << "\n";
        }
    }
    return result.str();
}