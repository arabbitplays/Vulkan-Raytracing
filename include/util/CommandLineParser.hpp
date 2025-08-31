#ifndef COMMANDLINEPARSER_HPP
#define COMMANDLINEPARSER_HPP

#include <string>
#include <vector>
#include "spdlog/spdlog.h"

namespace RtEngine {
	class CommandLineParser {
		struct IntArgument {
			int32_t *variable{};
			std::string description;
		};

		struct BoolArgument {
			bool *variable{};
			std::string description;
		};

		struct StringArgument {
			std::string *variable{};
			std::string description;
		};

	public:
		CommandLineParser() = default;

		void parse(int argc, char *argv[]) {
			std::vector<std::string> args(argv + 1, argv + argc); // Skip program name
			for (uint32_t i = 0; i < args.size(); i++) {
				if (int_arguments.contains(args[i]) && i < args.size() - 1) {
					*int_arguments[args[i]].variable = std::stoi(args[i + 1]);
					i++;
				} else if (bool_arguments.contains(args[i])) {
					*bool_arguments[args[i]].variable = true;
				} else if (string_arguments.contains(args[i])) {
					*string_arguments[args[i]].variable = args[i + 1];
					i++;
				} else {
					spdlog::error("Invalid argument: {}", args[i]);
				}
			}
		}

		void addInt(const std::string &key, int32_t *variable, const std::string &description) {
			const auto argument = IntArgument(variable, description);
			int_arguments[key] = argument;
		}

		void addFlag(const std::string &key, bool *variable, const std::string &description) {
			const auto argument = BoolArgument(variable, description);
			bool_arguments[key] = argument;
		}

		void addString(const std::string& key, std::string *variable, const std::string &description) {
			const auto argument = StringArgument(variable, description);
			string_arguments[key] = argument;
		}

		void printHelp() const {
			std::vector<std::string> options{};
			std::vector<std::string> descriptions{};

			for (const auto& argument: bool_arguments) {
				options.push_back(argument.first);
				descriptions.push_back(argument.second.description);
			}

			for (const auto& argument: int_arguments) {
				options.push_back(argument.first);
				descriptions.push_back(argument.second.description);
			}

			for (const auto& argument: string_arguments) {
				options.push_back(argument.first);
				descriptions.push_back(argument.second.description);
			}

			// calculate the length of the longest options to properly pad all the others
			size_t max_option_length = 0;
			for (const auto& option: options) {
				max_option_length = std::max(max_option_length, option.size());
			}

			spdlog::info("Options:");
			for (uint32_t i = 0; i < options.size(); i++) {
				spdlog::info("{} {}{}", options[i], std::string(max_option_length - options[i].size(), ' '),
							 descriptions[i]);
			}
		}

	private:
		std::unordered_map<std::string, IntArgument> int_arguments;
		std::unordered_map<std::string, BoolArgument> bool_arguments;
		std::unordered_map<std::string, StringArgument> string_arguments;
	};

} // namespace RtEngine
#endif // COMMANDLINEPARSER_HPP
