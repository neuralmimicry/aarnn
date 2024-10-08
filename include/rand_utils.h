#ifndef RAND_UTILS_H_INCLUDED
#define RAND_UTILS_H_INCLUDED

#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>

namespace ns_aarnn
{
inline double rand_val(double min, double max)
{
    if(max < min)
        std::swap(max, min);
    std::random_device r;

    std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
    std::mt19937 engine(seed);
    std::uniform_real_distribution<double> uni_dist{min, max};

    return uni_dist(engine);
}
}  // namespace aarnn

#endif  // RAND_UTILS_H_INCLUDED
