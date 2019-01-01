#ifndef INC_3PC_RANDOM_H
#define INC_3PC_RANDOM_H

#include <random>

class Random {
private:
    std::mt19937 eng;

public:
    Random();

    explicit Random(unsigned seed);

    template<typename T>
    T randomBetween(T begin, T end) {
        static_assert(std::is_arithmetic<T>::value, "Arguments must me integer or floating-point types");
        if (std::is_integral<T>::value) {
            std::uniform_int_distribution<T> range(begin, end);
            return range(eng);
        } else {
            std::uniform_int_distribution<T> range(begin, end);
            return range(eng);
        }
    }
};

#endif //INC_3PC_RANDOM_H
