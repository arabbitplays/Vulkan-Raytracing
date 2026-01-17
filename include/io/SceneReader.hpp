#ifndef SCENEREADER_H
#define SCENEREADER_H

#include <RuntimeContext.hpp>
#include <Scene.hpp>
#include <string>
#include <yaml-cpp/yaml.h>

namespace RtEngine {
	class SceneReader {
	public:
		SceneReader() = default;
		SceneReader(const std::shared_ptr<RuntimeContext> &runtimeContext) : runtime_context(runtimeContext) {}

		std::shared_ptr<Scene> readScene(const std::string &file_path,
										 std::unordered_map<std::string, std::shared_ptr<Material>> materials);
		std::shared_ptr<Camera> loadCamera(const YAML::Node &camera_node) const;
		void loadSceneLights(const YAML::Node &lights_node, std::shared_ptr<Scene> &scene);
		void initializeMaterial(const YAML::Node &material_node, std::shared_ptr<Material> &material);

	private:
		std::shared_ptr<Node> processSceneNodesRecursiv(const YAML::Node &yaml_node,
														const std::shared_ptr<Scene> &scene);
		void readComponents(const YAML::Node &yaml_node, std::shared_ptr<Node> &scene_node);

		std::shared_ptr<RuntimeContext> runtime_context;
	};

} // namespace RtEngine
#endif // SCENEREADER_H
