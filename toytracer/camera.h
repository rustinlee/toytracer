#pragma once

#include "toytracer.h"

class camera {
	public:
		camera(point3 position, vec3 forward, double vfov, double aspect_ratio) {
			auto theta = degrees_to_radians(vfov);
			auto h = tan(theta / 2.0);
			viewport_height = 2.0 * h;
			viewport_width = aspect_ratio * viewport_height;

			this->forward = unit_vector(forward);

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

		void move(vec3 direction, bool local = true) {
			point3 new_position = origin;
			if (local) {
				new_position += (right * direction.x() + up * direction.y() + forward * direction.z());
			} else {
				new_position += direction;
			}
			set_origin(new_position);
		}

		void rotate(double x, double y) {
			forward = unit_vector(forward + right * x + up * y);
			recalculate();
		}

	private:
		point3 origin;
		vec3 forward;
		vec3 right;
		vec3 up;

		point3 lower_left_corner;
		vec3 horizontal;
		vec3 vertical;

		double aspect_ratio = 16.0 / 9.0;
		double viewport_height;
		double viewport_width;
		double focal_length = 1.0;

		void recalculate() {
			right = unit_vector(cross(vec3(0, 1, 0), forward));
			up = cross(forward, right);

			horizontal = viewport_width * right;
			vertical = viewport_height * up;
			lower_left_corner = origin - horizontal / 2 - vertical / 2 - forward;
		}
};
