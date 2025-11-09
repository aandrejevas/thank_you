#pragma once

#include "../AA/include/AA/metaprogramming/general.hpp"
#include "../AA/include/AA/container/constified.hpp"
#include "../AA/include/AA/container/managed.hpp"
#include "../AA/include/AA/algorithm/arithmetic.hpp"
#include "../AA/include/AA/container/fixed_vector.hpp"
#include "utils.hpp"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>

#include <cstdio>
#include <format>



namespace {
	using namespace std::literals;

	struct application {
		// Member objects
	private:
		static constexpr std::string_view
			title = "Thank you 2025"sv,
			display_text = "Ačiū"sv,
			output_dir = "output/"sv,
			screenshot_extension = ".jpeg\0"sv;

		// Objects destroyed in reverse order of declaration.
		aa::managed<std::FILE *, std::fclose> log_file;
		aa::shallowly_managed<SDL_Window *> window;
		aa::shallowly_managed<SDL_Renderer *> renderer;
		aa::managed<TTF_Font *, TTF_CloseFont> font;
		aa::managed<SDL_Texture *, SDL_DestroyTexture> texture;
		aa::managed<SDL_Surface *, SDL_DestroySurface> is_text_srf;
		aa::shallowly_managed<uint32_t *> pixels;
		aa::managed<SDL_Semaphore *, SDL_DestroySemaphore> sem_block_thread;
		aa::shallowly_managed<SDL_Thread *> worker_thread;

		bool
			should_draw = true,
			should_take_screenshot = false,
			is_working = true;

		// Ne const, nes potencialiai gali pasikeisti.
		uint32_t width, height, pixel_count;

		aa::fixed_vector<char> screenshot_name;
		aa::fixed_vector<uint32_t> neighbors, good_neighbors;

		std::array<bool, aa::representable_values_v<aa::triplet<std::byte>>> color_used;

		// https://oeis.org/A005875
		static constexpr std::array num_of_ways = std::to_array<size_t>({
			1, 6, 12, 8, 6, 24, 24, 0, 12, 30, 24, 24, 8, 24, 48, 0, 6, 48, 36, 24, 24, 48, 24
		});
		// https://oeis.org/A000378
		static constexpr std::array sums_of_three_squares = std::to_array<uint32_t>({
			1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 16, 17, 18, 19, 20, 21, 22
		});
		std::array<uint32_t, num_of_ways[sums_of_three_squares[0]]> color_addends_0 = {
			0x01'00'00, 0xFF'00'00,
			0x00'01'00, 0x00'FF'00,
			0x00'00'01, 0x00'00'FF
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[1]]> color_addends_1 = {
			0x01'01'00, 0x01'FF'00, 0x01'00'01, 0x01'00'FF,
			0xFF'01'00, 0xFF'FF'00, 0xFF'00'01, 0xFF'00'FF,
			0x00'01'01, 0x00'01'FF, 0x00'FF'01, 0x00'FF'FF
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[2]]> color_addends_2 = {
			0x01'01'01, 0x01'FF'01, 0x01'01'FF, 0x01'FF'FF,
			0xFF'01'01, 0xFF'FF'01, 0xFF'01'FF, 0xFF'FF'FF
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[3]]> color_addends_3 = {
			0x02'00'00, 0xFE'00'00,
			0x00'02'00, 0x00'FE'00,
			0x00'00'02, 0x00'00'FE
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[4]]> color_addends_4 = {
			0x02'01'00, 0x02'FF'00, 0x02'00'01, 0x02'00'FF, 0x01'FE'00, 0xFF'FE'00, 0x00'FE'01, 0x00'FE'FF,
			0xFE'01'00, 0xFE'FF'00, 0xFE'00'01, 0xFE'00'FF, 0x01'00'02, 0xFF'00'02, 0x00'01'02, 0x00'FF'02,
			0x01'02'00, 0xFF'02'00, 0x00'02'01, 0x00'02'FF, 0x01'00'FE, 0xFF'00'FE, 0x00'01'FE, 0x00'FF'FE
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[5]]> color_addends_5 = {
			0x02'01'01, 0x02'FF'01, 0x02'01'FF, 0x02'FF'FF, 0x01'FE'01, 0xFF'FE'01, 0x01'FE'FF, 0xFF'FE'FF,
			0xFE'01'01, 0xFE'FF'01, 0xFE'01'FF, 0xFE'FF'FF, 0x01'01'02, 0xFF'01'02, 0x01'FF'02, 0xFF'FF'02,
			0x01'02'01, 0xFF'02'01, 0x01'02'FF, 0xFF'02'FF, 0x01'01'FE, 0xFF'01'FE, 0x01'FF'FE, 0xFF'FF'FE
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[6]]> color_addends_6 = {
			0x02'02'00, 0x02'FE'00, 0x02'00'02, 0x02'00'FE,
			0xFE'02'00, 0xFE'FE'00, 0xFE'00'02, 0xFE'00'FE,
			0x00'02'02, 0x00'FE'02, 0x00'02'FE, 0x00'FE'FE
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[7]]> color_addends_7 = {
			0x03'00'00, 0xFD'00'00,
			0x00'03'00, 0x00'FD'00,
			0x00'00'03, 0x00'00'FD,
			0x02'02'01, 0x02'FE'01, 0x02'01'02, 0x02'01'FE, 0x02'02'FF, 0x02'FE'FF, 0x02'FF'02, 0x02'FF'FE,
			0xFE'02'01, 0xFE'FE'01, 0xFE'01'02, 0xFE'01'FE, 0xFE'02'FF, 0xFE'FE'FF, 0xFE'FF'02, 0xFE'FF'FE,
			0x01'02'02, 0x01'FE'02, 0x01'02'FE, 0x01'FE'FE, 0xFF'02'02, 0xFF'FE'02, 0xFF'02'FE, 0xFF'FE'FE
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[8]]> color_addends_8 = {
			0x03'01'00, 0x03'FF'00, 0x03'00'01, 0x03'00'FF, 0x01'FD'00, 0xFF'FD'00, 0x00'FD'01, 0x00'FD'FF,
			0xFD'01'00, 0xFD'FF'00, 0xFD'00'01, 0xFD'00'FF, 0x01'00'03, 0xFF'00'03, 0x00'01'03, 0x00'FF'03,
			0x01'03'00, 0xFF'03'00, 0x00'03'01, 0x00'03'FF, 0x01'00'FD, 0xFF'00'FD, 0x00'01'FD, 0x00'FF'FD
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[9]]> color_addends_9 = {
			0x03'01'01, 0x03'FF'01, 0x03'01'FF, 0x03'FF'FF, 0x01'FD'01, 0xFF'FD'01, 0x01'FD'FF, 0xFF'FD'FF,
			0xFD'01'01, 0xFD'FF'01, 0xFD'01'FF, 0xFD'FF'FF, 0x01'01'03, 0xFF'01'03, 0x01'FF'03, 0xFF'FF'03,
			0x01'03'01, 0xFF'03'01, 0x01'03'FF, 0xFF'03'FF, 0x01'01'FD, 0xFF'01'FD, 0x01'FF'FD, 0xFF'FF'FD
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[10]]> color_addends_10 = {
			0x02'02'02, 0x02'FE'02, 0x02'02'FE, 0x02'FE'FE,
			0xFE'02'02, 0xFE'FE'02, 0xFE'02'FE, 0xFE'FE'FE,
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[11]]> color_addends_11 = {
			0x03'02'00, 0x03'FE'00, 0x03'00'02, 0x03'00'FE, 0x02'FD'00, 0xFE'FD'00, 0x00'FD'02, 0x00'FD'FE,
			0xFD'02'00, 0xFD'FE'00, 0xFD'00'02, 0xFD'00'FE, 0x02'00'03, 0xFE'00'03, 0x00'02'03, 0x00'FE'03,
			0x02'03'00, 0xFE'03'00, 0x00'03'02, 0x00'03'FE, 0x02'00'FD, 0xFE'00'FD, 0x00'02'FD, 0x00'FE'FD
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[12]]> color_addends_12 = {
			0x03'02'01, 0x03'FE'01, 0x03'01'02, 0x03'01'FE, 0x03'02'FF, 0x03'FE'FF, 0x03'FF'02, 0x03'FF'FE,
			0xFD'02'01, 0xFD'FE'01, 0xFD'01'02, 0xFD'01'FE, 0xFD'02'FF, 0xFD'FE'FF, 0xFD'FF'02, 0xFD'FF'FE,
			0x02'03'01, 0xFE'03'01, 0x01'03'02, 0x01'03'FE, 0x02'03'FF, 0xFE'03'FF, 0xFF'03'02, 0xFF'03'FE,
			0x02'FD'01, 0xFE'FD'01, 0x01'FD'02, 0x01'FD'FE, 0x02'FD'FF, 0xFE'FD'FF, 0xFF'FD'02, 0xFF'FD'FE,
			0x02'01'03, 0xFE'01'03, 0x01'02'03, 0x01'FE'03, 0x02'FF'03, 0xFE'FF'03, 0xFF'02'03, 0xFF'FE'03,
			0x02'01'FD, 0xFE'01'FD, 0x01'02'FD, 0x01'FE'FD, 0x02'FF'FD, 0xFE'FF'FD, 0xFF'02'FD, 0xFF'FE'FD
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[13]]> color_addends_13 = {
			0x04'00'00, 0xFC'00'00,
			0x00'04'00, 0x00'FC'00,
			0x00'00'04, 0x00'00'FC
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[14]]> color_addends_14 = {
			0x04'01'00, 0xFC'01'00, 0x04'00'01, 0xFC'00'01, 0x04'FF'00, 0xFC'FF'00, 0x04'00'FF, 0xFC'00'FF,
			0x01'04'00, 0x01'FC'00, 0x00'04'01, 0x00'FC'01, 0xFF'04'00, 0xFF'FC'00, 0x00'04'FF, 0x00'FC'FF,
			0x01'00'04, 0x01'00'FC, 0x00'01'04, 0x00'01'FC, 0xFF'00'04, 0xFF'00'FC, 0x00'FF'04, 0x00'FF'FC,
			0x03'02'02, 0x03'02'FE, 0x03'FE'02, 0x03'FE'FE, 0xFD'02'02, 0xFD'02'FE, 0xFD'FE'02, 0xFD'FE'FE,
			0x02'03'02, 0xFE'03'02, 0x02'03'FE, 0xFE'03'FE, 0x02'FD'02, 0xFE'FD'02, 0x02'FD'FE, 0xFE'FD'FE,
			0x02'02'03, 0xFE'02'03, 0x02'FE'03, 0xFE'FE'03, 0x02'02'FD, 0xFE'02'FD, 0x02'FE'FD, 0xFE'FE'FD
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[15]]> color_addends_15 = {
			0x03'03'00, 0x03'FD'00, 0xFD'03'00, 0xFD'FD'00,
			0x03'00'03, 0x03'00'FD, 0xFD'00'03, 0xFD'00'FD,
			0x00'03'03, 0x00'03'FD, 0x00'FD'03, 0x00'FD'FD,
			0x04'01'01, 0x04'FF'01, 0x04'01'FF, 0x04'FF'FF, 0x01'FC'01, 0xFF'FC'01, 0x01'FC'FF, 0xFF'FC'FF,
			0xFC'01'01, 0xFC'FF'01, 0xFC'01'FF, 0xFC'FF'FF, 0x01'01'04, 0xFF'01'04, 0x01'FF'04, 0xFF'FF'04,
			0x01'04'01, 0xFF'04'01, 0x01'04'FF, 0xFF'04'FF, 0x01'01'FC, 0xFF'01'FC, 0x01'FF'FC, 0xFF'FF'FC
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[16]]> color_addends_16 = {
			0x03'03'01, 0x03'FD'01, 0xFD'03'01, 0xFD'FD'01, 0x03'03'FF, 0x03'FD'FF, 0xFD'03'FF, 0xFD'FD'FF,
			0x03'01'03, 0x03'01'FD, 0xFD'01'03, 0xFD'01'FD, 0x03'FF'03, 0x03'FF'FD, 0xFD'FF'03, 0xFD'FF'FD,
			0x01'03'03, 0x01'03'FD, 0x01'FD'03, 0x01'FD'FD, 0xFF'03'03, 0xFF'03'FD, 0xFF'FD'03, 0xFF'FD'FD
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[17]]> color_addends_17 = {
			0x04'02'00, 0x04'FE'00, 0x04'00'02, 0x04'00'FE, 0x02'FC'00, 0xFE'FC'00, 0x00'FC'02, 0x00'FC'FE,
			0xFC'02'00, 0xFC'FE'00, 0xFC'00'02, 0xFC'00'FE, 0x02'00'04, 0xFE'00'04, 0x00'02'04, 0x00'FE'04,
			0x02'04'00, 0xFE'04'00, 0x00'04'02, 0x00'04'FE, 0x02'00'FC, 0xFE'00'FC, 0x00'02'FC, 0x00'FE'FC
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[18]]> color_addends_18 = {
			0x04'02'01, 0x04'FE'01, 0x04'01'02, 0x04'01'FE, 0x02'FC'01, 0xFE'FC'01, 0x01'FC'02, 0x01'FC'FE,
			0xFC'02'01, 0xFC'FE'01, 0xFC'01'02, 0xFC'01'FE, 0x02'01'04, 0xFE'01'04, 0x01'02'04, 0x01'FE'04,
			0x02'04'01, 0xFE'04'01, 0x01'04'02, 0x01'04'FE, 0x02'01'FC, 0xFE'01'FC, 0x01'02'FC, 0x01'FE'FC,
			0x04'02'FF, 0x04'FE'FF, 0x04'FF'02, 0x04'FF'FE, 0x02'FC'FF, 0xFE'FC'FF, 0xFF'FC'02, 0xFF'FC'FE,
			0xFC'02'FF, 0xFC'FE'FF, 0xFC'FF'02, 0xFC'FF'FE, 0x02'FF'04, 0xFE'FF'04, 0xFF'02'04, 0xFF'FE'04,
			0x02'04'FF, 0xFE'04'FF, 0xFF'04'02, 0xFF'04'FE, 0x02'FF'FC, 0xFE'FF'FC, 0xFF'02'FC, 0xFF'FE'FC
		};
		std::array<uint32_t, num_of_ways[sums_of_three_squares[19]]> color_addends_19 = {
			0x03'03'02, 0x03'FD'02, 0xFD'03'02, 0xFD'FD'02, 0x03'03'FE, 0x03'FD'FE, 0xFD'03'FE, 0xFD'FD'FE,
			0x03'02'03, 0x03'02'FD, 0xFD'02'03, 0xFD'02'FD, 0x03'FE'03, 0x03'FE'FD, 0xFD'FE'03, 0xFD'FE'FD,
			0x02'03'03, 0x02'03'FD, 0x02'FD'03, 0x02'FD'FD, 0xFE'03'03, 0xFE'03'FD, 0xFE'FD'03, 0xFE'FD'FD
		};
		const std::array<std::span<uint32_t>, sums_of_three_squares.size()> color_addends_lists = {
			color_addends_0, color_addends_1, color_addends_2, color_addends_3,
			color_addends_4, color_addends_5, color_addends_6, color_addends_7,
			color_addends_8, color_addends_9, color_addends_10, color_addends_11,
			color_addends_12, color_addends_13, color_addends_14, color_addends_15,
			color_addends_16, color_addends_17, color_addends_18, color_addends_19
		};



		// Member functions
		constexpr bool validate_data() const & {
			if (color_addends_lists.size() != sums_of_three_squares.size()) return false;

			return std::ranges::all_of(color_addends_lists | std::views::enumerate, [](const auto p) static -> bool {
				const auto & [index, color_addends] = p;
				if (color_addends.size() != num_of_ways[sums_of_three_squares[aa::unsign(index)]]
					|| color_addends.empty()) return false;

				const auto last = std::ranges::sort(color_addends);
				if (std::ranges::adjacent_find(color_addends) != last) return false;

				return std::ranges::all_of(color_addends, [&](const uint32_t addend) -> bool {
					return (
						aa::pow(aa::unsign(std::abs(aa::sign(aa::cast<uint32_t>(aa::cast<int8_t>(red(addend))))))) +
						aa::pow(aa::unsign(std::abs(aa::sign(aa::cast<uint32_t>(aa::cast<int8_t>(green(addend))))))) +
						aa::pow(aa::unsign(std::abs(aa::sign(aa::cast<uint32_t>(aa::cast<int8_t>(blue(addend)))))))
					) == sums_of_three_squares[aa::unsign(index)];
				});
			});
		}

		constexpr int work() & {
			// return 0;
			SDL_Event event = {.type = SDL_RegisterEvents(1)};

			const uint8_t * const is_text = std::bit_cast<uint8_t *>(is_text_srf->pixels);

			// std::println("pixel format: {:x}. pitch: {}", std::to_underlying(texture->format), pixels_srf->pitch);

			do {
				std::ranges::fill_n(pixels.get(), pixel_count, 0u);
				std::ranges::fill(color_used, false);

				{
					const uint32_t first_index = random(pixel_count);
					neighbors.emplace_back(first_index);
					pixels[first_index] = 0xFF'00'00'00u | random(0x01'00'00'00u);
				}
				do {
					aa::fixed_vector<uint32_t> & curr_neighbors =
						(neighbors.empty() || (!good_neighbors.empty() && SDL_randf() < 0.0001f))
						? good_neighbors : neighbors;

					const uint32_t & curr_index = curr_neighbors[random(curr_neighbors.size())];
					uint32_t & curr_color = pixels[curr_index];

					// Find nearest color
					std::ranges::any_of(std::views::iota(1u), [&](const uint32_t extent) -> bool {
						return std::ranges::any_of(color_addends_lists, [&](const std::span<uint32_t> color_addends) -> bool {
							return std::ranges::any_of(color_addends, [&](uint32_t & addend) -> bool {
								std::ranges::swap(addend, (&addend)[random(std::to_address(color_addends.end()) - &addend)]);

								const uint32_t r = red(curr_color) + (extent * aa::cast<uint32_t>(aa::cast<int8_t>(red(addend))));
								if (r > 0xFFu) return false;
								const uint32_t g = green(curr_color) + (extent * aa::cast<uint32_t>(aa::cast<int8_t>(green(addend))));
								if (g > 0xFFu) return false;
								const uint32_t b = blue(curr_color) + (extent * aa::cast<uint32_t>(aa::cast<int8_t>(blue(addend))));
								if (b > 0xFFu) return false;

								const uint32_t new_col = ((r << 16) | (g << 8) | (b << 0));
								if (color_used[new_col]) {
									return false;
								} else {
									curr_color = 0xFF'00'00'00u | new_col;
									color_used[new_col] = true;
									return true;
								}
							});
						});
					});

					// Find neighbors
					const auto find_neighbor = [&](const uint32_t new_index) -> void {
						if (pixels[new_index]) return;
						((is_text[curr_index] != is_text[new_index]) ? good_neighbors : neighbors).emplace_back(new_index);
						pixels[new_index] = curr_color;
					};
					const auto [pos_x, pos_y] = aa::pair{curr_index % width, curr_index / width};
					if /**/ (pos_x == (width - 1))				find_neighbor(curr_index - 1);
					else if (pos_x == 0)						find_neighbor(curr_index + 1);
					else [[likely]] { find_neighbor(curr_index - 1);		find_neighbor(curr_index + 1); }
					if /**/ (pos_y == (height - 1))				find_neighbor(curr_index - width);
					else if (pos_y == 0)						find_neighbor(curr_index + width);
					else [[likely]] { find_neighbor(curr_index - width);	find_neighbor(curr_index + width); }


					curr_neighbors.fast_pop(const_cast<uint32_t *>(&curr_index));
				} while (!neighbors.empty() || !good_neighbors.empty());

				// Stop working
				E(SDL_PushEvent(&event));
				SDL_WaitSemaphore(sem_block_thread);
			} while (is_working);

			return EXIT_SUCCESS;
		}

	public:
		constexpr SDL_AppResult init(const int argc, const char * const * const) & {
			// Negalime naudoti SDL numatytos funkcijos, nes ji labai neoptimali ir neišvengiamai spausdina \r\n.
			// Negalime rašyti į failo galą, nes tada reiktų failo valymo strategijos.
			E<error_kind::bad_log>(log_file = std::freopen("SDL_Log.log", "wb", stdout));
			std::setbuf(stdout, nullptr);

			SDL_SetLogOutputFunction([](void * const, const int c, const SDL_LogPriority p, const char * const m) static -> void {
				E(false, c, p, m);
			}, nullptr);

			E<error_kind::bad_argv>(argc == 1);
			// E<error_kind::info>(false);

			if (E<error_kind::bad_data>(validate_data())) return SDL_APP_FAILURE;


			if (E(SDL_SetHint(SDL_HINT_RENDER_DRIVER, "vulkan")))			return SDL_APP_FAILURE;
			if (E(SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "waitevent")))	return SDL_APP_FAILURE;

			if (E(SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, title.data())))	return SDL_APP_FAILURE;
			if (E(SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, "1.0")))		return SDL_APP_FAILURE;
			if (E(SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "Aleksandras Andrejevas")))		return SDL_APP_FAILURE;
			if (E(SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, "https://github.com/aandrejevas")))	return SDL_APP_FAILURE;

			if (E(SDL_InitSubSystem(SDL_INIT_VIDEO)))	return SDL_APP_FAILURE;
			if (E(TTF_Init()))							return SDL_APP_FAILURE;

			if (E(SDL_CreateWindowAndRenderer(title.data(), 0, 0, SDL_WINDOW_FULLSCREEN, &window.acquire(), &renderer.acquire())))
				return SDL_APP_FAILURE;


			if (E(SDL_GetWindowSizeInPixels(window, std::bit_cast<int *>(&width), std::bit_cast<int *>(&height))))
				return SDL_APP_FAILURE;
			pixel_count = width * height;

			if (E(texture = SDL_CreateTexture(renderer, SDL_PixelFormat::SDL_PIXELFORMAT_UNKNOWN,
				SDL_TextureAccess::SDL_TEXTUREACCESS_STREAMING, aa::sign(width), aa::sign(height)))
			) return SDL_APP_FAILURE;


			if (E(font = TTF_OpenFont("C:\\Windows\\Fonts\\Ruler Stencil Heavy.ttf", 980)))
				return SDL_APP_FAILURE;

			const aa::managed<SDL_Surface *, SDL_DestroySurface> text =
				TTF_RenderText_Solid(font, display_text.data(), display_text.size(), {255, 255, 255, SDL_ALPHA_OPAQUE});
			if (E(text.has_ownership())) return SDL_APP_FAILURE;

			const std::optional bbox = get_text_bbox(font, display_text);
			if (E(bbox.has_value())) return SDL_APP_FAILURE;


			SDL_Surface * const pixels_srf = aa::make_if([&](SDL_Surface *& s) -> bool {
				return SDL_LockTextureToSurface(texture, nullptr, &s);
			});
			if (E(pixels_srf)) return SDL_APP_FAILURE;

			if (E(SDL_BlitSurface(text, &*bbox, pixels_srf, &aa::stay(SDL_Rect{
				(texture->w - bbox->w) / 2,
				(texture->h - bbox->h) / 2, 0, 0})))) return SDL_APP_FAILURE;

			if (E((is_text_srf = SDL_ConvertSurface(pixels_srf, SDL_PixelFormat::SDL_PIXELFORMAT_RGB332)).has_ownership()))
				return SDL_APP_FAILURE;

			// Be šitų eilučių Segmentation fault.
			SDL_UnlockTexture(texture);
			if (E(SDL_LockTexture(texture, nullptr, std::bit_cast<void **>(&pixels.acquire()), &aa::stay<int>())))
				return SDL_APP_FAILURE;


			if (E((neighbors = {{pixel_count}}).has_ownership())) return SDL_APP_FAILURE;
			if (E((good_neighbors = {{pixel_count}}).has_ownership())) return SDL_APP_FAILURE;

			if (E(sem_block_thread = SDL_CreateSemaphore(0)))
				return SDL_APP_FAILURE;

			if (E(worker_thread = SDL_CreateThread([](void * const appstate) static -> int {
				return std::bit_cast<application *>(appstate)->work();
			}, "worker_thread", this)))
				return SDL_APP_FAILURE;


			if (E(SDL_CreateDirectory(output_dir.data()))) return SDL_APP_FAILURE;
			if (E((screenshot_name = {100 + output_dir.size(), output_dir}).has_ownership())) return SDL_APP_FAILURE;

			// Reikia šito kitaip teksto nenupiešia, nupiešia tik foną.
			// E(SDL_RenderPresent(renderer));

			return SDL_APP_CONTINUE;
		}

		constexpr SDL_AppResult event(const SDL_Event & event) & {
			switch (event.type) {
			case SDL_EventType::SDL_EVENT_QUIT:
				return SDL_APP_SUCCESS;

			case SDL_EventType::SDL_EVENT_KEY_DOWN:
				switch (event.key.key) {
				case SDLK_ESCAPE:
					return SDL_APP_SUCCESS;

				case SDLK_W:
					if (event.key.mod & SDL_KMOD_CTRL)
						return SDL_APP_SUCCESS;
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
						if (E(SDL_LockTexture(texture, nullptr, std::bit_cast<void **>(&pixels.acquire()), &aa::stay<int>())))
							return SDL_APP_FAILURE;

						SDL_SignalSemaphore(sem_block_thread);
					}
					break;
				}
				break;

			case SDL_EventType::SDL_EVENT_USER:
				should_draw = true;
				is_working = false;
				SDL_UnlockTexture(texture);
				break;
			}

			return SDL_APP_CONTINUE;
		}

		constexpr SDL_AppResult iterate() & {
			if (std::exchange(should_draw, false)) {
				// Draw
				E(SDL_RenderTexture(renderer, texture, nullptr, nullptr));


				switch (std::exchange(should_take_screenshot, false)) {
				case true:
					const SDL_DateTime d = get_current_date().value_or(aa::default_value);

					auto [out, _] = std::format_to_n(
						const_cast<char *>(screenshot_name.end()), aa::sign(screenshot_name.space() - screenshot_extension.size()),
						"img_{}-{:02}-{:02}_{:02}'{:02}'{:02}", d.year, d.month, d.day, d.hour, d.minute, d.second);
					std::ranges::copy(screenshot_extension, out);

					// https://gigi.nullneuron.net/gigilabs/saving-screenshots-in-sdl2/
					const aa::managed<SDL_Surface *, SDL_DestroySurface> screenshot = SDL_RenderReadPixels(renderer, nullptr);
					if (E(screenshot.has_ownership())) break;
					E(IMG_SaveJPG(screenshot, std::as_const(screenshot_name).data(), 100));
				}


				E(SDL_RenderPresent(renderer));
			}
			return SDL_APP_CONTINUE;
		}

		constexpr SDL_AppResult quit() & {
			if (worker_thread.has_ownership()) {
				is_working = false;
				SDL_SignalSemaphore(sem_block_thread);
				if (E<error_kind::bad_thread>(!aa::make([&](int & status) -> void {
					SDL_WaitThread(worker_thread, &status);
				})))
					return SDL_APP_FAILURE;
			}
			// E<error_kind::bad_data>(validate_data());
			return SDL_APP_SUCCESS;
		}
	};
}
