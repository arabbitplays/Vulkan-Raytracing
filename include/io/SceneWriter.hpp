//
// Created by oschdi on 2/4/25.
//

#ifndef SCENEWRITER_H
#define SCENEWRITER_H

#include <Scene.hpp>
#include <string>
#include <yaml-cpp/yaml.h>

class SceneWriter {
public:
  SceneWriter() = default;

  void writeScene(const std::string& filename, std::shared_ptr<Scene> scene);
};



#endif //SCENEWRITER_H
