#define GLM_FORCE_CXX2A
#define GLM_FORCE_EXPLICIT_CTOR
#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_SIZE_T_LENGTH
#include <glm/gtc/noise.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "../AA/include/AA/algorithm/arithmetic.hpp"
#include "../AA/include/AA/algorithm/int_math.hpp"
#include "../AA/include/AA/algorithm/init.hpp"

#include <SFML/Graphics.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <ios>
#include <iterator>
#include <algorithm>
#include <chrono>
#include <string>

using namespace std::placeholders;



template<std::floating_point T>
constexpr sf::Color lerp_color(const sf::Color c1, const sf::Color c2, const T amt) {
	return sf::Color{
		aa::round_lerp(c1.r, c2.r, amt),
		aa::round_lerp(c1.g, c2.g, amt),
		aa::round_lerp(c1.b, c2.b, amt)};
}

// https://en.wikipedia.org/wiki/HSL_and_HSV#Lightness
template<uint8_t M = 0, uint8_t A = 255>
constexpr sf::Color random_color() {
	const sf::Color color = sf::Color{(glm::linearRand<uint32_t>(0x00'00'00'00u, 0x00'FF'FF'FFu) << 8) | A};
	if constexpr (M) {
		switch (glm::linearRand<uint32_t>(0, 2)) {
			case 0: return (color.r < M ? sf::Color{M, color.g, color.b, A} : color);
			case 1: return (color.g < M ? sf::Color{color.r, M, color.b, A} : color);
			case 2: return (color.b < M ? sf::Color{color.r, color.g, M, A} : color);
		}
		std::unreachable();
	}
	return color;
}

template<uint8_t M = 255>
constexpr sf::Color random_bounded_color() {
	return sf::Color{
		aa::cast<uint8_t>(glm::linearRand<uint32_t>(0, M)),
		aa::cast<uint8_t>(glm::linearRand<uint32_t>(0, M)),
		aa::cast<uint8_t>(glm::linearRand<uint32_t>(0, M))};
}

int main() {
	std::ios_base::sync_with_stdio(false);
	std::filesystem::create_directory("output");
	std::srand(aa::cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count()));

	sf::RenderWindow window = sf::RenderWindow{sf::VideoMode::getDesktopMode(),
		"Thank_you_2023", sf::Style::Fullscreen, sf::ContextSettings{0, 0, 8}};
	window.setFramerateLimit(60);
	const sf::Vector2f window_size = sf::Vector2f{window.getSize()};

	sf::Texture screenshot;
	screenshot.create(window.getSize().x, window.getSize().y);
	{
		const sf::Font font = aa::make_with_invocable([](sf::Font &f) -> void {
			f.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");
		});
		sf::Text thanks = sf::Text{L"Ačiū", font, 920}; {
			// https://learnsfml.com/how-to-center-text/
			const sf::FloatRect &gb = thanks.getGlobalBounds(), &lb = thanks.getLocalBounds();
			thanks.setOrigin(gb.width * 0.5f + lb.left, gb.height * 0.5f + lb.top);
			thanks.setPosition(0.5f * window_size);
			thanks.setFillColor(sf::Color::White);
		}
		window.clear(sf::Color::Black);
		window.draw(thanks);
	}
	const sf::Image image = (screenshot.update(window), screenshot).copyToImage();
	const sf::RenderStates states = sf::RenderStates{sf::BlendMax};

	std::string str;
	str.reserve(50);
	sf::Event event;

	sf::VertexArray line = sf::VertexArray{sf::TriangleStrip};
	const auto draw = [&]() -> void {
		{
			const std::array<sf::Vertex, 4> mask = {
				sf::Vertex{sf::Vector2f{0, 0}, random_color<255>()},
				sf::Vertex{sf::Vector2f{window_size.x, 0}, random_color<255>()},
				sf::Vertex{sf::Vector2f{0, window_size.y}, random_color<255>()},
				sf::Vertex{window_size, random_color<255>()},
			};
			window.draw(mask.data(), mask.size(), sf::PrimitiveType::TriangleStrip);
		}
		const sf::Image grad = (screenshot.update(window), screenshot).copyToImage();
		const sf::Color background = random_bounded_color<127>();
		window.clear(background);

		const size_t lifetime = glm::linearRand<size_t>(100, 200);
		const size_t decay = std::ranges::max(aa::cast<size_t>(aa::cast<float>(lifetime) * 0.025f), 1uz);
		const float radius = glm::linearRand<float>(5, 30);
		const float freq = glm::linearRand<float>(0.0002f, 0.001f);
		const glm::vec2 phase = glm::linearRand(glm::vec2{-5000.f}, glm::vec2{5000.f});
		aa::repeat(5000, [&]() -> void {
			line.clear();

			glm::vec2 pos;
			do {
				pos = glm::linearRand(glm::vec2{10.f}, std::bit_cast<glm::vec2>(window_size) - 11.f);
			} while (image.getPixel(aa::cast<uint32_t>(pos.x), aa::cast<uint32_t>(pos.y)) == sf::Color::White);
			const sf::Color c2 = grad.getPixel(aa::cast<uint32_t>(pos.x), aa::cast<uint32_t>(pos.y));
			size_t life = 0; do {
				const float t = aa::cast<float>(life) / aa::cast<float>(lifetime);

				// https://www.bit-101.com/blog/2021/07/mapping-perlin-noise-to-angles/
				const glm::vec2 heading = glm::rotate(glm::vec2{1.f, 0.f},
					aa::norm_map<_1>(glm::simplex((pos * freq) + phase), -0.5f, 1.f, glm::two_pi<float>()));

				const glm::vec2 point = heading * std::lerp(radius, 0.5f, t);

				const sf::Color color = sf::Color{lerp_color(background, c2, t).toInteger()
					& aa::round_lerp(0xFF'FF'FF'00u, 0xFF'FF'FF'FFu, std::cbrt(t))};

				// https://gamedev.stackexchange.com/questions/70075/how-can-i-find-the-perpendicular-to-a-2d-vector
				line.append(sf::Vertex{std::bit_cast<sf::Vector2f>(pos + glm::vec2{-point.y, point.x}), color});
				line.append(sf::Vertex{std::bit_cast<sf::Vector2f>(pos + glm::vec2{point.y, -point.x}), color});

				pos += heading;
				if (pos.x < 0.f || window_size.x <= pos.x ||
					pos.y < 0.f || window_size.y <= pos.y || life == lifetime) break;
				if (image.getPixel(aa::cast<uint32_t>(pos.x), aa::cast<uint32_t>(pos.y)) == sf::Color::White) {
					life += std::ranges::min(lifetime - life, decay);
				} else ++life;
			} while (true);

			window.draw(line, states);
		});

		window.display();
	};
	draw();

	while (window.isOpen()) {
		while (window.pollEvent(event)) {
			switch (event.type) {
				case sf::Event::Closed:
					window.close();
					break;

				case sf::Event::KeyPressed:
					switch (event.key.code) {
						case sf::Keyboard::Escape:
							window.close();
							break;

						case sf::Keyboard::S:
							if (event.key.control) {
								std::format_to(std::back_inserter((str.clear(), str)), "output/img_{}.jpg",
									std::chrono::system_clock::now().time_since_epoch().count());
								(screenshot.update(window), screenshot).copyToImage().saveToFile(str);
							}
							break;

						case sf::Keyboard::R:
							draw();
							break;

						default: break;
					}
					break;

				default: break;
			}
		}
	}

	return EXIT_SUCCESS;
}
