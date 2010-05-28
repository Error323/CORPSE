#ifndef PFFG_SCOPEDTIMER_HDR
#define PFFG_SCOPEDTIMER_HDR

#include <map>
#include <string>

class ScopedTimer {
	public:
		ScopedTimer(const std::string&);
		~ScopedTimer();

		static unsigned int GetTaskTime(const std::string& t) {
			if (timings.find(t) != timings.end()) {
				return timings[t];
			}
			return 0;
		}
	private:
		const std::string task;
		unsigned int t1;
		unsigned int t2;
		unsigned int t3;

		static std::map<std::string, unsigned int> timings;
};

#endif
