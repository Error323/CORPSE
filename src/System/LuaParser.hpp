#ifndef PFFG_LUA_PARSER_HDR
#define PFFG_LUA_PARSER_HDR

#include <list>
#include <map>
#include <string>

#include "../Math/vec3fwd.hpp"
#include "../Math/vec4fwd.hpp"

struct lua_State;

struct LuaTable {
public:
	bool operator < (const LuaTable& t) const { return (this < &t); }
	bool operator == (const LuaTable&) const;

	~LuaTable();

	void Print(int) const;
	void Parse(lua_State*, int);

	typedef std::pair<LuaTable*, LuaTable*>     TblTblPair;
	typedef std::pair<LuaTable*, std::string>   TblStrPair;
	typedef std::pair<LuaTable*, float>         TblFltPair;
	typedef std::pair<std::string, LuaTable*>   StrTblPair;
	typedef std::pair<std::string, std::string> StrStrPair;
	typedef std::pair<std::string, float>       StrFltPair;
	typedef std::pair<int, LuaTable*>           IntTblPair;
	typedef std::pair<int, std::string>         IntStrPair;
	typedef std::pair<int, float>               IntFltPair;

	void GetTblTblKeys(std::list<LuaTable*>*  ) const;
	void GetTblStrKeys(std::list<LuaTable*>*  ) const;
	void GetTblFltKeys(std::list<LuaTable*>*  ) const;
	void GetStrTblKeys(std::list<std::string>*) const;
	void GetStrStrKeys(std::list<std::string>*) const;
	void GetStrFltKeys(std::list<std::string>*) const;
	void GetIntTblKeys(std::list<int>*        ) const;
	void GetIntStrKeys(std::list<int>*        ) const;
	void GetIntFltKeys(std::list<int>*        ) const;

	const LuaTable* GetTblVal(LuaTable*, LuaTable* defVal = 0) const;
	const LuaTable* GetTblVal(const std::string&, LuaTable* defVal = 0) const;
	const LuaTable* GetTblVal(int, LuaTable* defVal = 0) const;
	const std::string& GetStrVal(LuaTable*, const std::string& defVal) const;
	const std::string& GetStrVal(const std::string&, const std::string& defVal) const;
	const std::string& GetStrVal(int, const std::string& defVal) const;
	int GetFltVal(LuaTable*, float defVal) const;
	int GetFltVal(const std::string&, float defVal) const;
	int GetFltVal(int, float defVal) const;

	bool HasStrTblKey(const std::string& key) const { return (StrTblPairs.find(key) != StrTblPairs.end()); }
	bool HasStrStrKey(const std::string& key) const { return (StrStrPairs.find(key) != StrStrPairs.end()); }
	bool HasStrFltKey(const std::string& key) const { return (StrFltPairs.find(key) != StrFltPairs.end()); }



	template<typename T> void GetArray(const LuaTable* tbl, T* array, int len) const {
		for (int i = 0; i < len; i++) {
			array[i] = T(tbl->GetFltVal(i + 1, T(0)));
		}
	}

	template<typename V> V GetVec(const std::string& key, int len) const {
		const std::map<std::string, LuaTable*>::const_iterator it = StrTblPairs.find(key);

		V v;

		if (it != StrTblPairs.end()) {
			GetArray(it->second, &v.x, len);
		}

		return v;
	}

private:
	std::map<LuaTable*, LuaTable*>     TblTblPairs;
	std::map<LuaTable*, std::string>   TblStrPairs;
	std::map<LuaTable*, float>         TblFltPairs;
	std::map<std::string, LuaTable*>   StrTblPairs;
	std::map<std::string, std::string> StrStrPairs;
	std::map<std::string, float>       StrFltPairs;
	std::map<int, LuaTable*>           IntTblPairs;
	std::map<int, std::string>         IntStrPairs;
	std::map<int, float>               IntFltPairs;
};




struct LuaParser {
public:
	LuaParser(lua_State* state): luaState(state) {}
	~LuaParser();

	bool Execute(const std::string&, const std::string&);

	const LuaTable* GetRoot(const std::string& = "") const;
	const std::string& GetError(const std::string& = "") const;

private:
	lua_State* luaState;

	// root-table of most recently parsed file
	LuaTable* root;
	// error in most recently parsed file
	std::string error;

	std::map<std::string, LuaTable*> tables;
	std::map<std::string, std::string> errors;
};

#endif
