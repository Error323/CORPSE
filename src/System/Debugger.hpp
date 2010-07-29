#ifndef PFFG_DEBUGGER_HDR
#define PFFG_DEBUGGER_HDR

#include <map>
#include <string>

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

#define ASSERT(cond)                                                                                                                             \
	do {                                                                                                                                         \
		if (!(cond)) {                                                                                                                           \
			START();                                                                                                                             \
			FATAL("***ASSERTION FAILED***\n\n\tfile: %s\n\tline: %d\n\tfunc: %s\n\tcond: %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, #cond); \
			STOP();                                                                                                                              \
		}                                                                                                                                        \
	} while (0)

#define ASSERT_MSG(cond, ...) \
	do {                                                                                                                                                 \
		if (!(cond)) {                                                                                                                                   \
			START();                                                                                                                                     \
			FATAL("***ASSERTION FAILED***\n\n\tfile: %s\n\tline: %d\n\tfunc: %s\n\tcond: %s\n\ttext: ", __FILE__, __LINE__, __PRETTY_FUNCTION__, #cond); \
			FATAL(__VA_ARGS__);                                                                                                                          \
			FATAL("\n");                                                                                                                                 \
			STOP();                                                                                                                                      \
		} \
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

	const std::string& GetMessage() const { return mMessage; }
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

#endif // DEBUG_HDR
