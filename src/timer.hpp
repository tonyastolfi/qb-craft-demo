#pragma once

#include <chrono>

inline double
elapsed_seconds(const std::chrono::steady_clock::time_point &start) {
  using namespace std::chrono;

  return double((steady_clock::now() - start).count()) *
         steady_clock::period::num / steady_clock::period::den;
}
