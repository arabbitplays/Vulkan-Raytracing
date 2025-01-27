//
// Created by oschdi on 1/23/25.
//

#ifndef COMMANDLINEPARSER_HPP
#define COMMANDLINEPARSER_HPP

#include <vector>
#include <string>
#include "spdlog/spdlog.h"

class CommandLineParser {
    struct IntArgument {
        int32_t* variable;
        std::string description;
    };

    struct BoolArgument {
        bool* variable;
        std::string description;
    };

  public:
    CommandLineParser() = default;

    void parse(int argc, char *argv[]) {
        std::vector<std::string> args(argv + 1, argv + argc); // Skip program name
        for (uint32_t i = 0; i < args.size(); i++) {
            if (int_arguments.find(args[i]) != int_arguments.end() && i < args.size() - 1) {
                *int_arguments[args[i]].variable = std::stoi(args[i + 1]);
                i++;
            } else if (bool_arguments.find(args[i]) != bool_arguments.end()) {
                *bool_arguments[args[i]].variable = true;
            } else {
                spdlog::error("Invalid argument: {}", args[i]);
            }
        }
    }

    void addInt(std::string key, int32_t* variable, std::string description) {
        IntArgument argument = IntArgument(variable, description);
        int_arguments[key] = argument;
    }

    void addFlag(std::string key, bool* variable, std::string description) {
        BoolArgument argument = BoolArgument(variable, description);
        bool_arguments[key] = argument;
    }

    void printHelp() {
        spdlog::info("Options:");
        for (auto argument : bool_arguments) {
            spdlog::info("{} \t\t\t {}", argument.first, argument.second.description);
        }
    }

private:
    std::unordered_map<std::string, IntArgument> int_arguments;
    std::unordered_map<std::string, BoolArgument> bool_arguments;
};

#endif //COMMANDLINEPARSER_HPP
