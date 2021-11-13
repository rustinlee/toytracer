#pragma once

#include "toytracer.h"
#include "hittable.h"

struct hit_result;

class material {
	public:
		virtual bool scatter(const ray& r_in, const hit_result& result, color& attenuation, ray& scattered) const = 0;
};

class lambertian : public material {
	public:
		lambertian(const color& a) : albedo(a) {}

		virtual bool scatter(const ray& r_in, const hit_result& result, color& attenuation, ray& scattered) const override {
			auto scatter_direction = result.normal + random_unit_vector();
			
			// Catch degenerate scatter direction
			if (scatter_direction.near_zero())
				scatter_direction = result.normal;

			scattered = ray(result.p, scatter_direction);
			attenuation = albedo;
			return true;
		}

	public:
		color albedo;
};

class metal : public material {
	public:
		metal(const color& a, double r) : albedo(a), roughness(r < 1 ? r : 1) {}

		virtual bool scatter(const ray& r_in, const hit_result& result, color& attenuation, ray& scattered) const override {
			vec3 reflected = reflect(unit_vector(r_in.direction()), result.normal);
			scattered = ray(result.p, reflected + roughness * random_in_unit_sphere());
			attenuation = albedo;
			return (dot(scattered.direction(), result.normal) > 0);
		}

	public:
		color albedo;
		double roughness;
};
