//
// Created by oster on 16.09.2024.
//

#ifndef BASICS_ANALYTICS_HPP
#define BASICS_ANALYTICS_HPP


#include <chrono>
#include <iostream>

using namespace std::chrono;

class Analytics {
public:
    void startFrame() {
        startTime = std::chrono::high_resolution_clock::now();
    }

    void endFrame() {
        if (!startTime.has_value())
            return;

        auto currTime = std::chrono::high_resolution_clock::now();
        float difference = duration<float, seconds::period>(currTime - startTime.value()).count();
        samples.push_back(difference);
        startTime = currTime;

        if (samples.size() > 10) {
            float sum = 0;
            for (auto sample : samples) {
                sum += sample;
            }
            float averageTime = sum / 10.0f;
            std::cout << "FPS: " << 1 / averageTime << std::endl;

            samples.clear();
        }
    };
private:
    std::optional<time_point<system_clock, nanoseconds>> startTime;
    std::vector<float> samples;
};


#endif //BASICS_ANALYTICS_HPP
