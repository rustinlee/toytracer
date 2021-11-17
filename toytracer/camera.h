#pragma once

#include "toytracer.h"

class camera {
	public:
		camera(point3 position, vec3 forward, double vfov, double aspect_ratio) {
			auto theta = degrees_to_radians(vfov);
			auto h = tan(theta / 2.0);
			viewport_height = 2.0 * h;
			viewport_width = aspect_ratio * viewport_height;

			look_direction = -unit_vector(forward);

			origin = position;
			recalculate();
		}

		ray get_ray(double u, double v) const {
			return ray(origin, lower_left_corner + u * horizontal + v * vertical - origin);
		}

		point3 get_origin() {
			return origin;
		}

		void set_origin(point3 position) {
			origin = position;
			recalculate();
		}

	private:
		point3 origin;
		vec3 look_direction;
		point3 lower_left_corner;
		vec3 horizontal;
		vec3 vertical;
		double aspect_ratio = 16.0 / 9.0;
		double viewport_height;
		double viewport_width;
		double focal_length = 1.0;

		void recalculate() {
			vec3 u = unit_vector(cross(vec3(0, 1, 0), look_direction));
			vec3 v = cross(look_direction, u);

			horizontal = viewport_width * u;
			vertical = viewport_height * v;
			lower_left_corner = origin - horizontal / 2 - vertical / 2 - look_direction;
		}
};
