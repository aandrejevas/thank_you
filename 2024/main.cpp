// #undef NDEBUG
#define GLM_FORCE_MESSAGES
#define GLM_FORCE_EXPLICIT_CTOR
#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_SIZE_T_LENGTH
#define GLM_FORCE_UNRESTRICTED_GENTYPE
#include <glm/gtc/random.hpp>
#include <glm/vec2.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <SFML/Graphics.hpp>

#include "../AA/include/AA/metaprogramming/general.hpp"
#include "../AA/include/AA/container/fixed_vector.hpp"

#include <filesystem>
#include <chrono>
#include <thread>
#include <stop_token>
#include <semaphore>



struct neighbor {
	uint32_t index;
	sf::Color prev_col;
};

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

int main() {
	std::filesystem::create_directory("output");

	sf::RenderWindow window = sf::RenderWindow{sf::VideoMode::getDesktopMode(),
		"Thank_you_2024", sf::Style::Fullscreen, sf::ContextSettings{0, 0, 8}};
	window.setActive(false);
	window.setFramerateLimit(0);

	sf::Image smoke;
	std::counting_semaphore semaphore = std::counting_semaphore{aa::default_value};
	std::jthread drawing_thread = std::jthread{[&](const std::stop_token token) -> void {
		window.setActive(true);
		std::srand(aa::unsign<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count()));

		sf::Texture screenshot;
		screenshot.create(window.getSize().x, window.getSize().y);

		window.clear(sf::Color::Black);
		window.draw(aa::make_with_invocable([&](sf::Text &thanks) -> void {
			// https://learnsfml.com/how-to-center-text/
			const sf::FloatRect gb = thanks.getGlobalBounds(), lb = thanks.getLocalBounds();
			thanks.setOrigin(std::fma(gb.width, 0.5f, lb.left), std::fma(gb.height, 0.5f, lb.top));
			thanks.setPosition(0.5f * sf::Vector2f{window.getSize()});
			thanks.setFillColor(sf::Color::White);
		}, L"Ačiū", aa::make_with_invocable([](sf::Font &font) -> void {
			font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");
		}), 920u));
		window.display();

		screenshot.update(window);
		const sf::Image text_image = (smoke = screenshot.copyToImage());

		const size_t pixel_count = window.getSize().x * window.getSize().y;
		sf::Color *const smoke_data = reinterpret_cast<sf::Color *>(const_cast<std::uint8_t *>(smoke.getPixelsPtr()));

		aa::fixed_vector<neighbor> neighbors = {{pixel_count}};

		boost::geometry::index::rtree<sf::Color, boost::geometry::index::rstar<8>> tree;
		// Not constexpr because color_space could come from different sources.
		aa::fixed_array<sf::Color> color_space = {aa::representable_values_v<std::array<std::byte, 3>>};
		std::ranges::copy(std::views::transform(std::views::iota(0u, aa::cast<uint32_t>(color_space.size())),
			[](const uint32_t i) static -> sf::Color { return sf::Color{(i << 8) | 0xFFu}; }), color_space.data());
		sf::Color *const pixel_space_end = color_space.data() + pixel_count;

		do {
			semaphore.acquire();
			if (token.stop_requested()) {
				window.close();
				return;
			}

			// https://stackoverflow.com/questions/59336190/how-to-clear-sfml-image-very-fast-c
			std::ranges::fill_n(smoke_data, aa::sign(pixel_count), sf::Color::Transparent);

			const sf::Color first_color = color_space[glm::linearRand(0uz, color_space.last_index())];

			std::ranges::partial_sort(color_space, pixel_space_end, aa::default_v<std::ranges::less>,
				[&](const sf::Color c) -> auto { return boost::geometry::comparable_distance(c, first_color); });
			tree.insert(std::span{color_space.data(), pixel_count});
			assert(tree.size() == pixel_count);

			neighbors.emplace_back(aa::cast<uint32_t>(glm::linearRand(0uz, pixel_count - 1)), first_color);
			// size_t index = 0;
			do {
				// if (++index == 100'000) break;
				neighbor &curr = neighbors[glm::linearRand(0uz, neighbors.last_index())];
				sf::Color &new_col = smoke_data[curr.index];
				if (new_col == sf::Color::Transparent) {
					// tree.query(boost::geometry::index::nearest(curr.prev_col, 1), &new_col);
					// new_col = *tree.qbegin(boost::geometry::index::nearest(curr.prev_col, 1));
					// std::cout << aa::cast<size_t>(new_col.r) << ' ' << aa::cast<size_t>(new_col.g) << ' ' << aa::cast<size_t>(new_col.b) << ' ' << aa::cast<size_t>(new_col.a) << '\n';
					new_col = *tree.begin();
					tree.remove(new_col);

					const auto find_neighbor = [&](const uint32_t index) -> void {
						if (smoke_data[index] == sf::Color::Transparent) {
							neighbors.emplace_back(index, new_col);
						}
					};
					const glm::u32vec2 pos = glm::u32vec2{curr.index % window.getSize().x, curr.index / window.getSize().x};
					if (pos.x != (window.getSize().x - 1)) find_neighbor(curr.index + 1);
					if (pos.y != (window.getSize().y - 1)) find_neighbor(curr.index + window.getSize().x);
					if (pos.x != 0) find_neighbor(curr.index - 1);
					if (pos.y != 0) find_neighbor(curr.index - window.getSize().x);
				}
				neighbors.fast_erase(&curr);
			} while (!neighbors.empty());
			// assert(tree.empty());
			std::cout << "Done\n";

			screenshot.update(smoke);
			window.draw(sf::Sprite{screenshot});
			window.display();

			semaphore.try_acquire();
		} while (true);
	}};


	std::string filename; filename.reserve(50);
	sf::Event event;
	while (window.isOpen() && window.waitEvent(event)) {
		switch (event.type) {
		default: break;

		case sf::Event::Closed:
			goto EXIT;

		case sf::Event::KeyPressed:
			switch (event.key.code) {
			default: break;

			case sf::Keyboard::Escape:
				goto EXIT;

			case sf::Keyboard::S:
				if (event.key.control) {
					filename.clear();
					std::format_to(std::back_inserter(filename), "output/img_{}.jpg",
						std::chrono::system_clock::now().time_since_epoch().count());
					smoke.saveToFile(filename);
				}
				break;

			case sf::Keyboard::R:
				semaphore.try_acquire();
				semaphore.release();
				break;
			}
			break;
		}
	}

EXIT:
	drawing_thread.request_stop();
	semaphore.release(semaphore.max() - 1);
	drawing_thread.join();
	return EXIT_SUCCESS;
}
