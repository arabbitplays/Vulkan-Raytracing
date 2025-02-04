//
// Created by oschdi on 2/4/25.
//

#include "SceneWriter.hpp"
#include <iostream>
#include <fstream>
#include <spdlog/spdlog.h>

// Custom emitter to serialize glm::vec3
namespace YAML {
    template <>
    struct convert<glm::vec3> {
        static Node encode(const glm::vec3& v) {
            Node node;
            node.push_back(v.x);
            node.push_back(v.y);
            node.push_back(v.z);
            return node;
        }

        static bool decode(const Node& node, glm::vec3& v) {
            if (!node.IsSequence() || node.size() != 3) {
                return false;
            }
            v.x = node[0].as<float>();
            v.y = node[1].as<float>();
            v.z = node[2].as<float>();
            return true;
        }
    };
}


void SceneWriter::writeScene(const std::string& filename, std::shared_ptr<Scene> scene) {
    YAML::Emitter out;

    out << YAML::BeginMap;
    out << YAML::Key << "scene" << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "meshes" << YAML::Value << YAML::BeginSeq;
    for (const auto& mesh : scene->meshes) {
      out << YAML::BeginMap;
      out << YAML::Key << "name" << YAML::Value << mesh->name;
      out << YAML::Key << "path" << YAML::Value << mesh->path;
      out << YAML::EndMap;
    }
    out << YAML::EndSeq;

    out << YAML::Key << "camera" << YAML::Value << YAML::BeginMap;
    Camera camera = *scene->camera;
    out << YAML::Key << "position" << YAML::Value << YAML::convert<glm::vec3>::encode(camera.position);
    out << YAML::Key << "view_dir" << YAML::Value << YAML::convert<glm::vec3>::encode(camera.view_dir);
    out << YAML::Key << "fov" << YAML::Value << camera.fov;
    out << YAML::Key << "interactive" << YAML::Value << false;
    out << YAML::EndMap;

    out << YAML::EndMap;
    out << YAML::EndMap;

    std::ofstream fout(filename);
    fout << out.c_str();
    fout.close();

    spdlog::info("Scene successfully written to {}", filename);
}

