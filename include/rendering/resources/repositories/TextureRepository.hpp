#ifndef TEXTUREREPOSITORY_HPP
#define TEXTUREREPOSITORY_HPP

#include <DeletionQueue.hpp>
#include <memory>
#include <RessourceBuilder.hpp>
#include <unordered_map>

namespace RtEngine {
class TextureRepository {
public:
    TextureRepository() = default;
    TextureRepository(std::shared_ptr<RessourceBuilder> resource_builder);

    std::shared_ptr<Texture> getTexture(const std::string& name, TextureType type = PARAMETER);
    void addTexture(std::string path, TextureType type);
    void destroy();
private:
    void initDefaultTextures();

    std::shared_ptr<RessourceBuilder> resource_builder;
    DeletionQueue deletion_queue;

    std::unordered_map<std::string, std::shared_ptr<Texture>> texture_name_cache, texture_path_cache;
    std::shared_ptr<Texture> default_tex, default_normal_tex;
};



}
#endif //TEXTUREREPOSITORY_HPP
