#define SDL_MAIN_USE_CALLBACKS true
#include <SDL3/SDL_main.h>

#include "application.hpp"



constexpr SDL_AppResult SDL_AppInit(void ** const appstate, const int argc, char ** const argv)
{
	alignas(application) constinit static std::array<std::byte, sizeof(application)> buffer;

	return std::ranges::construct_at(std::bit_cast<application *>(*appstate = buffer.data()))->init(argc, argv);
}


constexpr SDL_AppResult SDL_AppEvent(void * const appstate, SDL_Event * const event)
{
	return std::bit_cast<application *>(appstate)->event(*event);
}


constexpr SDL_AppResult SDL_AppIterate(void * const appstate)
{
	return std::bit_cast<application *>(appstate)->iterate();
}


constexpr void SDL_AppQuit(void * const appstate, const SDL_AppResult result)
{
	std::ranges::destroy_at(std::bit_cast<application *>(appstate)->quit(result));

	while (TTF_WasInit()) {
		TTF_Quit();
	}
}
