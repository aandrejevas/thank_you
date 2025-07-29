#pragma once

#include "../AA/include/AA/metaprogramming/general.hpp"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>



constexpr std::optional<SDL_Rect> get_text_bbox(TTF_Font * const font, const std::string_view text) {
	return aa::apply<5>([&]<size_t... I> -> std::optional<SDL_Rect> {
		int text_w = 0, left = 0, top = aa::numeric_min, bottom = aa::numeric_max;
		const char * pstr = text.data();

		while (const uint32_t codepoint = SDL_StepUTF8(&pstr, nullptr)) {
			const std::optional o_metrics = ([&] -> std::optional<aa::quintet<int>> {
				if (aa::quintet<int> t; TTF_GetGlyphMetrics(font, codepoint, &t[aa::c<I>]...))
					return t; else return std::nullopt;
			})();
			if (!o_metrics) return std::nullopt;
			const auto & [min_x, max_x, min_y, max_y, advance] = *o_metrics;

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

		return SDL_Rect{left, TTF_GetFontAscent(font) - top, text_w, top - bottom};
	});
}
