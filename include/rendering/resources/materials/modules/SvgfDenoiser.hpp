//
// Created by oschdi on 23.09.25.
//

#ifndef VULKAN_RAYTRACING_SVGFDENOISER_HPP
#define VULKAN_RAYTRACING_SVGFDENOISER_HPP


class SvgfDenoiser {
public:
    SvgfDenoiser() = default;

    void createComputePipeline();
    void recordCommands();
private:

};


#endif //VULKAN_RAYTRACING_SVGFDENOISER_HPP