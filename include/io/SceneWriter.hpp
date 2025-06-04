#ifndef SCENEWRITER_H
#define SCENEWRITER_H

#include <Scene.hpp>
#include <string>
#include <yaml-cpp/yaml.h>

namespace RtEngine {
	class SceneWriter {
	public:
		SceneWriter() = default;

		void writeScene(const std::string &filename, std::shared_ptr<Scene> scene);

	private:
		void writeMaterial(YAML::Emitter &out, const std::shared_ptr<Material> &material);
		void writeSceneLights(YAML::Emitter &out, const std::shared_ptr<Scene> &scene);
		void writeComponent(YAML::Emitter &out, const std::shared_ptr<Component> &component);
		void writeSceneNode(YAML::Emitter &out, const std::shared_ptr<Node> &node);
	};

} // namespace RtEngine
#endif // SCENEWRITER_H
