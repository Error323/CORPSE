#ifndef PFFG_FILE_HANDLER_HDR
#define PFFG_FILE_HANDLER_HDR

#include <set>
#include <vector>
#include <string>

#include "./VFSModes.h"


class CFileHandler {
	public:
		CFileHandler(const char* filename, const char* modes = SPRING_VFS_RAW_FIRST);
		CFileHandler(const std::string& filename, const std::string& modes = SPRING_VFS_RAW_FIRST);
		~CFileHandler(void);

		int Read(void* buf, int length);
		void Seek(int pos);

		bool FileExists() const;
		bool AtEOF() const;
		int Peek() const;
		int GetPos() const;
		int FileSize() const;

		bool LoadStringData(std::string& data);

	public:
		static bool InReadDir(const std::string& path);
		static bool InWriteDir(const std::string& path);

		static std::vector<std::string> FindFiles(const std::string& path, const std::string& pattern);

		static std::vector<std::string> DirList(const std::string& path,
			const std::string& pattern,
			const std::string& modes);

		static std::vector<std::string> SubDirs(const std::string& path,
			const std::string& pattern,
			const std::string& modes);

		static std::string AllowModes(const std::string& modes, const std::string& allowed);
		static std::string ForbidModes(const std::string& modes, const std::string& forbidden);

	private:
		void Init(const std::string& filename, const std::string& modes);

		bool TryRawFS(const std::string& filename);
		bool TryModFS(const std::string& filename);
		bool TryMapFS(const std::string& filename);
		bool TryBaseFS(const std::string& filename);

	private:
		static bool InsertRawFiles(std::set<std::string>& fileSet,
			const std::string& path,
			const std::string& pattern);
		static bool InsertModFiles(std::set<std::string>& fileSet,
			const std::string& path,
			const std::string& pattern);
		static bool InsertMapFiles(std::set<std::string>& fileSet,
			const std::string& path,
			const std::string& pattern);
		static bool InsertBaseFiles(std::set<std::string>& fileSet,
			const std::string& path,
			const std::string& pattern);

		static bool InsertRawDirs(std::set<std::string>& dirSet,
			const std::string& path,
			const std::string& pattern);
		static bool InsertModDirs(std::set<std::string>& dirSet,
			const std::string& path,
			const std::string& pattern);
		static bool InsertMapDirs(std::set<std::string>& dirSet,
			const std::string& path,
			const std::string& pattern);
		static bool InsertBaseDirs(std::set<std::string>& dirSet,
			const std::string& path,
			const std::string& pattern);

	private:
		std::ifstream* ifs;
		unsigned char* hpiFileBuffer;
		int hpiLength;
		int hpiOffset;
		int filesize;
};

#endif
