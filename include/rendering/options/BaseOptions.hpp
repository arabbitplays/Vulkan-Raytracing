//
// Created by oschdi on 3/4/25.
//

#ifndef BASEOPTIONS_HPP
#define BASEOPTIONS_HPP

#include <string>

constexpr std::string RECURSION_DEPTH_OPTION_NAME = "Max_Depth";
constexpr std::string RESOURCES_DIR_OPTION_NAME = "Resources_Dir";
constexpr std::string CURR_SCENE_OPTION_NAME = "Scene";

struct BaseOptions {
  int32_t max_depth = 3;
  std::string resources_dir;
  std::string curr_scene_name;
};

#endif //BASEOPTIONS_HPP
