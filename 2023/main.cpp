#define GLM_FORCE_CXX2A
#define GLM_FORCE_EXPLICIT_CTOR
#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_SIZE_T_LENGTH
#include <glm/gtc/noise.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include "../AA/include/AA/algorithm/int_math.hpp"
#include "../AA/include/AA/algorithm/init.hpp"
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <format>
#include <numbers>



template<std::floating_point T>
constexpr sf::Color lerp_color(const sf::Color c1, const sf::Color c2, const T amt) {
	return sf::Color{
		aa::round_lerp(c1.r, c2.r, amt), aa::round_lerp(c1.g, c2.g, amt),
		aa::round_lerp(c1.b, c2.b, amt), aa::round_lerp(c1.a, c2.a, amt)};
}

// https://en.wikipedia.org/wiki/HSL_and_HSV#Lightness
template<uint8_t M = 0>
constexpr sf::Color random_color() {
	const sf::Color color = sf::Color{glm::linearRand<uint32_t>(0x00'00'00'FFu, 0xFF'FF'FF'FFu)};
	if constexpr (M) {
		switch (glm::linearRand<uint32_t>(0, 2)) {
			case 0: return (color.r < M ? sf::Color{M, color.g, color.b} : color);
			case 1: return (color.g < M ? sf::Color{color.r, M, color.b} : color);
			case 2: return (color.b < M ? sf::Color{color.r, color.g, M} : color);
		}
		std::unreachable();
	}
	return color;
}

int main() {
	std::ios_base::sync_with_stdio(false);
	std::filesystem::create_directory("output");
	std::srand(aa::cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count()));

	sf::RenderWindow window = sf::RenderWindow{sf::VideoMode::getDesktopMode(),
		"Thank_you_2023", sf::Style::Fullscreen, sf::ContextSettings{0, 0, 8}};
	window.setFramerateLimit(60);

	{
		const sf::Font font = aa::make_with_invocable([](sf::Font &f) -> void {
			f.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");
		});
		sf::Text thanks = sf::Text{L"Ačiū", font, 920}; {
			// https://learnsfml.com/how-to-center-text/
			const sf::FloatRect &gb = thanks.getGlobalBounds(), &lb = thanks.getLocalBounds();
			thanks.setOrigin(gb.width * 0.5f + lb.left, gb.height * 0.5f + lb.top);
			thanks.setPosition(0.5f * sf::Vector2f{window.getSize()});
			thanks.setFillColor(sf::Color::White);
		}

		window.clear(sf::Color::Black);
		window.draw(thanks);
	}
	sf::Texture screenshot;
	screenshot.create(window.getSize().x, window.getSize().y);
	const sf::Image image = (screenshot.update(window), screenshot).copyToImage();

	std::string str;
	str.reserve(50);
	sf::Event event;

	sf::VertexArray line = sf::VertexArray{sf::TriangleStrip};
	const auto draw = [&]() -> void {
		const sf::Color background = random_color();
		window.clear(background);

		const size_t lifetime = glm::linearRand<size_t>(100, 400);
		const float radius = glm::linearRand<float>(1, 50);
		const float freq = glm::linearRand<float>(0.005f, 0.01f);
		const float phase = aa::cast<float>(100 * glm::linearRand<size_t>(0, 1000));
		const sf::Color c1 = sf::Color{background.toInteger() & 0xFF'FF'FF'01u}, c2 = random_color();
		aa::repeat(glm::linearRand<size_t>(1000, 2000), [&]() -> void {
			line.clear();

			glm::vec2 pos, prev;
			do {
				pos = glm::linearRand(glm::vec2{10.f}, glm::vec2{window.getSize().x, window.getSize().y} - 11.f);
			} while (image.getPixel(aa::cast<uint32_t>(pos.x), aa::cast<uint32_t>(pos.y)) == sf::Color::White);
			size_t life = 0; do {
				const float t = aa::cast<float>(life) / aa::cast<float>(lifetime);
				const float theta = glm::simplex((pos * freq) + phase) * std::numbers::pi_v<float>;
				const glm::vec2 heading = glm::rotate(glm::vec2{1.f, 0.f}, theta),
					point = heading * std::lerp(radius, 1.f, t);
				const sf::Color color = lerp_color(c1, c2, t);
				if (!life) {
					// https://gamedev.stackexchange.com/questions/70075/how-can-i-find-the-perpendicular-to-a-2d-vector
					line.append(sf::Vertex{std::bit_cast<sf::Vector2f>(pos + glm::vec2{-point.y, point.x}), color});
					line.append(sf::Vertex{std::bit_cast<sf::Vector2f>(pos + glm::vec2{point.y, -point.x}), color});
				} else {
					const sf::Color trans = sf::Color{color.toInteger() & 0xFF'FF'FF'00u};
					if (glm::orientedAngle(prev, heading) >= 0) {
						line.append(sf::Vertex{line[line.getVertexCount() - 2].position, color});
						line.append(sf::Vertex{std::bit_cast<sf::Vector2f>(pos + glm::vec2{point.y, -point.x}), color});
					} else {
						line.append(sf::Vertex{std::bit_cast<sf::Vector2f>(pos + glm::vec2{-point.y, point.x}), color});
						line.append(sf::Vertex{line[line.getVertexCount() - 2].position, color});
					}
				}

				pos += heading;
				prev = heading;
				if (pos.x < 0.f || aa::cast<float>(window.getSize().x) <= pos.x ||
					pos.y < 0.f || aa::cast<float>(window.getSize().y) <= pos.y) break;
				if (image.getPixel(aa::cast<uint32_t>(pos.x), aa::cast<uint32_t>(pos.y)) == sf::Color::White) {
					if (life != lifetime) {
						life += std::ranges::min(lifetime - life, 3uz);
					} else break;
				} else ++life;
			} while (life <= lifetime);

			window.draw(line);
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
								std::format_to(std::back_inserter((str.clear(), str)), "output/img_{}.png",
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
