#ifndef PFFG_EXTLOADER_HPP
#define PFFG_EXTLOADER_HPP

class CExtLoader {
	public:
		CExtLoader();
		~CExtLoader() {}

		bool LoadExtensions();

	private:
		bool haveOGL2;
};

#endif
