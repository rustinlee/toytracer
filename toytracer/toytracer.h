#pragma once

#include <cmath>
#include <limits>
#include <memory>
#include <random>

using std::shared_ptr;
using std::make_shared;
using std::sqrt;

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

inline double degrees_to_radians(double degrees) {
	return degrees * pi / 180.0;
}

inline double random_double() {
	// Returns a random real in [0,1)
	uint32_t a;
	_rdrand32_step(&a);
	return a / double(_UI32_MAX);
}

inline double random_double(double min, double max) {
	// Returns a random real in [min,max)
	return min + (max - min) * random_double();
}

#include "ray.h"
#include "vec3.h"
