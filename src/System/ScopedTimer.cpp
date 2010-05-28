#include <SDL/SDL_timer.h>

#include "./ScopedTimer.hpp"

std::map<std::string, unsigned int> ScopedTimer::timings;

ScopedTimer::ScopedTimer(const std::string& s): task(s), t1(SDL_GetTicks()) {
}

ScopedTimer::~ScopedTimer() {
	t2 = SDL_GetTicks();
	t3 = t2 - t1;

	// <task> took <t3> msecs
	if (timings.find(task) != timings.end()) {
		timings[task] += t3;
	} else {
		timings[task] = t3;
	}
}
