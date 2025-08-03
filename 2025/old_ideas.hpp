using strategy = uint32_t(*)(uint32_t);
std::array<strategy, 6> strategy_list_1 = { // Distance = √1 = 1
	&modify_color<1, 0, 0>, &modify_color<-1u, 0, 0>,
	&modify_color<0, 1, 0>, &modify_color<0, -1u, 0>,
	&modify_color<0, 0, 1>, &modify_color<0, 0, -1u>
};
std::array<strategy, 12> strategy_list_2 = { // Distance = √2
	&modify_color<+1u, 1, 0>, &modify_color<+1u, -1u, 0>, &modify_color<+1u, 0, 1>, &modify_color<+1u, 0, -1u>,
	&modify_color<-1u, 1, 0>, &modify_color<-1u, -1u, 0>, &modify_color<-1u, 0, 1>, &modify_color<-1u, 0, -1u>,
	&modify_color<0, +1u, 1>, &modify_color<0, +1u, -1u>, &modify_color<0, -1u, 1>, &modify_color<0, -1u, -1u>
};
std::array<strategy, 8> strategy_list_3 = { // Distance = √3
	&modify_color<+1u, 1, 1>, &modify_color<+1u, -1u, 1>, &modify_color<+1u, 1, -1u>, &modify_color<+1u, -1u, -1u>,
	&modify_color<-1u, 1, 1>, &modify_color<-1u, -1u, 1>, &modify_color<-1u, 1, -1u>, &modify_color<-1u, -1u, -1u>
};
std::array<strategy, 6> strategy_list_4 = { // Distance = √4 = 2
	&modify_color<2, 0, 0>, &modify_color<-2u, 0, 0>,
	&modify_color<0, 2, 0>, &modify_color<0, -2u, 0>,
	&modify_color<0, 0, 2>, &modify_color<0, 0, -2u>
};
std::array<strategy, 24> strategy_list_5 = { // Distance = √5
	&modify_color<+2u, 1, 0>, &modify_color<+2u, -1u, 0>, &modify_color<+2u, 0, 1>, &modify_color<+2u, 0, -1u>,
	&modify_color<-2u, 1, 0>, &modify_color<-2u, -1u, 0>, &modify_color<-2u, 0, 1>, &modify_color<-2u, 0, -1u>,
	&modify_color<1, +2u, 0>, &modify_color<-1u, +2u, 0>, &modify_color<0, +2u, 1>, &modify_color<0, +2u, -1u>,
	&modify_color<1, -2u, 0>, &modify_color<-1u, -2u, 0>, &modify_color<0, -2u, 1>, &modify_color<0, -2u, -1u>,
	&modify_color<1, 0, +2u>, &modify_color<-1u, 0, +2u>, &modify_color<0, 1, +2u>, &modify_color<0, -1u, +2u>,
	&modify_color<1, 0, -2u>, &modify_color<-1u, 0, -2u>, &modify_color<0, 1, -2u>, &modify_color<0, -1u, -2u>
};
std::array<strategy, 24> strategy_list_6 = { // Distance = √6
	&modify_color<+2u, 1, 1>, &modify_color<+2u, -1u, 1>, &modify_color<+2u, 1, -1u>, &modify_color<+2u, -1u, -1u>,
	&modify_color<-2u, 1, 1>, &modify_color<-2u, -1u, 1>, &modify_color<-2u, 1, -1u>, &modify_color<-2u, -1u, -1u>,
	&modify_color<1, +2u, 1>, &modify_color<-1u, +2u, 1>, &modify_color<1, +2u, -1u>, &modify_color<-1u, +2u, -1u>,
	&modify_color<1, -2u, 1>, &modify_color<-1u, -2u, 1>, &modify_color<1, -2u, -1u>, &modify_color<-1u, -2u, -1u>,
	&modify_color<1, 1, +2u>, &modify_color<-1u, 1, +2u>, &modify_color<1, -1u, +2u>, &modify_color<-1u, -1u, +2u>,
	&modify_color<1, 1, -2u>, &modify_color<-1u, 1, -2u>, &modify_color<1, -1u, -2u>, &modify_color<-1u, -1u, -2u>
};
const std::array<std::span<strategy>, 6> strategy_lists = {
	strategy_list_1,
	strategy_list_2,
	strategy_list_3,
	strategy_list_4,
	strategy_list_5,
	strategy_list_6
};

constexpr bool validate_strategies() const & {
	uint32_t d = 0;
	return std::ranges::all_of(strategy_lists, [&](const std::span<strategy> strategy_list) -> bool {
		if (strategy_list.empty()) return false;

		const auto last = std::ranges::sort(strategy_list);
		if (std::ranges::adjacent_find(strategy_list) != last) return false;

		d = d + 1;
		return std::ranges::all_of(strategy_list, [&](const strategy strat) -> bool {
			const uint32_t c = strat(0x7F'7F'7F);
			return (
				aa::pow(aa::unsign(std::abs(0x7F - aa::sign((c >> 16)	& 0xFFu)))) +
				aa::pow(aa::unsign(std::abs(0x7F - aa::sign((c >> 8)	& 0xFFu)))) +
				aa::pow(aa::unsign(std::abs(0x7F - aa::sign((c >> 0)	& 0xFFu))))
			) == d;
		});
	});
}

const auto find_neighbor = [&](const uint32_t new_index) -> void {
	if (pixels[new_index]) return;
	neighbors.emplace_back(new_index);

	if (!std::ranges::any_of(strategy_lists, [&](const std::span<strategy> strategy_list) -> bool {
		return std::ranges::any_of(strategy_list, [&](strategy & strat) -> bool {
			std::ranges::swap(strat, (&strat)[random(std::to_address(strategy_list.end()) - &strat)]);
			const uint32_t new_col = strat(curr_color);
			if (color_used[new_col]) {
				return false;
			} else {
				pixels[new_index] = 0xFF'00'00'00u | new_col;
				color_used[new_col] = true;
				return true;
			}
		});
	})) {
		pixels[new_index] = 0xFF'00'00'00u;
	}
};

template<uint32_t R, uint32_t G, uint32_t B>
constexpr uint32_t modify_color(const uint32_t c) {
	// c example: 0xFF'FF'00'00
	const uint32_t r = ((c >> 16)	& 0xFFu) + R;
	if (r > 0xFFu) return 0;
	const uint32_t g = ((c >> 8)	& 0xFFu) + G;
	if (g > 0xFFu) return 0;
	const uint32_t b = ((c >> 0)	& 0xFFu) + B;
	if (b > 0xFFu) return 0;

	return ((r << 16) | (g << 8) | (b << 0));
}
