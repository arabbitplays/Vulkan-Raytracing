//
// Created by oschdi on 5/6/25.
//

#ifndef SCENEUTIL_HPP
#define SCENEUTIL_HPP

#include <Node.hpp>

namespace RtEngine {
	class SceneUtil {
	public:
		static std::vector<std::shared_ptr<MeshAsset>> collectMeshAssets(const std::shared_ptr<Node> &root_node) {
			auto mesh_map = std::make_shared<std::unordered_map<std::string, std::shared_ptr<MeshAsset>>>();
			collectMeshAssetsRecursive(root_node, mesh_map);

			std::vector<std::shared_ptr<MeshAsset>> mesh_assets;
			for (auto mesh_asset: *mesh_map) {
				mesh_assets.push_back(mesh_asset.second);
			}
			return mesh_assets;
		}

	private:
		static void collectMeshAssetsRecursive(
				const std::shared_ptr<Node> &root_node,
				std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<MeshAsset>>> &mesh_map) {
			for (auto child_node: root_node->children) {
				std::shared_ptr<MeshRenderer> mesh_renderer = child_node->getComponent<MeshRenderer>();
				if (mesh_renderer && !mesh_map->contains(mesh_renderer->meshAsset->name)) {
					(*mesh_map)[mesh_renderer->meshAsset->name] = mesh_renderer->meshAsset;
				}
				collectMeshAssetsRecursive(child_node, mesh_map);
			}
		}
	};
} // namespace RtEngine

#endif // SCENEUTIL_HPP
