#include "vec3.h"
#include "ray.h"
#include "color.h"
#include "color32.h"

#include <iostream>
#include <SDL.h>
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

auto origin = point3(0, 0, 0);
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

color32 ray_color(const ray& r) {
	// Test for sphere intersection
	auto t = hit_sphere(point3(0, 0, -1), 0.5, r);
	if (t > 0.0) {
		// Hit sphere, return surface normal
		vec3 n = unit_vector(r.at(t) - vec3(0, 0, -1));
		//return 0.5 * color(n.x() + 1, n.y() + 1, n.z() + 1);
		return color32(static_cast<uint8_t>(255.999 * (n.x() + 1) * 0.5),
		               static_cast<uint8_t>(255.999 * (n.y() + 1) * 0.5),
		               static_cast<uint8_t>(255.999 * (n.z() + 1) * 0.5),
		               SDL_ALPHA_OPAQUE);
	}

	// Miss, return sky gradient
	vec3 unit_direction = unit_vector(r.direction());
	t = 0.5 * (unit_direction.y() + 1.0);
	//return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0);
	return (1.0 - t) * color32(255, 255, 255, 255) + t * color32(128, 200, 255, 255);
}

void render_pixels(std::vector<uint8_t> &pixels, int start_index, int pixels_to_render) {

	for (int i = start_index; i < start_index + pixels_to_render; i++)
	{
		int x = i % image_width;
		int y = floor(i / image_width);
		auto u = double(x) / (image_width - 1);
		auto v = double((image_height - 1) - y) / (image_height - 1);
		ray r(origin, lower_left_corner + u * horizontal + v * vertical - origin);
		//image_buffer[x][y] = ray_color(r);
		color32 c = ray_color(r);
		const unsigned int offset = (image_width * 4 * y) + x * 4;
		pixels[offset + 0] = c.r();
		pixels[offset + 1] = c.g();
		pixels[offset + 2] = c.b();
		pixels[offset + 3] = c.a();
	}
}

int main(int argc, char** args) {
	const int batch_size = image_width * 10; // Pixels to render per thread
	//auto image_buffer = new color32[image_width][image_height];
	std::vector<std::thread> threads;
	std::vector<uint8_t> pixels(image_width * image_height * 4, 0);

	SDL_Event ev;
	bool running = true;

	// Initialize SDL systems, window, window surface, renderer, texture
	SDL_Surface* win_surface = NULL;
	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;
	SDL_Texture* texture = NULL;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0) {
		std::cout << "Error initializing SDL: " << SDL_GetError() << std::endl;
		system("pause");
		return 1;
	}

	window = SDL_CreateWindow("toytracer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, image_width, image_height, SDL_WINDOW_SHOWN);
	if (!window) {
		std::cout << "Error creating window: " << SDL_GetError() << std::endl;
		system("pause");
		return 1;
	}

	win_surface = SDL_GetWindowSurface(window);
	if (!win_surface) {
		std::cout << "Error getting surface: " << SDL_GetError() << std::endl;
		system("pause");
		return 1;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		std::cout << "Error creating renderer: " << SDL_GetError() << std::endl;
		system("pause");
		return 1;
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, image_width, image_height);
	if (!texture) {
		std::cout << "Error creating texture: " << SDL_GetError() << std::endl;
		system("pause");
		return 1;
	}

	// Main rendering loop
	while (running) {
		// Handle input events
		while (SDL_PollEvent(&ev) != 0) {
			switch (ev.type) {
				case SDL_QUIT:
					running = false;
					break;
			}
		}

		origin += point3(0.01, 0, 0);

		// Render pixel colors into array
		for (int i = 0; i < pixel_count / batch_size; i++) {
			// Create thread for batch of pixels to render
			threads.push_back(std::thread(render_pixels, std::ref(pixels), i * batch_size, batch_size));
		}

		for (auto& th : threads) {
			th.join();
		}

		threads.clear();

		/*
		for (unsigned int i = 0; i < 1000; i++)
		{
			const unsigned int x = rand() % image_width;
			const unsigned int y = rand() % image_height;

			const unsigned int offset = (image_width * 4 * y) + x * 4;
			pixels[offset + 0] = rand() % 256;        // b
			pixels[offset + 1] = rand() % 256;        // g
			pixels[offset + 2] = rand() % 256;        // r
			pixels[offset + 3] = SDL_ALPHA_OPAQUE;    // a
		}
		*/

		SDL_UpdateTexture(texture, NULL, pixels.data(), image_width * 4);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		// Put rendered pixels to surface
		//std::cerr << win_surface->format->BytesPerPixel << std::endl;

		SDL_Delay(10);
	}

	/*
	// Write out rendered pixels to PPM image file
	std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

	// Iterate over output pixels, left to right, top to bottom
	for (int j = image_height - 1; j >= 0; --j) {
		std::cerr << "\rScanlines to be written: " << j << ' ' << std::flush;
		for (int i = 0; i < image_width; ++i) {
			write_color(std::cout, image_buffer[i][j]);
		}
	}
	*/

	// Clean up SDL
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	// Release image buffer memory
	/*
	for (int i = 0; i < image_width; ++i) {
		delete[] image_buffer[i];
	}
	delete[] image_buffer;
	*/

	return 0;
}
