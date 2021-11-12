#pragma once

#include <cstdint>

class color32 
{
	public:
		color32() : e{ 0,0,0 } {}
		color32(uint8_t e0, uint8_t e1, uint8_t e2, uint8_t e3) : e{ e0, e1, e2, e3 } {}

		uint8_t r() const { return e[0]; }
		uint8_t g() const { return e[1]; }
		uint8_t b() const { return e[2]; }
		uint8_t a() const { return e[3]; }

		color32& operator+=(const color32& v) {
			e[0] += v.e[0];
			e[1] += v.e[1];
			e[2] += v.e[2];
			e[3] += v.e[3];
			return *this;
		}

		color32& operator*=(const double t) {
			e[0] *= t;
			e[1] *= t;
			e[2] *= t;
			e[3] *= t;
			return *this;
		}

	public:
		uint8_t e[4];
};

inline color32 operator+(const color32& u, const color32& v) {
	return color32(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2], u.e[3] + v.e[3]);
}

inline color32 operator*(const double u, const color32& v) {
	return color32(u * v.e[0], u * v.e[1], u * v.e[2], u * v.e[3]);
}

inline color32 operator*(const color32& v, const double t) {
	return t * v;
}
