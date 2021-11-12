#pragma once

#include "hittable.h"
#include "vec3.h"

class sphere : public hittable {
	public:
		sphere() {}
		sphere(point3 cen, double r) : center(cen), radius(r) {};

		virtual bool hit(const ray& r, double t_min, double t_max, hit_result& result) const override;

public:
	point3 center;
	double radius;
};

bool sphere::hit(const ray& r, double t_min, double t_max, hit_result& result) const {
	vec3 oc = r.origin() - center;
	auto a = r.direction().length_squared();
	auto half_b = dot(oc, r.direction());
	auto c = oc.length_squared() - radius * radius;

	auto discriminant = half_b * half_b - a * c;
	if (discriminant < 0) return false;
	auto sqrtd = sqrt(discriminant);

	auto root = (-half_b - sqrtd) / a;
	if (root < t_min || t_max < root) {
		root = (-half_b + sqrtd) / a;
		if (root < t_min || t_max < root)
			return false;
	}

	result.t = root;
	result.p = r.at(root);
	vec3 outward_normal = (result.p - center) / radius;
	result.set_face_normal(r, outward_normal);

	return true;
}