#define SDL_MAIN_HANDLED
#include "SDL.h"

#include "toytracer.h"

#include "color.h"
#include "color32.h"
#include "hittable_list.h"
#include "sphere.h"

#include <iostream>
#include <vector>
#include <thread>

// Image
const auto aspect_ratio = 16.0 / 9.0;
const int image_width = 640;
const int image_height = 360; // static_cast<int>(image_width / aspect_ratio);
const int pixel_count = image_width * image_height;

// Scene
hittable_list scene;

// Camera
const auto viewport_height = 2.0;
const auto viewport_width = aspect_ratio * viewport_height;
const auto focal_length = 1.0;

auto origin = point3(0, 0, 0);
const auto horizontal = vec3(viewport_width, 0, 0);
const auto vertical = vec3(0, viewport_height, 0);

color32 ray_color(const ray& r) {
	// Test for scene intersections
	hit_result result;
	if (scene.hit(r, 0, infinity, result)) {
		// Hit, return surface normal
		return color32(static_cast<uint8_t>(255.999 * (result.normal.x() + 1) * 0.5),
		               static_cast<uint8_t>(255.999 * (result.normal.y() + 1) * 0.5),
		               static_cast<uint8_t>(255.999 * (result.normal.z() + 1) * 0.5),
		               SDL_ALPHA_OPAQUE);
	}

	// Miss, return sky gradient
	vec3 unit_direction = unit_vector(r.direction());
	auto t = 0.5 * (unit_direction.y() + 1.0);
	return (1.0 - t) * color32(255, 255, 255, 255) + t * color32(128, 200, 255, 255);
}

void render_pixels(std::vector<uint8_t> &pixels, int start_index, int pixels_to_render) {

	for (int i = start_index; i < start_index + pixels_to_render; i++)
	{
		int x = i % image_width;
		int y = floor(i / image_width);
		auto u = double(x) / (image_width - 1);
		auto v = double((image_height - 1) - y) / (image_height - 1);
		const auto lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3(0, 0, focal_length);
		ray r(origin, lower_left_corner + u * horizontal + v * vertical - origin);
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
	std::vector<std::thread> threads;
	std::vector<uint8_t> pixels(image_width * image_height * 4, 0);

	SDL_Event ev;
	bool running = true;

	// Initialize SDL systems, window, window surface, renderer, texture
	SDL_Surface* win_surface = NULL;
	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;
	SDL_Texture* texture = NULL;

	SDL_SetMainReady();
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

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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

	// Scene definition
	scene.add(make_shared<sphere>(point3(0, 0, -1), 0.5));
	scene.add(make_shared<sphere>(point3(0, -100.5, -1), 100));

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

		// Push pixels to window surface
		SDL_UpdateTexture(texture, NULL, pixels.data(), image_width * 4);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
	}

	// Clean up SDL
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
