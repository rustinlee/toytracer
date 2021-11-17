#define SDL_MAIN_HANDLED
#include "SDL.h"

#include "toytracer.h"

#include "color.h"
#include "color32.h"
#include "hittable_list.h"
#include "sphere.h"
#include "camera.h"
#include "material.h"

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <future>
#include <chrono>

using std::vector;
using std::thread;
using std::future;

// Image
const auto aspect_ratio = 16.0 / 9.0;
const int image_width = 640*2;
const int image_height = 360*2; // static_cast<int>(image_width / aspect_ratio);
const int pixel_count = image_width * image_height;
const int max_bounces = 8;

// Performance
const int batch_size = image_width * 5; // Pixels to render per thread
const int batch_count = 32;

// Scene
hittable_list scene;

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
			ray scattered;
			color attenuation;
			if (result.mat_ptr->scatter(r, result, attenuation, scattered))
				return attenuation * ray_color(scattered, depth + 1);
			return color(0, 0, 0);
		}
	}

	// Miss, return sky gradient
	vec3 unit_direction = unit_vector(r.direction());
	auto t = 0.5 * (unit_direction.y() + 1.0);
	return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0);
}

void render_pixels(camera cam, color color_sums[], uint32_t pixel_sample_counts[], std::vector<uint8_t>& pixels, int start_index, int pixels_to_render) {
	start_index %= (image_height * image_width);
	for (int i = start_index; i < start_index + pixels_to_render; i++) {
		int x = i % image_width;
		int y = floor(i / image_width);

		auto u = (double(x) + random_double()) / (image_width - 1);
		auto v = (double((image_height - 1) - y) + random_double()) / (image_height - 1);
		ray r = cam.get_ray(u, v);
		color_sums[i] += ray_color(r, 0);
		pixel_sample_counts[i] += 1;
		
		// Divide sum by number of samples, perform gamma correction, and write final pixel value
		auto scale = 1.0 / double(pixel_sample_counts[i]);
		const unsigned int offset = (image_width * 4 * y) + x * 4;
		pixels[offset + 0] = static_cast<uint8_t>(255.999 * sqrt(color_sums[i].x() * scale));
		pixels[offset + 1] = static_cast<uint8_t>(255.999 * sqrt(color_sums[i].y() * scale));
		pixels[offset + 2] = static_cast<uint8_t>(255.999 * sqrt(color_sums[i].z() * scale));
		pixels[offset + 3] = SDL_ALPHA_OPAQUE;
	}
}

void wait_for_render_threads(future<void> render_futures[]) {
	for (int i = 0; i < batch_count; i++) {
		if (render_futures[i].valid()) {
			render_futures[i].get();
		}
	}
}

void clear_color_sums(color color_sums[], uint32_t pixel_sample_counts[]) {
	for (int y = 0; y < image_height; y++) {
		for (int x = 0; x < image_width; x++) {
			auto i = x + y * image_width;
			color_sums[i] = color(0, 0, 0);
			pixel_sample_counts[i] = 0;
		}
	}
}

int main(int argc, char** args) {
	bool image_buffer_dirty = false;

	future<void> render_futures[batch_count] = {};
	vector<uint8_t> pixels(image_width * image_height * 4, 0);
	color* color_sums = new color[image_width * image_height];
	uint32_t* pixel_sample_counts = new uint32_t[image_width * image_height];

	int batches_dispatched = 0;
	clear_color_sums(color_sums, pixel_sample_counts);

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
	auto material_ground = make_shared<lambertian>(color(0.8, 0.8, 0.0));
	auto material_center = make_shared<lambertian>(color(0.7, 0.3, 0.3));
	auto material_left   = make_shared<metal>(color(0.8, 0.8, 0.8), 0.3);
	auto material_right  = make_shared<metal>(color(0.8, 0.6, 0.2), 1.0);

	scene.add(make_shared<sphere>(point3( 0.0, -100.5, -1.0), 100.0, material_ground));
	scene.add(make_shared<sphere>(point3( 0.0,    0.0, -1.0),   0.5, material_center));
	scene.add(make_shared<sphere>(point3(-1.0,    0.0, -1.0),   0.5, material_left));
	scene.add(make_shared<sphere>(point3( 1.0,    0.0, -1.0),   0.5, material_right));

	// Camera
	camera cam = camera(vec3(0, 1, -2), vec3(0, -1, 1), 90.0, aspect_ratio);

	// Initialize render futures array
	for (int i = 0; i < batch_count; i++) {
		render_futures[i] = std::async(std::launch::async, render_pixels, cam, std::ref(color_sums), std::ref(pixel_sample_counts), std::ref(pixels), batches_dispatched * batch_size, batch_size);
		batches_dispatched++;
	}

	// Main rendering loop
	while (running) {
		uint64_t start = SDL_GetPerformanceCounter();

		// If image buffer is dirty, clear pixel arrays and wait for all old render threads to complete
		// TODO: we should interrupt threads that are no longer relevant instead of having to wait for them to finish
		if (image_buffer_dirty) {
			wait_for_render_threads(render_futures);
			clear_color_sums(color_sums, pixel_sample_counts);
			batches_dispatched = 0;
			image_buffer_dirty = false;
		}

		// Render pixel colors into array
		for (int i = 0; i < batch_count; i++) {
			if (render_futures[i].valid()) {
				// Check if each render thread has completed; if so, launch another in its place
				const auto fs = render_futures[i].wait_for(std::chrono::seconds(0));
				if (fs == std::future_status::ready) {
					render_futures[i].get();
					render_futures[i] = std::async(std::launch::async, render_pixels, cam, std::ref(color_sums), std::ref(pixel_sample_counts), std::ref(pixels), batches_dispatched * batch_size, batch_size);
					batches_dispatched++;
				}
			} else {
				render_futures[i] = std::async(std::launch::async, render_pixels, cam, std::ref(color_sums), std::ref(pixel_sample_counts), std::ref(pixels), batches_dispatched * batch_size, batch_size);
				batches_dispatched++;
			}
		}

		// Push pixels to window surface
		SDL_UpdateTexture(texture, NULL, pixels.data(), image_width * 4);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		uint64_t end = SDL_GetPerformanceCounter();
		float delta = (end - start) / (float)SDL_GetPerformanceFrequency();

		// Handle input events
		while (SDL_PollEvent(&ev) != 0) {
			switch (ev.type) {
				case SDL_QUIT:
					running = false;
					break;
				case SDL_KEYDOWN:
					// N key - Toggle rendering normals
					if (ev.key.keysym.sym == SDLK_n) {
						render_normals = !render_normals;
						image_buffer_dirty = true;
					}
					// WASD keys - Move camera origin
					if (ev.key.keysym.sym == SDLK_w) {
						cam.set_origin(cam.get_origin() + point3(0, 0, -5) * delta);
						image_buffer_dirty = true;
					}
					if (ev.key.keysym.sym == SDLK_a) {
						cam.set_origin(cam.get_origin() + point3(-5, 0, 0) * delta);
						image_buffer_dirty = true;
					}
					if (ev.key.keysym.sym == SDLK_s) {
						cam.set_origin(cam.get_origin() + point3(0, 0, 5) * delta);
						image_buffer_dirty = true;
					}
					if (ev.key.keysym.sym == SDLK_d) {
						cam.set_origin(cam.get_origin() + point3(5, 0, 0) * delta);
						image_buffer_dirty = true;
					}
					break;
			}
		}

		char title[16];
		sprintf_s(title, "%f", 1.0f / delta);
		SDL_SetWindowTitle(window, title);
	}

	// Clean up SDL
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	delete[] color_sums;
	delete[] pixel_sample_counts;

	return 0;
}
