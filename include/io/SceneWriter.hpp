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

private:
  void writeMaterial(YAML::Emitter& out, const std::shared_ptr<Material>& material);
  void writeSceneLights(YAML::Emitter& out, const std::shared_ptr<Scene>& scene);
  void writeSceneNode(YAML::Emitter& out, const std::shared_ptr<Node>& node);
};



#endif //SCENEWRITER_H
