#ifndef DEBUG_HDR
#define DEBUG_HDR

#include "../Input/InputReceiver.hpp"
#include "../Input/InputHandler.hpp"

#include <map>

#ifdef WIN32
	#define BREAKPOINT __debugbreak()
#elif defined(__powerpc64__)
	#define BREAKPOINT asm volatile ("tw 31,1,1")
#else
	#define BREAKPOINT asm("int $3")
#endif

#define START() theDebugger->Begin(__FILE__, __LINE__)
#define STOP() if (theDebugger->End()) BREAKPOINT

#define ASSERT(cond) \
	do { \
		if (!(cond)) { \
			START(); \
			FATAL("***Assertion Failed***\n\n\tfile = %s\n\tline = %d\n\tcond = %s\n", __FILE__, __LINE__, #cond); \
			STOP(); \
		} \
	} while (0)
			

#define ASSERT_MSG(cond, ...) \
	do { \
		if (!(cond)) { \
			START(); \
			FATAL("***Assertion Failed***\n\n\tfile: %s\n\tline: %d\n\tcond: %s\n\ttext: ", __FILE__, __LINE__, #cond); \
			FATAL(__VA_ARGS__); \
			FATAL("\n"); \
			STOP(); \
		} \
	} while (0)

#define FATAL(...) \
	do { \
		char buffer[2048]; \
		sprintf(buffer, __VA_ARGS__); \
		theDebugger->Print(buffer); \
	} while (0)
	

class Debug: public CInputReceiver {
public:
	Debug() {}
	~Debug();
	
	void Init();
	void Begin(const char*, int);
	bool End();
	void Print(const char*);
	void Pause();
	void KeyReleased(int);

	static Debug* GetInstance();

private:
	char mKey[1024];
	std::map<char*, bool> mIgnoreForever;
	int mKeyReleased;
	CInputHandler *mInputHandler;
	static Debug *mInstance;
};

extern Debug *theDebugger;

#endif // DEBUG_HDR
