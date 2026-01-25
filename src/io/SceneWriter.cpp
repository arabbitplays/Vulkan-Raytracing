#include "SceneWriter.hpp"
#include <MeshRenderer.hpp>
#include <QuickTimer.hpp>
#include <SceneUtil.hpp>
#include <TransformUtil.hpp>
#include <YAML_glm.hpp>
#include <fstream>
#include <iostream>
#include <MetalRoughMaterial.hpp>
#include <spdlog/spdlog.h>

namespace RtEngine {
	void SceneWriter::writeScene(const std::string &filename, std::shared_ptr<Scene> scene) {
		QuickTimer quick_timer("Writing scene to file");

		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "scene" << YAML::Value << YAML::BeginMap;

		out << YAML::Key << "material_name" << YAML::Value << scene->material->name;

		writeSceneLights(out, scene);

		std::vector<std::shared_ptr<MeshAsset>> meshes = SceneUtil::collectMeshAssets(scene->getRootNode());
		out << YAML::Key << "meshes" << YAML::Value << YAML::BeginSeq;
		for (const auto &mesh: meshes) {
			out << YAML::BeginMap;
			out << YAML::Key << "path" << YAML::Value << mesh->path;
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;

		writeMaterial(out, scene->material);

		out << YAML::Key << "nodes" << YAML::Value << YAML::BeginSeq;
		for (const auto &node: scene->nodes["root"]->children) {
			writeSceneNode(out, node);
		}
		out << YAML::EndSeq;

		out << YAML::EndMap;
		out << YAML::EndMap;

		std::string path = filename + ".yaml";
		std::ofstream fout(path);
		fout << out.c_str();
		fout.close();

		spdlog::info("Scene successfully written to {}", path);
	}

	void SceneWriter::writeMaterial(YAML::Emitter &out, const std::shared_ptr<Material> &material) {
		out << YAML::Key << "materials" << YAML::Value << YAML::BeginSeq;
		for (auto &instance: material->getInstances()) {
			out << instance->writeResourcesToYaml();
		}
		out << YAML::EndSeq;
	}

	void SceneWriter::writeSceneLights(YAML::Emitter &out, const std::shared_ptr<Scene> &scene) {
		out << YAML::Key << "lights" << YAML::Value << YAML::BeginMap;

		if (scene->sun.intensity != 0) {
			out << YAML::Key << "sun" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "direction" << YAML::Value << YAML::convert<glm::vec3>::encode(scene->sun.direction);
			out << YAML::Key << "color" << YAML::Value << YAML::convert<glm::vec3>::encode(scene->sun.color);
			out << YAML::Key << "intensity" << YAML::Value << scene->sun.intensity;
			out << YAML::EndMap;
		}

		out << YAML::Key << "point_lights" << YAML::Value << YAML::BeginSeq;
		for (const auto &light: scene->pointLights) {
			if (light.intensity != 0) {
				out << YAML::BeginMap;
				out << YAML::Key << "position" << YAML::Value << YAML::convert<glm::vec3>::encode(light.position);
				out << YAML::Key << "color" << YAML::Value << YAML::convert<glm::vec3>::encode(light.color);
				out << YAML::Key << "intensity" << YAML::Value << light.intensity;
				out << YAML::EndMap;
			}
		}
		out << YAML::EndSeq;

		out << YAML::EndMap;
	}

	float roundToDecimal(float value, int decimalPlaces) {
		float multiplier = std::pow(10.0f, decimalPlaces);
		return std::round(value * multiplier) / multiplier;
	}

	// Round each component of the glm::vec3
	glm::vec3 roundVec3(glm::vec3 v, int decimalPlaces) {
		v.x = roundToDecimal(v.x, decimalPlaces);
		v.y = roundToDecimal(v.y, decimalPlaces);
		v.z = roundToDecimal(v.z, decimalPlaces);
		return v;
	}

	void SceneWriter::writeSceneNode(YAML::Emitter &out, const std::shared_ptr<Node> &node) {
		out << YAML::BeginMap;
		out << YAML::Key << "name" << YAML::Value << node->name;

		out << YAML::Key << "components" << YAML::Value << YAML::BeginSeq;
		for (auto &component: node->components) {
			writeComponent(out, component);
		}
		out << YAML::EndSeq;

		out << YAML::Key << "children" << YAML::Value << YAML::BeginSeq;
		for (const auto &child: node->children) {
			writeSceneNode(out, child);
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;
	}

	void SceneWriter::writeComponent(YAML::Emitter &out, const std::shared_ptr<Component> &component) {
		/*std::shared_ptr<PropertiesManager> properties = component->getProperties();
		std::vector<std::shared_ptr<PropertiesSection>> sections = properties->getSections(PERSISTENT_PROPERTY_FLAG);
		assert(sections.size() <= 1);
		if (sections.empty())
			return;

		out << YAML::BeginMap;

		for (auto &section: sections) {
			out << YAML::Key << section->section_name << YAML::BeginMap;

			for (auto &prop: section->bool_properties) {
				if (prop->flags & PERSISTENT_PROPERTY_FLAG)
					out << YAML::Key << prop->name << YAML::Value << *prop->var;
			}

			for (auto &prop: section->float_properties) {
				if (prop->flags & PERSISTENT_PROPERTY_FLAG)
					out << YAML::Key << prop->name << YAML::Value << *prop->var;
			}

			for (auto &prop: section->int_properties) {
				if (prop->flags & PERSISTENT_PROPERTY_FLAG)
					out << YAML::Key << prop->name << YAML::Value << *prop->var;
			}

			for (auto &prop: section->string_properties) {
				if (prop->flags & PERSISTENT_PROPERTY_FLAG)
					out << YAML::Key << prop->name << YAML::Value << *prop->var;
			}

			for (auto &prop: section->vector_properties) {
				if (prop->flags & PERSISTENT_PROPERTY_FLAG)
					out << YAML::Key << prop->name << YAML::Value << YAML::convert<glm::vec3>::encode(*prop->var);
			}

			for (auto &prop: section->selection_properties) {
				if (prop->flags & PERSISTENT_PROPERTY_FLAG)
					out << YAML::Key << prop->name << YAML::Value << *prop->var;
			}

			out << YAML::EndMap;
		}
		auto section = sections.at(0);

		out << YAML::EndMap;*/
	}
} // namespace RtEngine
