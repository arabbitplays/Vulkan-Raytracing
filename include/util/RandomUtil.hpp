//
// Created by oschdi on 6/5/25.
//

#ifndef RANDOMUTIL_HPP
#define RANDOMUTIL_HPP

#include <random>

namespace RtEngine {
    class RandomUtil {
    public:
        static uint32_t generateInt() {
            static std::mt19937 gen(init_seed());
            static std::uniform_int_distribution<uint32_t> dist(0, std::numeric_limits<uint32_t>::max());
            return dist(gen);
        }
    private:
        static uint32_t init_seed() {
            std::random_device rd;
            return rd();
        }
    };
}


#endif //RANDOMUTIL_HPP
