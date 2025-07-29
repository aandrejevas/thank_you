#pragma once

#include "../AA/include/AA/metaprogramming/general.hpp"
#include "../AA/include/AA/container/constified.hpp"
#include "../AA/include/AA/container/managed.hpp"
#include "../AA/include/AA/container/fixed_vector.hpp"
#include "utils.hpp"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstdio>
#include <format>
#include <print>
#include <source_location>



namespace {
	using namespace std::literals;

	struct application {
		// Member objects
	private:
		static constexpr std::string_view title = "Thank you 2025"sv;
		static constexpr std::string_view display_text = "Ačiū"sv;
		static constexpr std::string_view output_dir = "output/"sv;
		static constexpr std::string_view screenshot_extension = ".bmp\0"sv;

		// Objects destroyed in reverse order of declaration.
		aa::managed<std::FILE *, std::fclose> log_file;
		aa::shallowly_managed<SDL_Window *> window;
		aa::shallowly_managed<SDL_Renderer *> renderer;
		aa::managed<TTF_Font *, TTF_CloseFont> font;
		aa::managed<SDL_Surface *, SDL_DestroySurface> surface;
		aa::managed<SDL_Texture *, SDL_DestroyTexture> texture;
		aa::managed<SDL_Semaphore *, SDL_DestroySemaphore> sem_block_thread;
		aa::shallowly_managed<SDL_Thread *> worker_thread;

		aa::constified<SDL_FRect, aa::default_value, [](const SDL_FRect & r) static -> bool {
			return std::bit_cast<std::array<float, 4>>(r) == aa::default_value;
		}> text_dst;

		bool should_draw = true, should_take_screenshot = false, is_working = true;

		aa::fixed_vector<char> screenshot_name = {100, output_dir};



		// Member functions
		enum struct error_kind : size_t {
			SDL,
			log_file,
			bad_argv
		};

		template<error_kind ERROR = error_kind::SDL>
		constexpr bool E(const bool cond,
			const int category = SDL_LogCategory::SDL_LOG_CATEGORY_APPLICATION,
			const SDL_LogPriority priority = SDL_LogPriority::SDL_LOG_PRIORITY_ERROR,
			const char * const msg = nullptr,
			const std::source_location l = std::source_location::current()) const &
		{
			if (cond) return false;
			else {
				if constexpr (ERROR == error_kind::bad_argv) {
					SDL_SetError("%s", "Wrong parameters provided to program");
				} else if constexpr (ERROR == error_kind::log_file) {
					SDL_SetError("%s", "Could not open log file");
				}
				std::println(log_file ? log_file.get() : stderr, "{}:{}:{} '{}': {}. Category: {}. Priority: {}.",
					l.file_name(), l.line(), l.column(), l.function_name(), msg ? msg : SDL_GetError(),
					aa::unsign(category), aa::unsign(priority));
				return true;
			}
		}

		constexpr int work() & {
			SDL_Event event = {.type = SDL_RegisterEvents(1)};

			do {
				SDL_Delay(1000);


				E(SDL_PushEvent(&event));
				SDL_WaitSemaphore(sem_block_thread);
			} while (is_working);

			return EXIT_SUCCESS;
		}

	public:
		constexpr SDL_AppResult init(const int argc, const char * const * const) & {
			// Negalime naudoti SDL numatytos funkcijos, nes ji labai neoptimali ir neišvengiamai spausdina \r\n.
			// Negalime rašyti į failo galą, nes tada reiktų failo valymo strategijos.
			E<error_kind::log_file>(log_file = std::fopen("SDL_Log.txt", "wb"));
			E<error_kind::bad_argv>(argc == 1);

			SDL_SetLogOutputFunction([](void * const appstate, const int category, const SDL_LogPriority priority, const char * const message) static -> void {
				std::bit_cast<application *>(appstate)->E(false, category, priority, message);
			}, this);


			E(SDL_SetHint(SDL_HINT_RENDER_DRIVER, "vulkan"));
			E(SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "waitevent"));

			E(SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, title.data()));
			E(SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, "1.0"));
			E(SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "Aleksandras Andrejevas"));
			E(SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, "https://github.com/aandrejevas"));

			if (E(SDL_InitSubSystem(SDL_INIT_VIDEO)))	return SDL_AppResult::SDL_APP_FAILURE;
			if (E(TTF_Init()))							return SDL_AppResult::SDL_APP_FAILURE;

			if (E(SDL_CreateWindowAndRenderer(title.data(), 0, 0, SDL_WINDOW_FULLSCREEN, &window.acquire(), &renderer.acquire())))
				return SDL_AppResult::SDL_APP_FAILURE;

			if (E(font = TTF_OpenFont("C:\\Windows\\Fonts\\Ruler Stencil Heavy.ttf", 980)))
				return SDL_AppResult::SDL_APP_FAILURE;

			if (E(surface = TTF_RenderText_Solid(font, display_text.data(), display_text.size(), {255, 255, 255, SDL_ALPHA_OPAQUE})))
				return SDL_AppResult::SDL_APP_FAILURE;

			if (E(texture = SDL_CreateTextureFromSurface(renderer, surface)))
				return SDL_AppResult::SDL_APP_FAILURE;


			const auto [window_w, window_h] = aa::make_tuple([&]<size_t... I>(aa::pair<int> &t) -> void {
				E(SDL_GetWindowSizeInPixels(window, &t[aa::c<I>]...));
			});
			const std::optional o_bbox = get_text_bbox(font, display_text);
			if (E(o_bbox.has_value())) return SDL_AppResult::SDL_APP_FAILURE;
			const SDL_Rect & bbox = *o_bbox;

			text_dst = {
				aa::cast<float>((window_w - bbox.w) / 2 - bbox.x),
				aa::cast<float>((window_h - bbox.h) / 2 - bbox.y),
				aa::cast<float>(texture->w), aa::cast<float>(texture->h)
			};

			if (E(sem_block_thread = SDL_CreateSemaphore(0)))
				return SDL_AppResult::SDL_APP_FAILURE;

			if (E(worker_thread = SDL_CreateThread([](void * const appstate) static -> int {
				return std::bit_cast<application *>(appstate)->work();
			}, "worker_thread", this)))
				return SDL_AppResult::SDL_APP_FAILURE;


			E(SDL_CreateDirectory(output_dir.data()));

			// Reikia šito kitaip teksto nenupiešia, nupiešia tik foną.
			E(SDL_RenderPresent(renderer));

			return SDL_AppResult::SDL_APP_CONTINUE;
		}

		constexpr SDL_AppResult event(const SDL_Event & event) & {
			switch (event.type) {
			case SDL_EventType::SDL_EVENT_QUIT:
				return SDL_AppResult::SDL_APP_SUCCESS;

			case SDL_EventType::SDL_EVENT_KEY_DOWN:
				switch (event.key.key) {
				case SDLK_ESCAPE:
					return SDL_AppResult::SDL_APP_SUCCESS;

				case SDLK_W:
					if (event.key.mod & SDL_KMOD_CTRL)
						return SDL_AppResult::SDL_APP_SUCCESS;
					break;

				case SDLK_S:
					if (!event.key.repeat && (event.key.mod & SDL_KMOD_CTRL)) {
						should_draw = true;
						should_take_screenshot = true;
					}
					break;

				case SDLK_R:
					if (!is_working && !event.key.repeat) {
						is_working = true;
						SDL_SignalSemaphore(sem_block_thread);
					}
					break;
				}
				break;

			case SDL_EventType::SDL_EVENT_USER:
				should_draw = true;
				is_working = false;
				break;
			}

			return SDL_AppResult::SDL_APP_CONTINUE;
		}

		constexpr SDL_AppResult iterate() & {
			if (std::exchange(should_draw, false)) {

				// Update state
				const double now = static_cast<double>(SDL_GetTicks()) / 1000.0;  /* convert from milliseconds to seconds. */
				/* choose the color for the frame we will draw. The sine wave trick makes it fade between colors smoothly. */
				const float red = static_cast<float>(0.5 + 0.5 * SDL_sin(now));
				const float green = static_cast<float>(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
				const float blue = static_cast<float>(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));

				// Draw
				E(SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT));
				E(SDL_RenderClear(renderer));

				E(SDL_RenderTexture(renderer, texture, nullptr, &text_dst));

				switch (std::exchange(should_take_screenshot, false)) {
				case true:
					const std::optional o_d = ([&] -> std::optional<SDL_Time> {
						if (SDL_Time current_time; SDL_GetCurrentTime(&current_time))
							return current_time; else return std::nullopt;
					})().and_then([&](const SDL_Time current_time) -> std::optional<SDL_DateTime> {
						if (SDL_DateTime current_date; SDL_TimeToDateTime(current_time, &current_date, true))
							return current_date; else return std::nullopt;
					});
					if (E(o_d.has_value())) break;
					const SDL_DateTime & d = *o_d;

					auto [out, _] = std::format_to_n(
						const_cast<char *>(screenshot_name.end()), aa::sign(screenshot_name.space() - screenshot_extension.size()),
						"img_{}-{:02}-{:02}_{:02}'{:02}'{:02}", d.year, d.month, d.day, d.hour, d.minute, d.second);
					std::ranges::copy(screenshot_extension, out);

					// https://gigi.nullneuron.net/gigilabs/saving-screenshots-in-sdl2/
					const aa::managed<SDL_Surface *, SDL_DestroySurface> surface = SDL_RenderReadPixels(renderer, nullptr);
					if (E(surface)) break;
					E(SDL_SaveBMP(surface, std::as_const(screenshot_name).data()));
				}

				E(SDL_RenderPresent(renderer));
			}
			return SDL_AppResult::SDL_APP_CONTINUE;
		}

		constexpr application * quit(const SDL_AppResult) & {
			is_working = false;
			SDL_SignalSemaphore(sem_block_thread);
			SDL_WaitThread(worker_thread, nullptr);

			return this;
		}
	};
}
