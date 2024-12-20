//
// Created by oschdi on 12/16/24.
//

#ifndef QUICKTIMER_HPP
#define QUICKTIMER_HPP

#include <chrono>
#include <iostream>

struct QuickTimer {
    using clock = std::chrono::high_resolution_clock;
    const char* name;
    clock::time_point start;
    bool print_on_exit;
    explicit QuickTimer(const char* name_, bool print_on_exit_ = true) : name(name_), print_on_exit(print_on_exit_) {
        start = clock::now();
    }

    ~QuickTimer() {
      if (print_on_exit) {
        using namespace std::chrono;
        const auto dur = duration_cast<microseconds>(clock::now() - start).count();
        std::cout << name << ": " << dur / 1000. << " ms" << std::endl;
      }
    }
};

#endif //QUICKTIMER_HPP
