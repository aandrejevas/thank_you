#define SDL_MAIN_USE_CALLBACKS true
#include <SDL3/SDL_main.h>

#include "application.hpp"



constexpr application * get(void * const appstate)
{
	return std::bit_cast<application *>(appstate);
}

template<auto F, class... A>
constexpr SDL_AppResult process(void * const appstate, A&&... args)
{
	application & app = *get(appstate);
	switch ((app.*F)(std::forward<A>(args)...)) {
	case SDL_APP_CONTINUE: return SDL_APP_CONTINUE;
	case SDL_APP_SUCCESS: return (app.quit() == SDL_APP_FAILURE ? SDL_APP_FAILURE : SDL_APP_SUCCESS);
	default: return (app.quit(), SDL_APP_FAILURE);
	}
}


constexpr SDL_AppResult SDL_AppInit(void ** const appstate, const int argc, char ** const argv)
{
	alignas(application) constinit static std::array<std::byte, sizeof(application)> buffer;

	return process<&application::init>(std::ranges::construct_at(get(*appstate = buffer.data())), argc, argv);
}


constexpr SDL_AppResult SDL_AppEvent(void * const appstate, SDL_Event * const event)
{
	return process<&application::event>(appstate, *event);
}


constexpr SDL_AppResult SDL_AppIterate(void * const appstate)
{
	return process<&application::iterate>(appstate);
}


constexpr void SDL_AppQuit(void * const appstate, const SDL_AppResult)
{
	std::ranges::destroy_at(get(appstate));

	while (TTF_WasInit()) {
		TTF_Quit();
	}
}
