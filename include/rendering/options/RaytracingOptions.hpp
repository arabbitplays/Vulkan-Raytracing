//
// Created by oschdi on 1/20/25.
//

#ifndef RAYTRACINGOPTIONS_HPP
#define RAYTRACINGOPTIONS_HPP

struct RaytracingOptions {
    int32_t recursion_depth = 5;
    int32_t shadows = 1;
    int32_t fresnel = 1;
    int32_t dispersion = 0;
    int32_t normal_mapping = 1;

    uint32_t sample_light = 0;
    uint32_t curr_sample_count = 0;
    uint32_t emitting_instances_count = 0;
};


#endif //RAYTRACINGOPTIONS_HPP
