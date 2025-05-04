#include "SceneManager.hpp"

#include <DescriptorLayoutBuilder.hpp>
#include <MeshRenderer.hpp>
#include <OptionsWindow.hpp>
#include <QuickTimer.hpp>
#include <SceneReader.hpp>
#include <SceneWriter.hpp>

namespace RtEngine {
void collectMeshAssetsRecursive(const std::shared_ptr<Node>& root_node, std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<MeshAsset>>>& mesh_map)
{
    for (auto child_node : root_node->children)
    {
        std::shared_ptr<MeshRenderer> mesh_renderer = child_node->getComponent<MeshRenderer>();
        if (mesh_renderer && !mesh_map->contains(mesh_renderer->meshAsset->name))
        {
            (*mesh_map)[mesh_renderer->meshAsset->name] = mesh_renderer->meshAsset;
        }
        collectMeshAssetsRecursive(child_node, mesh_map);
    }
}

std::vector<std::shared_ptr<MeshAsset>> collectMeshAssets(const std::shared_ptr<Node>& root_node)
{
    auto mesh_map = std::make_shared<std::unordered_map<std::string, std::shared_ptr<MeshAsset>>>();
    collectMeshAssetsRecursive(root_node, mesh_map);

    std::vector<std::shared_ptr<MeshAsset>> mesh_assets;
    for (auto mesh_asset : *mesh_map)
    {
        mesh_assets.push_back(mesh_asset.second);
    }
    return mesh_assets;
}

void SceneManager::createScene(std::string scene_path) {
    QuickTimer timer{"Scene Creation", true};

    if (scene != nullptr) {
        scene_resource_deletion_queue.flush();
        for (auto& material : defaultMaterials)
        {
            material.second->reset();
        }
    }

    SceneReader reader = SceneReader(context);
    scene = reader.readScene(scene_path, defaultMaterials);

    scene_resource_deletion_queue.pushFunction([&]() {
        scene->clearResources();
    });

    std::vector<std::shared_ptr<MeshAsset>> mesh_assets = collectMeshAssets(scene->getRootNode());
    geometry_manager->createGeometryBuffers(mesh_assets);
    scene->material->writeMaterial();
    createBlas(mesh_assets);
    createUniformBuffers();
    bufferUpdateFlags = static_cast<uint8_t>(GEOMETRY_UPDATE) | static_cast<uint8_t>(MATERIAL_UPDATE);
}

void SceneManager::createBlas(std::vector<std::shared_ptr<MeshAsset>>& meshes) {
    QuickTimer timer{"BLAS Build", true};
    VkDevice device = context->device_manager->getDevice();

    uint32_t object_id = 0;
    for (auto& meshAsset : meshes) {
        meshAsset->accelerationStructure = std::make_shared<AccelerationStructure>(device, *context->resource_builder, *context->command_manager, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);

        meshAsset->accelerationStructure->addTriangleGeometry(geometry_manager->getVertexBuffer(), geometry_manager->getIndexBuffer(),
            meshAsset->vertex_count - 1, meshAsset->triangle_count, sizeof(Vertex),
            meshAsset->instance_data.vertex_offset, meshAsset->instance_data.triangle_offset);
        meshAsset->accelerationStructure->build();
        meshAsset->geometry_id = object_id++;
    }

    scene_resource_deletion_queue.pushFunction([&]()
    {
        for (auto& meshAsset : collectMeshAssets(scene->getRootNode())) {
            meshAsset->accelerationStructure->destroy();
        }
    });
}

void SceneManager::createSceneLayout() {
    DescriptorLayoutBuilder layoutBuilder;

    layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR); // TLAS
    layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // render image
    layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // scene data
    layoutBuilder.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // vertex buffer
    layoutBuilder.addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // index buffer
    layoutBuilder.addBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // geometry buffer
    layoutBuilder.addBinding(6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // instance buffer
    layoutBuilder.addBinding(7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // emitting instances buffer
    layoutBuilder.addBinding(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6); // env map
    layoutBuilder.addBinding(9, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // rng tex

    scene_descsriptor_set_layout = layoutBuilder.build(context->device_manager->getDevice(), VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR);
    main_deletion_queue.pushFunction([&]() {
        vkDestroyDescriptorSetLayout(context->device_manager->getDevice(), scene_descsriptor_set_layout, nullptr);
    });
}

void SceneManager::createSceneDescriptorSets(const VkDescriptorSetLayout& layout)
{
    for (int i = 0; i < max_frames_in_flight; i++) {
        scene_descriptor_sets.push_back(context->descriptor_allocator->allocate(context->device_manager->getDevice(), layout));
    }
}

void SceneManager::createUniformBuffers() {
    VkDeviceSize size = sizeof(SceneData);

    sceneUniformBuffers.resize(max_frames_in_flight);
    sceneUniformBuffersMapped.resize(max_frames_in_flight);

    for (size_t i = 0; i < max_frames_in_flight; i++) {
        sceneUniformBuffers[i] = context->resource_builder->createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkMapMemory(context->device_manager->getDevice(), sceneUniformBuffers[i].bufferMemory, 0, size, 0, &sceneUniformBuffersMapped[i]);

        scene_resource_deletion_queue.pushFunction([&, i]() {
            context->resource_builder->destroyBuffer(sceneUniformBuffers[i]);
        });
    }
}

// ----------------------------------------------------------------------------------------------------------------

void SceneManager::updateScene(DrawContext& draw_context, uint32_t current_image_idx, const AllocatedImage& current_image, const AllocatedImage&
                               rng_tex) {
    assert(scene != nullptr);

    //QuickTimer timer{"Scene Update", true};
    VkDevice device = context->device_manager->getDevice();

    scene->update(context->swapchain->extent.width, context->swapchain->extent.height);

    if (!(bufferUpdateFlags & NO_UPDATE))
        vkDeviceWaitIdle(device);

    draw_context.objects.clear();
    scene->nodes["root"]->draw(draw_context);

    if (bufferUpdateFlags & GEOMETRY_UPDATE) {
        instance_manager->createInstanceMappingBuffer(draw_context.objects);
    }

    if (bufferUpdateFlags & GEOMETRY_UPDATE || bufferUpdateFlags & MATERIAL_UPDATE)
    {
        instance_manager->createEmittingInstancesBuffer(draw_context.objects, getMaterial());
    }

    if (bufferUpdateFlags != NO_UPDATE)
    {
        if (bufferUpdateFlags & MATERIAL_UPDATE)
        {
            scene->material->writeMaterial();
        }
    }

    if (bufferUpdateFlags & GEOMETRY_UPDATE)
    {
        updateTlas(top_level_acceleration_structure, draw_context.objects);
    }

    std::shared_ptr<SceneData> scene_data = scene->createSceneData();
    memcpy(sceneUniformBuffersMapped[current_image_idx], scene_data.get(), sizeof(SceneData));

    updateSceneDescriptorSets(current_image_idx, current_image, rng_tex);

    bufferUpdateFlags = NO_UPDATE;
}

void SceneManager::updateTlas(std::shared_ptr<AccelerationStructure>& tlas, std::vector<RenderObject> objects)
{
    uint32_t instance_id = 0;
    for (int i = 0; i < objects.size(); i++) {
        tlas->addInstance(objects[i].acceleration_structure, objects[i].transform, instance_id++);
    }

    if (tlas->getHandle() == VK_NULL_HANDLE) {
        tlas->addInstanceGeometry();
    } else {
        tlas->update_instance_geometry(0);
    }
    tlas->build();
}

void SceneManager::updateSceneDescriptorSets(uint32_t current_image_idx, const AllocatedImage& current_image, const AllocatedImage& rng_tex) {
    VkDevice device = context->device_manager->getDevice();

    VkAccelerationStructureKHR tlas_handle = top_level_acceleration_structure->getHandle();
    if (bufferUpdateFlags & GEOMETRY_UPDATE)
    {
        context->descriptor_allocator->writeAccelerationStructure(0, tlas_handle, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
        context->descriptor_allocator->writeBuffer(3, geometry_manager->getVertexBuffer().handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        context->descriptor_allocator->writeBuffer(4, geometry_manager->getIndexBuffer().handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        context->descriptor_allocator->writeBuffer(5, geometry_manager->getGeometryMappingBuffer().handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        context->descriptor_allocator->writeBuffer(6, instance_manager->getInstanceBuffer().handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        context->descriptor_allocator->writeBuffer(7, instance_manager->getEmittingInstancesBuffer().handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

        std::vector<VkImageView> views{};
        for (uint32_t i = 0; i < 6; i++) {
            views.push_back(scene->environment_map[i].imageView);
        }
        context->descriptor_allocator->writeImages(8, views, defaultSamplerLinear, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        for (int i = 0; i < max_frames_in_flight; i++) {
            context->descriptor_allocator->updateSet(device, scene_descriptor_sets[i]);
            context->descriptor_allocator->clearWrites();
        }
    }

    context->descriptor_allocator->writeImage(1, current_image.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    context->descriptor_allocator->writeBuffer(2, sceneUniformBuffers[current_image_idx].handle, sizeof(SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    context->descriptor_allocator->writeImage(9, rng_tex.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    context->descriptor_allocator->updateSet(device, scene_descriptor_sets[current_image_idx]);
    context->descriptor_allocator->clearWrites();
}

// -----------------------------------------------------------------------------------------------------------------

void SceneManager::initDefaultResources(VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties) {
    createDefaultTextures();
    createDefaultSamplers();
    createDefaultMaterials(raytracingProperties);
}

void SceneManager::createDefaultTextures() {
    uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
    whiteImage = context->resource_builder->createImage((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_SRGB,
                               VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
    greyImage = context->resource_builder->createImage((void*)&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_SRGB,
                                              VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
    blackImage = context->resource_builder->createImage((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_SRGB,
                                               VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    //checkerboard image
    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
    std::array<uint32_t, 16 *16 > pixels; //for 16x16 checkerboard texture
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }
    }
    errorCheckerboardImage = context->resource_builder->createImage(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_SRGB,
                                           VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    main_deletion_queue.pushFunction([&]() {
        context->resource_builder->destroyImage(whiteImage);
        context->resource_builder->destroyImage(greyImage);
        context->resource_builder->destroyImage(blackImage);
        context->resource_builder->destroyImage(errorCheckerboardImage);
    });
}

void SceneManager::createDefaultSamplers() {
    VkDevice device = context->device_manager->getDevice();

    VkSamplerCreateInfo samplerInfo = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerNearest) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerLinear) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(context->device_manager->getPhysicalDevice(), &properties);
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerAnisotropic) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    main_deletion_queue.pushFunction([&]() {
        vkDestroySampler(context->device_manager->getDevice(), defaultSamplerLinear, nullptr);
        vkDestroySampler(context->device_manager->getDevice(), defaultSamplerNearest, nullptr);
        vkDestroySampler(context->device_manager->getDevice(), defaultSamplerAnisotropic, nullptr);
    });
}

void SceneManager::createDefaultMaterials(VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties)
{
    auto phong_material = std::make_shared<PhongMaterial>(context);
    phong_material->buildPipelines(scene_descsriptor_set_layout);
    phong_material->pipeline->createShaderBindingTables(raytracingProperties);
    defaultMaterials["phong"] = phong_material;
    main_deletion_queue.pushFunction([&]() {
        defaultMaterials["phong"]->clearRessources();
    });

    auto metal_rough_material = std::make_shared<MetalRoughMaterial>(context, defaultSamplerLinear);
    metal_rough_material->buildPipelines(scene_descsriptor_set_layout);
    metal_rough_material->pipeline->createShaderBindingTables(raytracingProperties);
    defaultMaterials["metal_rough"] = metal_rough_material;
    main_deletion_queue.pushFunction([&]() {
        defaultMaterials["metal_rough"]->clearRessources();
    });
}

std::shared_ptr<Material> SceneManager::getMaterial() const
{
    return scene->material;
}

VkDescriptorSet SceneManager::getSceneDescriptorSet() const
{
    return scene_descriptor_sets[0];
}


uint32_t SceneManager::getEmittingInstancesCount() {
    return instance_manager->getEmittingInstancesCount();
}


void SceneManager::clearRessources() {
    scene_resource_deletion_queue.flush();
    main_deletion_queue.flush();
}
}
