#pragma once

#include "../AA/include/AA/metaprogramming/general.hpp"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <print>
#include <source_location>



constexpr uint32_t red(const uint32_t c) {
	return ((c >> 16)	& 0xFFu);
}

constexpr uint32_t green(const uint32_t c) {
	return ((c >> 8)	& 0xFFu);
}

constexpr uint32_t blue(const uint32_t c) {
	return ((c >> 0)	& 0xFFu);
}

template<std::integral X>
constexpr X random(const X x) {
	return aa::sign_cast<X>(SDL_rand(aa::sign_cast<int32_t>(x)));
}

constexpr std::optional<SDL_Rect> get_text_bbox(TTF_Font * const font, const std::string_view text) {
	using metrics_t = aa::quintet<int>;
	return aa::apply<std::tuple_size_v<metrics_t>>([&]<size_t... I> -> std::optional<SDL_Rect> {
		int text_w = 0, left = 0, top = aa::numeric_min, bottom = aa::numeric_max;
		const char * pstr = text.data();

		while (const uint32_t codepoint = SDL_StepUTF8(&pstr, nullptr)) {
			const std::optional metrics = aa::make_opt([&](metrics_t & m) {
				return TTF_GetGlyphMetrics(font, codepoint, &m[aa::c<I>]...);
			});
			if (!metrics) return std::nullopt;
			const auto & [min_x, max_x, min_y, max_y, advance] = *metrics;

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

enum struct error_kind : size_t {
	SDL,
	log_file,
	bad_argv,
	bad_data,
	bad_color,
	info
};

template<error_kind ERROR = error_kind::SDL>
constexpr bool E(const bool cond,
	const int category = SDL_LogCategory::SDL_LOG_CATEGORY_APPLICATION,
	const SDL_LogPriority priority = SDL_LogPriority::SDL_LOG_PRIORITY_ERROR,
	const char * const msg = nullptr,
	const std::source_location l = std::source_location::current())
{
	if (cond) return false;
	else {
		/**/ if constexpr (ERROR == error_kind::bad_argv)		SDL_SetError("%s", "Wrong parameters provided to program");
		else if constexpr (ERROR == error_kind::log_file)		SDL_SetError("%s", "Could not open the log file");
		else if constexpr (ERROR == error_kind::bad_data)		SDL_SetError("%s", "Data is incorrect");
		else if constexpr (ERROR == error_kind::bad_color)		SDL_SetError("%s", "Failed to find a valid color");
		else if constexpr (ERROR == error_kind::info)			SDL_SetError("%s", "Nothing happened");

		std::println("{}:{}:{} '{}': {}. Category: {}. Priority: {}.",
			l.file_name(), l.line(), l.column(), l.function_name(), msg ? msg : SDL_GetError(),
			aa::unsign(category), aa::unsign(priority));
		return true;
	}
}
