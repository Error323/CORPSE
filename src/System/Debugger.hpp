#ifndef PFFG_DEBUGGER_HDR
#define PFFG_DEBUGGER_HDR

#ifdef PFFG_DEBUG
	#include <cstdio>
	#include <cstdlib>

	#include <map>
	#include <string>

	#if (!defined(WIN32) && !defined(__powerpc64__))
	#include <execinfo.h>
	#endif

	#include "../Input/InputReceiver.hpp"



	#ifdef WIN32
		#define BREAKPOINT __debugbreak()
	#elif defined(__powerpc64__)
		#define BREAKPOINT asm volatile ("tw 31,1,1")
	#else
		#define BREAKPOINT asm("int $3")
	#endif

	#define START() Debugger::GetInstance()->Begin(__FILE__, __LINE__)
	#define STOP() if (Debugger::GetInstance()->End()) BREAKPOINT
	#define BACKTRACE() Debugger::GetInstance()->DumpStack()

	#define PFFG_ASSERT(cond)                                                                                                                        \
		do {                                                                                                                                         \
			if (!(cond)) {                                                                                                                           \
				START();                                                                                                                             \
				FATAL("***ASSERTION FAILED***\n\n\tfile: %s\n\tline: %d\n\tfunc: %s\n\tcond: %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, #cond); \
				BACKTRACE();                                                                                                                         \
				STOP();                                                                                                                              \
			}                                                                                                                                        \
		} while (0)

	#define PFFG_ASSERT_MSG(cond, ...)                                                                                                                       \
		do {                                                                                                                                                 \
			if (!(cond)) {                                                                                                                                   \
				START();                                                                                                                                     \
				FATAL("***ASSERTION FAILED***\n\n\tfile: %s\n\tline: %d\n\tfunc: %s\n\tcond: %s\n\ttext: ", __FILE__, __LINE__, __PRETTY_FUNCTION__, #cond); \
				FATAL(__VA_ARGS__);                                                                                                                          \
				FATAL("\n");                                                                                                                                 \
				BACKTRACE();                                                                                                                                 \
				STOP();                                                                                                                                      \
			}                                                                                                                                                \
		} while (0)

	#define FATAL(...) \
		do {                                        \
			char buffer[2048];                      \
			snprintf(buffer, 2048, __VA_ARGS__);    \
			Debugger::GetInstance()->Print(buffer); \
		} while (0)



	class CInputHandler;
	class Debugger: public CInputReceiver {
	public:
		Debugger();
		~Debugger();

		void Begin(const char*, int);
		bool End();
		void Print(const char*);
		void DumpStack();

		const char* GetMessage() const { return mMessage.c_str(); }
		bool IsEnabled() const { return mEnabled; }

		void Pause() {}
		void KeyReleased(int);

		static Debugger* GetInstance();
		static void FreeInstance(Debugger*);

	private:
		char mKey[1024];

		std::map<char*, bool> mIgnoreForever;
		std::string mMessage;

		bool mEnabled;
		int mKeyReleased;

		CInputHandler* mInputHandler;
	};

#else
	#include <cassert>
	#define PFFG_ASSERT(c) assert(c)
	#define PFFG_ASSERT_MSG

	// dummy
	class Debugger {
	public:
		static Debugger* GetInstance();
		static void FreeInstance(Debugger*);

		const char* GetMessage() const { return ""; }
		bool IsEnabled() const { return false; }
	};

#endif // PFFG_DEBUG



#endif // PFFG_DEBUGGER_HDR
