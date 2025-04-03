#define GLM_FORCE_MESSAGES
#define GLM_FORCE_EXPLICIT_CTOR
#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_SIZE_T_LENGTH
#define GLM_FORCE_UNRESTRICTED_GENTYPE
#include <glm/gtc/random.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <SFML/Graphics.hpp>

#include "../AA/include/AA/metaprogramming/general.hpp"
#include "../AA/include/AA/container/fixed_vector.hpp"

#include <cstdlib>
#include <filesystem>
#include <chrono>
#include <thread>
#include <semaphore>



template<>
struct boost::geometry::traits::tag<sf::Color> : std::type_identity<boost::geometry::point_tag> {};
template<>
struct boost::geometry::traits::dimension<sf::Color> : aa::constant<3uz> {};
template<>
struct boost::geometry::traits::coordinate_type<sf::Color> : std::type_identity<uint8_t> {};
template<>
struct boost::geometry::traits::coordinate_system<sf::Color> : std::type_identity<boost::geometry::cs::cartesian> {};
template<size_t I>
struct boost::geometry::traits::access<sf::Color, I> {
	static constexpr uint8_t get(const sf::Color c) {
		/**/ if constexpr (I == 0)	return c.r;
		else if constexpr (I == 1)	return c.g;
		else						return c.b;
	}
};

constexpr void boost::throw_exception(const std::exception &) {
	std::abort();
}

// https://www.youtube.com/watch?v=dVQDYne8Bkc
int main() {
	std::filesystem::create_directory("output");

	sf::RenderWindow window = sf::RenderWindow{sf::VideoMode::getDesktopMode(),
		"Thank_you_2024", sf::Style::None, sf::State::Fullscreen};


	std::counting_semaphore
		sem_block_thread = std::counting_semaphore{0},
		sem_signal_draw = std::counting_semaphore{0};
	bool is_thread_working = true, should_draw = false;

	sf::Texture screenshot = sf::Texture{window.getSize()};
	const sf::Sprite sprite = sf::Sprite{screenshot};
	if (window.isOpen()) {
		window.clear(sf::Color::Black);
		// Even if we disable font smoothing, it still gets smoothed so we just don't bother.
		const sf::Font font = sf::Font{"C:\\Windows\\Fonts\\Ruler Stencil Heavy.ttf"};
		window.draw(aa::make_with_invocable([&](sf::Text &thanks) -> void {
			// https://learnsfml.com/how-to-center-text/
			const sf::FloatRect gb = thanks.getGlobalBounds(), lb = thanks.getLocalBounds();
			thanks.setOrigin({std::fma(gb.size.x, 0.5f, lb.position.x), std::fma(gb.size.y, 0.5f, lb.position.y)});
			thanks.setPosition(0.5f * sf::Vector2f{window.getSize()});
			thanks.setFillColor(sf::Color::White);
		},
			font, L"Ačiū", 980u));
		window.display();
		screenshot.update(window);
	}
	sf::Image smoke = screenshot.copyToImage();

	std::thread worker_thread = std::thread{[&] -> void {
		std::srand(aa::unsign<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count()));

		const sf::Vector2u window_size = window.getSize();

		aa::pmr::fixed_array<sf::Color> smoke_data = {window_size.x * window_size.y,
			reinterpret_cast<sf::Color *>(const_cast<std::uint8_t *>(smoke.getPixelsPtr()))};

		const aa::fixed_array text_data = aa::make_with_invocable([&](aa::fixed_array<sf::Color> &td) ->
			void { std::ranges::copy(smoke_data, td.data()); }, smoke_data.size());

		aa::fixed_vector<const uint32_t> neighbors = {{smoke_data.size()}};

		using rtree = boost::geometry::index::rtree<sf::Color, boost::geometry::index::rstar<16>>;
		const rtree packed_tree = ([] static {
			const auto view = std::views::transform(
				std::views::iota(0u, aa::value_v<uint32_t, aa::representable_values_v<std::array<std::byte, 3>>>),
				[](const uint32_t i) static { return sf::Color{(i << 8) | 0xFFu}; });
			return rtree{view.begin(), view.end()};
		})();

		do {
			// https://stackoverflow.com/questions/59336190/how-to-clear-sfml-image-very-fast-c
			std::ranges::fill(smoke_data, sf::Color::Transparent);

			// We don't partial sort the color space to insert only the needed amount of colors into the tree because
			// in the corners some visual artifacts could appear because of not having access to closer colors.
			rtree tree = packed_tree;

			do {
				const size_t index = glm::linearRand(0uz, smoke_data.last_index());
				if (text_data[index] == sf::Color::Black) {
					neighbors.emplace_back(aa::cast<uint32_t>(index));
					smoke_data[index] = sf::Color{(glm::linearRand(0u, 0x00'FF'FF'FFu) << 8) | 0xFFu};
					break;
				}
			} while (true);
		DRAW:
			do {
				const uint32_t &curr_index = neighbors[glm::linearRand(0uz, neighbors.last_index())];
				if (text_data[curr_index] != sf::Color::Black && glm::linearRand(0.f, 1.f) < 0.9f) goto DRAW;

				sf::Color &new_col = smoke_data[curr_index];

				tree.query(boost::geometry::index::nearest(new_col, 1), &new_col);
				tree.remove(new_col);

				const auto find_neighbor = [&](const uint32_t index) -> void {
					if (smoke_data[index] != sf::Color::Transparent) return;
					neighbors.emplace_back(index);
					smoke_data[index] = new_col;
				};
				const sf::Vector2u pos = {curr_index % window_size.x, curr_index / window_size.x};
				if (pos.x != (window_size.x - 1))	find_neighbor(curr_index + 1);
				if (pos.y != (window_size.y - 1))	find_neighbor(curr_index + window_size.x);
				if (pos.x != 0)						find_neighbor(curr_index - 1);
				if (pos.y != 0)						find_neighbor(curr_index - window_size.x);

				neighbors.fast_erase(&curr_index);
			} while (!neighbors.empty());

			// We have to have this sem bc otherwise we could start changing smoke while drawing.
			should_draw = true;
			sem_signal_draw.acquire();

			// Stop working
			is_thread_working = false;
			sem_block_thread.acquire();
		} while (true);
	}};


	std::string filename; filename.reserve(50);
	std::optional<sf::Event> event;
	const sf::Time timeout = sf::milliseconds(10);

	while (window.isOpen()) {
		while ((event = window.pollEvent())) {
			std::as_const(event)->visit([&]<class T>(const T &data) -> void {
				/*	*/ if constexpr (std::same_as<T, sf::Event::Closed>) {
					goto STOP;

				} else if constexpr (std::same_as<T, sf::Event::KeyPressed>) {
					switch (data.code) {
					default: break;

					case sf::Keyboard::Key::Escape:
						goto STOP;

					case sf::Keyboard::Key::S:
						if (data.control && !is_thread_working) {
							filename.clear();
							std::format_to(std::back_inserter(filename), "output/img_{}.jpg",
								std::chrono::system_clock::now().time_since_epoch().count());
							if (!smoke.saveToFile(filename)) {
								goto STOP;
							}
						}
						break;

					case sf::Keyboard::Key::R:
						if (!is_thread_working) {
							is_thread_working = true;
							sem_block_thread.release();
						}
						break;
					}
				}
				if (false) {
					[[maybe_unused]] STOP:
					window.close();
					// There is no way to kill the thread from main so we do this.
					std::quick_exit(EXIT_SUCCESS);
				}
			});
		}
		sf::sleep(timeout);
		if (should_draw) {
			screenshot.update(smoke);
			window.draw(sprite);
			window.display();

			// If release happens before acquire then acquire will just not block, otherwise release will notify.
			should_draw = false;
			sem_signal_draw.release();
		}
	}

	return EXIT_SUCCESS;
}
