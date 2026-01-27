//
// Created by oschdi on 2/13/25.
//

#ifndef YAML_GLM_HPP
#define YAML_GLM_HPP

#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

// Custom emitter to serialize glm::vec3
namespace YAML {
	template<>
	struct convert<glm::vec3> {
		static Node encode(const glm::vec3 &v) {
			Node node;
			node.push_back(v.x);
			node.push_back(v.y);
			node.push_back(v.z);
			return node;
		}

		static bool decode(const Node &node, glm::vec3 &v) {
			if (!node.IsSequence() || node.size() != 3) {
				return false;
			}
			v.x = node[0].as<float>();
			v.y = node[1].as<float>();
			v.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4> {
		static Node encode(const glm::vec4 &v) {
			Node node;
			node.push_back(v.x);
			node.push_back(v.y);
			node.push_back(v.z);
			node.push_back(v.w);
			return node;
		}

		static bool decode(const Node &node, glm::vec4 &v) {
			if (!node.IsSequence() || node.size() != 3) {
				return false;
			}
			v.x = node[0].as<float>();
			v.y = node[1].as<float>();
			v.z = node[2].as<float>();
			v.z = node[3].as<float>();
			return true;
		}
	};
} // namespace YAML

#endif // YAML_GLM_HPP
