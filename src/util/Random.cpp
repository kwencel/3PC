#include <random>
#include <chrono>
#include "Random.h"

Random::Random() {
    unsigned seed = (unsigned) std::chrono::system_clock::now().time_since_epoch().count();
    eng.seed(seed);
}

Random::Random(unsigned seed) {
    eng.seed(seed);
}