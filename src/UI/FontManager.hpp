#ifndef PFFG_FTGLFONTMANAGER_HDR
#define PFFG_FTGLFONTMANAGER_HDR

#include <map>
#include <string>

class FTFont;
class IFontManager {
public:
	static IFontManager* GetInstance();
	static void FreeInstance(IFontManager*);

	virtual FTFont* GetFont(const std::string&, int) = 0;

protected:
	IFontManager() {}
	virtual ~IFontManager() {}
};


class FTGLFontManager: public IFontManager {
public:
	static FTGLFontManager* GetInstance();
	static void FreeInstance(FTGLFontManager*);

	FTGLFontManager() {}
	FTFont* GetFont(const std::string&, int);

protected:
	~FTGLFontManager();

	typedef std::map<std::string, FTFont*> FontMap;
	typedef FontMap::const_iterator FontMapIt;

	FontMap fonts;
};

#endif
