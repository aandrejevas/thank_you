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
#include <atomic>



struct neighbor {
	const uint32_t index;
	const sf::Color prev_col;
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
		"Thank_you_2024", sf::Style::Fullscreen};
	window.setActive(false);
	window.setFramerateLimit(0);

	sf::Image smoke;
	std::atomic_unsigned_lock_free is_drawing = true;
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
			font.loadFromFile("C:\\Windows\\Fonts\\Ruler Stencil Heavy.ttf");
		}), 980u));
		window.display();

		screenshot.update(window);
		smoke = screenshot.copyToImage();

		aa::pmr::fixed_array<sf::Color> smoke_data = {window.getSize().x * window.getSize().y,
			reinterpret_cast<sf::Color *>(const_cast<std::uint8_t *>(smoke.getPixelsPtr()))};

		const aa::fixed_array text_data = aa::make_with_invocable([&](aa::fixed_array<sf::Color> &td) ->
			void { std::ranges::copy(smoke_data, td.data()); }, smoke_data.size());

		aa::fixed_vector<neighbor> neighbors = {{smoke_data.size() / 2}};

		// Not constexpr because color_space could come from different sources.
		const aa::fixed_array color_space = aa::make_with_invocable([](aa::fixed_array<sf::Color> &cs) -> void {
			std::ranges::copy(std::views::transform(std::views::iota(0u, aa::cast<uint32_t>(cs.size())),
				[](const uint32_t i) static -> sf::Color { return sf::Color{(i << 8) | 0xFFu}; }), cs.data());
		}, aa::representable_values_v<std::array<std::byte, 3>>);

		do {
			is_drawing.operator--();
			is_drawing.wait(false);
			if (token.stop_requested()) {
				window.close();
				return;
			}

			// https://stackoverflow.com/questions/59336190/how-to-clear-sfml-image-very-fast-c
			std::ranges::fill(smoke_data, sf::Color::Transparent);

			// We don't partial sort the color space to insert only the needed amount of colors into the tree because
			// in the corners some visual artifacts could appear because of not having access to closer colors.
			using rtree = boost::geometry::index::rtree<sf::Color, boost::geometry::index::rstar<16>>;
			rtree tree = rtree{color_space};

			do {
				const size_t index = glm::linearRand(0uz, smoke_data.last_index());
				if (text_data[index] == sf::Color::Black) {
					neighbors.emplace_back(
						aa::cast<uint32_t>(index),
						color_space[glm::linearRand(0uz, color_space.last_index())]);
					break;
				}
			} while (true);
			do {
				neighbor &curr = neighbors[glm::linearRand(0uz, neighbors.last_index())];
				sf::Color &new_col = smoke_data[curr.index];
				if (new_col == sf::Color::Transparent) {
					tree.query(boost::geometry::index::nearest(curr.prev_col, 1), &new_col);
					tree.remove(new_col);

					const auto find_neighbor = [&](const uint32_t index) -> void {
						if (smoke_data[index] == sf::Color::Transparent &&
							!(text_data[index] == sf::Color::Black && text_data[curr.index] != sf::Color::Black))
						{
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

			screenshot.update(smoke);
			window.draw(sf::Sprite{screenshot});
			window.display();
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
				if (event.key.control && !is_drawing) {
					filename.clear();
					std::format_to(std::back_inserter(filename), "output/img_{}.jpg",
						std::chrono::system_clock::now().time_since_epoch().count());
					smoke.saveToFile(filename);
				}
				break;

			case sf::Keyboard::R:
				is_drawing = true;
				is_drawing.notify_one();
				break;
			}
			break;
		}
	}

EXIT:
	drawing_thread.request_stop();
	is_drawing.operator++();
	is_drawing.notify_one();
	drawing_thread.join();
	return EXIT_SUCCESS;
}
