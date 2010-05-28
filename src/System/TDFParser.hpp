#ifndef PFFG_TDFPARSER_HDR
#define PFFG_TDFPARSER_HDR

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>

class CTDFParser {
public:
	struct parse_error: public std::runtime_error {
	private:
		std::size_t line, column;
		std::string filename;
	public:
		parse_error(std::string const& line_of_error, std::size_t line, std::size_t column, std::string const& filename) throw();
		parse_error(std::size_t line, std::size_t column, std::string const& filename) throw();
		parse_error(std::string const& message, std::string const& line_of_error, std::size_t line, std::size_t column, std::string const& filename) throw();
		~parse_error() throw();
		std::size_t get_line() const;
		std::size_t get_column() const;
		std::string const& get_filename() const;
	};

	struct TDFSection {
		TDFSection* construct_subsection(const std::string& name);
		~TDFSection();
		
		std::map<std::string, TDFSection*> sections;
		std::map<std::string, std::string> values;
		void print(std::ostream& out) const;
		void add_name_value(const std::string& name, const std::string& value);

		template<typename T> void AddPair(const std::string& key, const T& value) {
			std::ostringstream buf;
			buf << value;
			add_name_value(key, buf.str());
		}
	};

	CTDFParser() {};
	CTDFParser(std::string const& filename);
	CTDFParser(const char* buffer, std::size_t size);

	void print(std::ostream& out) const;
	bool LoadFile(std::string const& file);
	void LoadBuffer(const char* buffer, std::size_t size);
	virtual ~CTDFParser() {}



	std::string SGetValueDef(const std::string& defaultvalue, const std::string& location) const;
	bool SGetValue(std::string& value, const std::string& location) const;

	template <typename T> T GetVal(const std::string& location) const {
		T t;
		std::string s;
		std::stringstream ss;

		SGetValue(s, location);
		ss << s;
		ss >> t;

		return t;
	};
	template <typename T> bool GetVal(T& val, const std::string& location) const {
		std::string buf;
		if (SGetValue(buf, location)) {
			std::istringstream stream(buf);
			stream >> val;
			return true;
		}
		return false;
	};
	
	template<typename T> int GetVector(std::vector<T>& vec, const std::string& location) const {
		std::string vecstring;
		std::stringstream stream;
		SGetValue(vecstring, location);
		stream << vecstring;

		int i = 0;
		T v = T(0);

		while (stream >> v) {
			vec.push_back(v);
			i++;
		}

		return i;
	}

	const std::map<std::string, std::string>& GetAllValues(const std::string& location) const;
	std::vector<std::string> GetSectionList(std::string const& location) const;
	bool SectionExist(std::string const& location) const;

	template<typename T> void ParseArray(const std::string& value, T* array, int length) const {
		std::stringstream stream; stream << value;

		for (int i = 0; i < length; i++) {
			stream >> array[i];
		}
	}



	template<typename T> void GetDef(T& value, const std::string& defvalue, const std::string& key) const {
		std::string str = SGetValueDef(defvalue, key);
		std::istringstream stream(str);
		stream >> value;
	}

	void GetDef(std::string& value, const std::string& defvalue, const std::string& key) const {
		value = SGetValueDef(defvalue, key);
	}

	template<typename T> void GetTDef(T& value, const T& defvalue, const std::string& key) const {
		std::string str;
		if (!SGetValue(str, key)) {
			value = defvalue;
			return;
		}

		std::stringstream stream;
		stream << str;
		stream >> value;
	}



	vec3f GetVec3(vec3f def, const std::string& location) const;
	vec4f GetVec4(vec4f def, const std::string& location) const;
	TDFSection* GetRootSection() { return &root_section; }

private:
	TDFSection root_section;
	std::string filename;

	std::vector<std::string> GetLocationVector(std::string const& location) const;
	void parse_buffer(char const* buf, std::size_t size);
};

#endif
