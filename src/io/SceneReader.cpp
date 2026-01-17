#include "SceneReader.hpp"

#include <AccelerationStructure.hpp>
#include <InteractiveCamera.hpp>
#include <MeshRenderer.hpp>
#include <MetalRoughMaterial.hpp>
#include <Node.hpp>
#include <QuickTimer.hpp>
#include <Rigidbody.hpp>
#include <TransformUtil.hpp>
#include <YAML_glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <spdlog/spdlog.h>

namespace RtEngine {
	std::shared_ptr<Scene>
	SceneReader::readScene(const std::string &file_path,
						   std::unordered_map<std::string, std::shared_ptr<Material>> materials) {
		QuickTimer quick_timer("Reading scene from file");

		try {
			YAML::Node config = YAML::LoadFile(file_path);
			YAML::Node scene_node = config["scene"];

			auto material_name = scene_node["material_name"].as<std::string>();
			if (!materials.contains(material_name))
				throw std::runtime_error("Material " + material_name + " does not exist");
			// TODO remove vulkan context from reader?
			std::shared_ptr<Scene> scene =
					std::make_shared<Scene>(file_path, materials[material_name]);
			scene->environment_map = std::make_shared<EnvironmentMap>(runtime_context->texture_repository);

			scene->camera = loadCamera(scene_node["camera"]);
			loadSceneLights(scene_node["lights"], scene);

			if (scene_node["environment_map"]) {
				scene->environment_map->loadFromYaml(scene_node["environment_map"]);
			}

			for (const auto &mesh_node: scene_node["meshes"]) {
				std::string mesh_path = mesh_node["path"].as<std::string>();
				runtime_context->mesh_repository->addMesh(mesh_path);
			}

			initializeMaterial(scene_node["materials"], materials[material_name]);

			std::shared_ptr<Node> scene_graph_node = std::make_shared<Node>();
			scene_graph_node->name = "root";
			glm::mat4 identity = glm::mat4(1.0f);
			scene_graph_node->transform->setLocalTransform(identity);
			scene_graph_node->children = {};
			for (const auto &yaml_mesh_node: scene_node["nodes"]) {
				scene_graph_node->children.push_back(
						processSceneNodesRecursiv(static_cast<YAML::Node>(yaml_mesh_node), scene));
			}
			scene->addNode(scene_graph_node->name, scene_graph_node);

			return scene;
		} catch (const YAML::Exception &e) {
			throw std::runtime_error(e.what());
		}
	}

	std::shared_ptr<Camera> SceneReader::loadCamera(const YAML::Node &camera_node) const {
		if (camera_node["interactive"].as<bool>()) {
			return std::make_shared<InteractiveCamera>(camera_node["fov"].as<float>(), camera_node["position"].as<glm::vec3>(),
					camera_node["view_dir"].as<glm::vec3>());
		} else {
			return std::make_shared<Camera>(camera_node["fov"].as<float>(),
											camera_node["position"].as<glm::vec3>(),
											camera_node["view_dir"].as<glm::vec3>());
		}
	}

	void SceneReader::loadSceneLights(const YAML::Node &lights_node, std::shared_ptr<Scene> &scene) {
		if (lights_node["sun"]) {
			YAML::Node sun_node = lights_node["sun"];
			scene->sun = DirectionalLight(sun_node["direction"].as<glm::vec3>(), sun_node["color"].as<glm::vec3>(),
										  sun_node["intensity"].as<float>());
		}

		uint32_t point_light_index = 0;
		for (auto &point_light_node: lights_node["point_lights"]) {
			scene->pointLights[point_light_index++] =
					PointLight(point_light_node["position"].as<glm::vec3>(), point_light_node["color"].as<glm::vec3>(),
							   point_light_node["intensity"].as<float>());
		}
	}

	void SceneReader::initializeMaterial(const YAML::Node &material_nodes, std::shared_ptr<Material> &material) {
		runtime_context->curr_material = material;
		for (const auto &material_node: material_nodes) {
			material->loadInstance(material_node);
		}
	}

	std::shared_ptr<Node> SceneReader::processSceneNodesRecursiv(const YAML::Node &yaml_node,
																 const std::shared_ptr<Scene> &scene) {
		std::shared_ptr<Node> scene_graph_node = std::make_shared<Node>();
		scene_graph_node->name = yaml_node["name"].as<std::string>();
		readComponents(yaml_node, scene_graph_node);
		scene_graph_node->children = {};
		for (auto &child_node: yaml_node["children"]) {
			scene_graph_node->children.push_back(processSceneNodesRecursiv(child_node, scene));
		}
		scene_graph_node->refreshTransform(glm::mat4(1.0f));

		scene->addNode(scene_graph_node->name, scene_graph_node);
		return scene_graph_node;
	}

	void SceneReader::readComponents(const YAML::Node &yaml_node, std::shared_ptr<Node> &scene_node) {
		for (auto &comp_node: yaml_node["components"]) {
			std::string comp_name = "";
			YAML::Node section_node;
			for (auto &pair: comp_node) {
				comp_name = pair.first.as<std::string>();
				section_node = comp_node[comp_name];
			}
			if (comp_name == Transform::COMPONENT_NAME) {
				scene_node->transform->initProperties(comp_node);
			} else if (comp_name == MeshRenderer::COMPONENT_NAME) {
				std::shared_ptr<MeshRenderer> mesh_component =
						std::make_shared<MeshRenderer>(runtime_context, scene_node);
				mesh_component->initProperties(comp_node);
				scene_node->addComponent(mesh_component);
			} else if (comp_name == Rigidbody::COMPONENT_NAME) {
				auto rb = std::make_shared<Rigidbody>(scene_node);
				rb->initProperties(comp_node);
				scene_node->addComponent(rb);
			}
		}
	}
} // namespace RtEngine
