#include "vec3.h"
#include "ray.h"
#include "color.h"

#include <iostream>
#include <vector>
#include <thread>

// Image
const auto aspect_ratio = 16.0 / 9.0;
const int image_width = 640;
const int image_height = 360; // static_cast<int>(image_width / aspect_ratio);
const int pixel_count = image_width * image_height;

// Camera
const auto viewport_height = 2.0;
const auto viewport_width = aspect_ratio * viewport_height;
const auto focal_length = 1.0;

const auto origin = point3(0, 0, 0);
const auto horizontal = vec3(viewport_width, 0, 0);
const auto vertical = vec3(0, viewport_height, 0);
const auto lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3(0, 0, focal_length);

double hit_sphere(const point3& center, double radius, const ray& r) {
	vec3 oc = r.origin() - center;
	auto a = dot(r.direction(), r.direction());
	auto b = 2.0 * dot(oc, r.direction());
	auto c = dot(oc, oc) - radius * radius;
	auto discriminant = b * b - 4 * a * c;
	if (discriminant < 0) {
		// No real solutions, missed sphere
		return -1.0;
	} else {
		// 1 or 2 real solutions, return smallest t for ray r that gives a point on the sphere
		return (-b - sqrt(discriminant)) / (2.0 * a);
	}
}

color ray_color(const ray& r) {
	// Test for sphere intersection
	auto t = hit_sphere(point3(0, 0, -1), 0.5, r);
	if (t > 0.0) {
		// Hit sphere, return surface normal
		vec3 n = unit_vector(r.at(t) - vec3(0, 0, -1));
		return 0.5 * color(n.x() + 1, n.y() + 1, n.z() + 1);
	}

	// Miss, return sky gradient
	vec3 unit_direction = unit_vector(r.direction());
	t = 0.5 * (unit_direction.y() + 1.0);
	return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0);
}

void render_pixels(color image_buffer[][image_height], int start_index, int pixels_to_render) {

	for (int i = start_index; i < start_index + pixels_to_render; i++)
	{
		int x = i % image_width;
		int y = floor(i / image_width);
		auto u = double(x) / (image_width - 1);
		auto v = double(y) / (image_height - 1);
		ray r(origin, lower_left_corner + u * horizontal + v * vertical - origin);
		image_buffer[x][y] = ray_color(r);
	}
}

int main() {
	// Render pixel colors into array
	const int batch_size = 1024; // Pixels to render per thread
	auto image_buffer = new color[image_width][image_height];
	std::vector<std::thread> threads;
	for (int i = 0; i < pixel_count / batch_size; i++) {
		// Create thread for batch of pixels to render
		threads.push_back(std::thread(render_pixels, image_buffer, i * batch_size, batch_size));
	}

	for (auto& th : threads) {
		th.join();
	}

	// Write out rendered pixels to PPM image file
	std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

	// Iterate over output pixels, left to right, top to bottom
	for (int j = image_height - 1; j >= 0; --j) {
		//std::cerr << "\rScanlines to be written: " << j << ' ' << std::flush;
		for (int i = 0; i < image_width; ++i) {
			write_color(std::cout, image_buffer[i][j]);
		}
	}

	// Release image buffer memory
	for (int i = 0; i < image_height; ++i) {
		delete[] image_buffer[i];
	}
	delete[] image_buffer;

	std::cerr << "\nRender complete.\n";
}