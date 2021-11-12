#define SDL_MAIN_HANDLED
#include "SDL.h"

#include "toytracer.h"

#include "color.h"
#include "color32.h"
#include "hittable_list.h"
#include "sphere.h"
#include "camera.h"

#include <iostream>
#include <string>
#include <vector>
#include <thread>

// Image
const auto aspect_ratio = 16.0 / 9.0;
const int image_width = 640;
const int image_height = 360; // static_cast<int>(image_width / aspect_ratio);
const int pixel_count = image_width * image_height;
int samples_per_pixel = 4;
const int max_bounces = 8;

// Scene
hittable_list scene;

// Camera
camera cam;

// Debug visualizations
bool render_normals;

color ray_color(const ray& r, int depth) {
	if (depth >= max_bounces)
		return color(0, 0, 0);

	// Test for scene intersections
	hit_result result;
	if (scene.hit(r, 0.001, infinity, result)) {
		if (render_normals) {
			// Hit, return surface normal
			return 0.5 * color(result.normal.x() + 1,
			                   result.normal.y() + 1,
			                   result.normal.z() + 1);
		} else {
			point3 bounce_direction = result.p + result.normal + random_unit_vector();
			return 0.5 * ray_color(ray(result.p, bounce_direction - result.p), depth + 1);
		}
	}

	// Miss, return sky gradient
	vec3 unit_direction = unit_vector(r.direction());
	auto t = 0.5 * (unit_direction.y() + 1.0);
	return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0);
}

void render_pixels(std::vector<uint8_t> &pixels, int start_index, int pixels_to_render) {

	for (int i = start_index; i < start_index + pixels_to_render; i++)
	{
		int x = i % image_width;
		int y = floor(i / image_width);

		color color_sum(0, 0, 0);
		for (int s = 0; s < samples_per_pixel; ++s) {
			auto u = (double(x) + random_double()) / (image_width - 1);
			auto v = (double((image_height - 1) - y) + random_double()) / (image_height - 1);
			ray r = cam.get_ray(u, v);
			color_sum += ray_color(r, 0);
		}
		
		// Divide sum by number of samples, perform gamma correction, and write final pixel value
		auto scale = 1.0 / samples_per_pixel;
		const unsigned int offset = (image_width * 4 * y) + x * 4;
		pixels[offset + 0] = static_cast<uint8_t>(255.999 * sqrt(color_sum.x() * scale));
		pixels[offset + 1] = static_cast<uint8_t>(255.999 * sqrt(color_sum.y() * scale));
		pixels[offset + 2] = static_cast<uint8_t>(255.999 * sqrt(color_sum.z() * scale));
		pixels[offset + 3] = SDL_ALPHA_OPAQUE;
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
		uint64_t start = SDL_GetPerformanceCounter();

		// Handle input events
		while (SDL_PollEvent(&ev) != 0) {
			switch (ev.type) {
				case SDL_QUIT:
					running = false;
					break;
				case SDL_KEYDOWN:
					// N key - Toggle rendering normals
					if (ev.key.keysym.sym == SDLK_n)
						render_normals = !render_normals;
					// Number keys - Set samples per pixel
					if (ev.key.keysym.sym == SDLK_1)
						samples_per_pixel = 1 << 0;
					if (ev.key.keysym.sym == SDLK_2)
						samples_per_pixel = 1 << 1;
					if (ev.key.keysym.sym == SDLK_3)
						samples_per_pixel = 1 << 2;
					if (ev.key.keysym.sym == SDLK_4)
						samples_per_pixel = 1 << 3;
					if (ev.key.keysym.sym == SDLK_5)
						samples_per_pixel = 1 << 4;
					if (ev.key.keysym.sym == SDLK_6)
						samples_per_pixel = 1 << 5;
					if (ev.key.keysym.sym == SDLK_7)
						samples_per_pixel = 1 << 6;
					if (ev.key.keysym.sym == SDLK_8)
						samples_per_pixel = 1 << 7;
					if (ev.key.keysym.sym == SDLK_9)
						samples_per_pixel = 1 << 8;
					break;
			}
		}

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

		uint64_t end = SDL_GetPerformanceCounter();
		float elapsed = (end - start) / (float)SDL_GetPerformanceFrequency();
		char title[16];
		sprintf_s(title, "%f", 1.0f / elapsed);
		SDL_SetWindowTitle(window, title);
	}

	// Clean up SDL
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
