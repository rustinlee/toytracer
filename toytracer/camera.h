#pragma once

#include "toytracer.h"

class camera {
	public:
		camera() {
			origin = point3(0, 0, 0);
			horizontal = vec3(viewport_width, 0.0, 0.0);
			vertical = vec3(0.0, viewport_height, 0.0);
			lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3(0, 0, focal_length);
		}

		ray get_ray(double u, double v) const {
			return ray(origin, lower_left_corner + u * horizontal + v * vertical - origin);
		}

		void set_origin(point3 new_origin) {
			origin = new_origin;
			horizontal = vec3(viewport_width, 0.0, 0.0);
			vertical = vec3(0.0, viewport_height, 0.0);
			lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3(0, 0, focal_length);
		}

	private:
		point3 origin;
		point3 lower_left_corner;
		vec3 horizontal;
		vec3 vertical;
		double aspect_ratio = 16.0 / 9.0;
		double viewport_height = 2.0;
		double viewport_width = aspect_ratio * viewport_height;
		double focal_length = 1.0;
};
