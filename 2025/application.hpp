#pragma once

#include "../AA/include/AA/metaprogramming/general.hpp"
#include "../AA/include/AA/container/constified.hpp"
#include "../AA/include/AA/container/managed.hpp"
#include "../AA/include/AA/container/fixed_vector.hpp"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <source_location>
#include <cstdio>
#include <format>
#include <chrono>



enum struct error_kind : size_t {
	SDL,
	bad_argv
};

template<error_kind ERROR = error_kind::SDL>
constexpr bool E(const bool cond, const std::source_location l = std::source_location::current()) {
	if (cond) return false;
	else {
		if constexpr (ERROR == error_kind::bad_argv) {
			SDL_SetError("Wrong arguments provided to program");
		}
		SDL_LogError(SDL_LogCategory::SDL_LOG_CATEGORY_APPLICATION,
			"%s:%u:%u '%s': %s", l.file_name(), l.line(), l.column(), l.function_name(), SDL_GetError());
		return true;
	}
}

constexpr SDL_Rect get_text_bbox(TTF_Font * const font, const std::string_view text) {
	int text_w = 0, left = 0, top = aa::numeric_min, bottom = aa::numeric_max;
	const char * pstr = text.data();

	while (const uint32_t codepoint = SDL_StepUTF8(&pstr, nullptr)) {
		const auto [min_x, max_x, min_y, max_y, advance] = aa::make_tuple([&]<size_t... I>(aa::quintet<int> &t) -> void {
			E(TTF_GetGlyphMetrics(font, codepoint, &t[aa::c<I>]...));
		});
		if (!text_w) {
			text_w += advance - min_x;
			left = min_x;
		} else if (pstr == text.end()) {
			text_w += max_x;
		} else {
			text_w += advance;
		}
		bottom = std::ranges::min(bottom, min_y);
		top = std::ranges::max(top, max_y);
	}

	return {left, TTF_GetFontAscent(font) - top, text_w, top - bottom};
}

constexpr void log(void * const, const int, const SDL_LogPriority, const char * const message) {
	// Nenaudojame fputs ir fputc, nes reikia vienu iškvietimu spausdinti.
	std::puts(message);
}

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
			std::freopen("SDL_Log.txt", "wb", stdout);
			SDL_SetLogOutputFunction(log, nullptr);
			E<error_kind::bad_argv>(argc == 1);


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
			const SDL_Rect bbox = get_text_bbox(font, display_text);

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

				if (std::exchange(should_take_screenshot, false)) {
					// https://stackoverflow.com/questions/76106361/stdformating-stdchrono-seconds-without-fractional-digits
					auto [out, _] = std::format_to_n(
						const_cast<char *>(screenshot_name.end()), aa::sign(screenshot_name.space() - screenshot_extension.size()),
						"img_{0:%Y}-{0:%m}-{0:%d}_{0:%H}'{0:%M}'{0:%S}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
					std::ranges::copy(screenshot_extension, out);

					// https://gigi.nullneuron.net/gigilabs/saving-screenshots-in-sdl2/
					const aa::managed<SDL_Surface *, SDL_DestroySurface> surface = SDL_RenderReadPixels(renderer, nullptr);
					if (!E(surface)) {
						E(SDL_SaveBMP(surface, std::as_const(screenshot_name).data()));
					}
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
