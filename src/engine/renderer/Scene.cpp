#include "Scene.hpp"

#include <PhongMaterial.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <components/Camera.hpp>

#include "SceneUtil.hpp"

namespace RtEngine {
	std::shared_ptr<SceneData> Scene::createSceneData(uint32_t emitting_object_count) {
		auto sceneData = std::make_shared<SceneData>();

		//TODO support multiple
		std::vector<std::shared_ptr<Camera>> cameras = SceneUtil::collectCameras(getRootNode());
		assert(cameras.size() > 0);
		std::shared_ptr<Camera> camera = cameras[0];

		sceneData->inverse_view = camera->getInverseView();
		sceneData->inverse_proj = camera->getInverseProjection();
		sceneData->view_pos = glm::vec4(camera->getPosition(), 0.0f);

		std::array<glm::vec4, POINT_LIGHT_COUNT> point_light_positions = {};
		std::array<glm::vec4, POINT_LIGHT_COUNT> point_light_colors = {};
		for (uint32_t i = 0; i < POINT_LIGHT_COUNT; i++) {
			point_light_positions[i] = glm::vec4{pointLights[i].position, pointLights[i].intensity};
			point_light_colors[i] = glm::vec4{pointLights[i].color, 0.0f};
		}

		sceneData->pointLightPositions = point_light_positions;
		sceneData->pointLightColors = point_light_colors;
		sceneData->sunlightDirection = glm::vec4(sun.direction, sun.intensity);
		sceneData->sunlightColor = glm::vec4(sun.color, 0.0f);
		sceneData->sunlightColor = glm::vec4{1, 0, 0, 1.0f};

		sceneData->ambientColor = glm::vec4(0.05f);

		sceneData->emitting_object_count = emitting_object_count;

		return sceneData;
	}

	void Scene::addNode(std::string name, std::shared_ptr<Node> node) {
		assert(!nodes.contains(name));
		nodes[name] = std::move(node);
	}

	std::shared_ptr<Node> Scene::getRootNode() { return nodes["root"]; }

	void Scene::update() {
		getRootNode()->refreshTransform(glm::mat4(1.0f));

		for (auto &node: nodes) {
			node.second->update();
		}
	}

	std::vector<std::shared_ptr<MeshAsset>> Scene::getMeshAssets() {
		return SceneUtil::collectMeshAssets(getRootNode());
	}

	std::vector<std::shared_ptr<MaterialInstance>> Scene::getMaterialInstances() {
		return  SceneUtil::collectMaterialInstances(getRootNode());
	}

	void Scene::fillDrawContext(const std::shared_ptr<DrawContext> &draw_context) {
		getRootNode()->draw(*draw_context);
	}

	std::shared_ptr<Material> Scene::getMaterial() {
		return material;
	}

	std::shared_ptr<EnvironmentMap> Scene::getEnvironmentMap() {
		return environment_map;
	}

	void * Scene::getSceneData(size_t *size, uint32_t emitting_instances_count) {
		last_scene_data = createSceneData(emitting_instances_count);
		*size = sizeof(SceneData);
		return last_scene_data.get();
	}
} // namespace RtEngine
