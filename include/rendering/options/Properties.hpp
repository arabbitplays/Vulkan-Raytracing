//
// Created by oschdi on 2/28/25.
//

#ifndef PROPERTIES_HPP
#define PROPERTIES_HPP

#include <string>
#include <unordered_map>

struct BoolOption {
  std::string name;
  int32_t* var;
  bool imgui_option;
};

struct Properties {
public:
  Properties(std::string name) : section_name(name) {}

  void addBool(const std::string& name, int32_t* var) {
    bool_options.push_back(std::make_shared<BoolOption>(name, var, *var > 0 ? true : false));
  }

  std::string section_name;
  std::vector<std::shared_ptr<BoolOption>> bool_options;
};

#endif //PROPERTIES_HPP
