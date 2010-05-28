#ifndef PFFG_LOGGER_HDR
#define PFFG_LOGGER_HDR

#include <string>
#include <fstream>

struct LuaParser;

enum LogLevel {
	LOG_BASIC,
	LOG_DEBUG,
};

class CLogger {
	public:
		CLogger(LuaParser*);

		~CLogger() {
			log.flush();
			log.close();
		}

		std::string GetLogName();

		CLogger& operator << (const char* s) {
			log << s;
			return *this;
		}
		template<typename T> CLogger& operator << (const T& t) {
			log << t;
			return *this;
		}

		template<typename T> CLogger& Log(const T& t, LogLevel lvl = LOG_BASIC) {
			switch (lvl) {
				case LOG_BASIC: {
					log << t; log << std::endl;
				} break;
				case LOG_DEBUG: {
					/* TODO */
				} break;
				default: {
				} break;
			}

			return *this;
		}

	private:
		std::string dir;
		std::string name;
		std::ofstream log;
};

#endif
