//
// Created by oschdi on 12/30/24.
//

#include "SceneManager.hpp"

#include <DescriptorLayoutBuilder.hpp>
#include <OptionsWindow.hpp>
#include <QuickTimer.hpp>

void SceneManager::createScene(SceneType scene_type) {
    QuickTimer timer{"Scene Creation", true};

    switch (scene_type) {
        case SceneType::CORNELL_BOX:
    scene = std::make_shared<CornellBox>(context->mesh_builder, *context->resource_builder, context->swapchain->extent.width, context->swapchain->extent.height, phong_material);
            break;
        case SceneType::PLANE:
    scene = std::make_shared<PlaneScene>(context->mesh_builder, *context->resource_builder, context->swapchain->extent.width, context->swapchain->extent.height, phong_material);
            break;
    }

    scene_ressource_deletion_queue.pushFunction([&]() {
        scene->clearRessources();
    });

    createSceneBuffers();
    createBlas();
    createUniformBuffers();
    createSceneDescriptorSets();
}

void SceneManager::createSceneBuffers() {
    vertex_buffer = context->mesh_builder->createVertexBuffer(scene->meshes);
    index_buffer = context->mesh_builder->createIndexBuffer(scene->meshes);
    phong_material->writeMaterial();

    geometry_mapping_buffer = context->mesh_builder->createGeometryMappingBuffer(scene->meshes);

    scene_ressource_deletion_queue.pushFunction([&]() {
        context->resource_builder->destroyBuffer(vertex_buffer);
        context->resource_builder->destroyBuffer(index_buffer);
        context->resource_builder->destroyBuffer(geometry_mapping_buffer);
    });
}

void SceneManager::createBlas() {
    QuickTimer timer{"BLAS Build", true};
    uint32_t object_id = 0;
    for (auto& meshAsset : scene->meshes) {
        meshAsset->accelerationStructure = std::make_shared<AccelerationStructure>(context->device, *context->resource_builder, *context->command_manager, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);

        meshAsset->accelerationStructure->addTriangleGeometry(vertex_buffer, index_buffer,
            meshAsset->vertex_count - 1, meshAsset->triangle_count, sizeof(Vertex),
            meshAsset->instance_data.vertex_offset, meshAsset->instance_data.triangle_offset);
        meshAsset->accelerationStructure->build();
        meshAsset->geometry_id = object_id++;
    }
}

void SceneManager::createSceneDescriptorSets() {
    context->descriptor_allocator->writeBuffer(3, vertex_buffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    context->descriptor_allocator->writeBuffer(4, index_buffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    context->descriptor_allocator->writeBuffer(5, geometry_mapping_buffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    std::vector<VkImageView> views{};
    for (uint32_t i = 0; i < 6; i++) {
        views.push_back(scene->environment_map[i].imageView);
    }
    context->descriptor_allocator->writeImages(7, views, defaultSamplerLinear, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    for (int i = 0; i < max_frames_in_flight; i++) {
        scene_descriptor_sets.push_back(context->descriptor_allocator->allocate(context->device, scene_descsriptor_set_layout));
        context->descriptor_allocator->updateSet(context->device, scene_descriptor_sets[i]);
        context->descriptor_allocator->clearWrites();
    }

}

void SceneManager::createSceneLayout() {
    DescriptorLayoutBuilder layoutBuilder;

    layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
    layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    layoutBuilder.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    layoutBuilder.addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    layoutBuilder.addBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    layoutBuilder.addBinding(6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    layoutBuilder.addBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6); // env map

    scene_descsriptor_set_layout = layoutBuilder.build(context->device, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR);
    main_deletion_queue.pushFunction([&]() {
        vkDestroyDescriptorSetLayout(context->device, scene_descsriptor_set_layout, nullptr);
    });
}


void SceneManager::createUniformBuffers() {
    VkDeviceSize size = sizeof(SceneData);

    sceneUniformBuffers.resize(max_frames_in_flight);
    sceneUniformBuffersMapped.resize(max_frames_in_flight);

    for (size_t i = 0; i < max_frames_in_flight; i++) {
        sceneUniformBuffers[i] = context->resource_builder->createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkMapMemory(context->device, sceneUniformBuffers[i].bufferMemory, 0, size, 0, &sceneUniformBuffersMapped[i]);

        scene_ressource_deletion_queue.pushFunction([&, i]() {
            context->resource_builder->destroyBuffer(sceneUniformBuffers[i]);
        });
    }
}

void SceneManager::initDefaultResources() {
    createDefaultTextures();
    createDefaultSamplers();
    createDefaultMaterials();
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
    VkSamplerCreateInfo samplerInfo = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;

    if (vkCreateSampler(context->device, &samplerInfo, nullptr, &defaultSamplerNearest) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    if (vkCreateSampler(context->device, &samplerInfo, nullptr, &defaultSamplerLinear) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    main_deletion_queue.pushFunction([&]() {
        vkDestroySampler(context->device, defaultSamplerLinear, nullptr);
        vkDestroySampler(context->device, defaultSamplerNearest, nullptr);
    });
}


void SceneManager::createDefaultMaterials() {
    phong_material = std::make_shared<PhongMaterial>(context->device, *context->resource_builder);
    phong_material->buildPipelines(scene_descsriptor_set_layout);
    main_deletion_queue.pushFunction([&]() {
        phong_material->clearRessources();
    });
    //default_phong = createPhongMaterial(glm::vec3{1, 0, 0}, 1, 1, 1);
}

PhongMaterial SceneManager::getMaterial() {
    return *phong_material;
}

void SceneManager::clearRessources() {
    main_deletion_queue.flush();
    scene_ressource_deletion_queue.flush();
}
