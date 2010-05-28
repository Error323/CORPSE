#include <algorithm>
#include <cctype>
#include <limits.h>
#include <stdexcept>
#include <boost/scoped_array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/spirit/utility/confix.hpp>
#include <boost/spirit/core.hpp>
#include <boost/spirit/symbols.hpp>
#include <boost/spirit/attribute.hpp>
#include <boost/spirit/dynamic.hpp>
#include <boost/spirit/phoenix.hpp>
#include <boost/spirit/utility/chset.hpp>
#include <boost/spirit/utility/lists.hpp>
#include <boost/spirit/iterator/file_iterator.hpp>
#include <boost/spirit/utility/grammar_def.hpp>
#include <boost/spirit/iterator/position_iterator.hpp>
#include <boost/spirit/phoenix/binders.hpp>
#include <boost/spirit/error_handling/exceptions.hpp>

#include "../Math/vec3fwd.hpp"
#include "../Math/vec4fwd.hpp"
#include "../Math/vec3.hpp"
#include "../Math/vec4.hpp"

#include "./TDFParser.hpp"
#include "./TDFGrammar.hpp"
#include "./FileHandler.hpp"

using boost::spirit::parse;
using boost::spirit::space_p;
using boost::spirit::comment_p;
using boost::spirit::parse_info;

CTDFParser::parse_error::parse_error( std::size_t l, std::size_t c, std::string const& f) throw():
	std::runtime_error(
		"Parse error in " + f +
		" at line " + boost::lexical_cast<std::string>(l) +
		" column " + boost::lexical_cast<std::string>(c) +
		"."
	),
	line(l),
	column(c),
	filename(f) {
}

CTDFParser::parse_error::parse_error( std::string const& line_of_error, std::size_t l, std::size_t c, std::string const& f) throw():
	std::runtime_error(
		"Parse error in " + f +
		" at line " + boost::lexical_cast<std::string>(l) +
		" column " + boost::lexical_cast<std::string>(c) +
		" near\n" + line_of_error
	),
	line(l),
	column(c),
	filename(f) {
}

CTDFParser::parse_error::parse_error( std::string const& message, std::string const& line_of_error, std::size_t l, std::size_t c, std::string const& f) throw():
	std::runtime_error(
		"Parse error '" + message + "' in " + f +
		" at line " + boost::lexical_cast<std::string>(l) +
		" column " + boost::lexical_cast<std::string>(c) +
		" near\n" + line_of_error
	),
	line(l),
	column(c),
	filename(f) {
}

CTDFParser::parse_error::~parse_error() throw() {}
std::size_t CTDFParser::parse_error::get_line() const { return line; }
std::size_t CTDFParser::parse_error::get_column() const { return column; }
const std::string& CTDFParser::parse_error::get_filename() const { return filename; }

void CTDFParser::TDFSection::print(std::ostream& out) const {
	for (std::map<std::string, TDFSection*>::const_iterator it = sections.begin(), e=sections.end(); it != e; ++it) {
		out << "[" << it->first << "]\n{\n";
		it->second->print(out);
		out << "}\n";
	}

	for (std::map<std::string,std::string>::const_iterator it = values.begin(), e=values.end(); it != e; ++it) {
		out << it->first  << "=" << it->second << ";\n";
	}
}

CTDFParser::TDFSection* CTDFParser::TDFSection::construct_subsection(const std::string& name) {
	std::map<std::string, TDFSection*>::iterator it = sections.find(name);

	if (it != sections.end())
		return it->second;
	else {
		TDFSection* ret = new TDFSection();
		sections[name] = ret;
		return ret;
	}
}

void CTDFParser::TDFSection::add_name_value(const std::string& name, const std::string& value) {
	values[name] = value;
}



CTDFParser::TDFSection::~TDFSection() {
	for (std::map<std::string, TDFSection*>::iterator it = sections.begin(), e=sections.end(); it != e; ++it)
		delete it->second;
}

void CTDFParser::print(std::ostream& out) const {
	root_section.print(out);
}

void CTDFParser::parse_buffer(char const* buf, std::size_t size) {
	std::list<std::string> junk_data;
	TDFGrammar grammar(&root_section, &junk_data);
	boost::spirit::parse_info<char const*> info;
	std::string message;
	typedef boost::spirit::position_iterator2<char const*> iterator_t;
	iterator_t error_it(buf, buf + size);

	try {
		info = boost::spirit::parse(
			buf
			, buf + size
			, grammar
			, space_p
			| comment_p("/*", "*/")
			| comment_p("//")
		);
	}
	catch (boost::spirit::parser_error<TDFGrammar::Errors, char const*>& e) {
		// thrown by assertion parsers in TDFGrammar

		switch(e.descriptor) {
			case TDFGrammar::semicolon_expected: message = "semicolon expected"; break;
			case TDFGrammar::equals_sign_expected: message = "equals sign in name value pair expected"; break;
			case TDFGrammar::square_bracket_expected: message = "square bracket to close section name expected"; break;
			case TDFGrammar::brace_expected: message = "brace or further name value pairs expected"; break;
			default: message = "unknown boost::spirit::parser_error exception"; break;
		};

		std::ptrdiff_t target_pos = e.where - buf;
		for (int i = 1; i < target_pos; ++i) {
			++error_it;
			if (error_it != (iterator_t(buf + i, buf + size))) {
				++i;
			}
		}
	}

	for( std::list<std::string>::const_iterator it = junk_data.begin(), e = junk_data.end(); it !=e ; ++it) {
		std::string temp = boost::trim_copy(*it);

		if (! temp.empty()) {
			// logOutput.Print("Junk in "+ filename +  ": " + temp);
		}
	}

	if (!message.empty())
		throw parse_error(message, error_it.get_currentline(), error_it.get_position().line, error_it.get_position().column, filename);

	// a different error might have happened
	if (!info.full) {
		std::ptrdiff_t target_pos = info.stop - buf;
		for (int i = 1; i < target_pos; ++i) {
			++error_it;

			if (error_it != (iterator_t(buf + i, buf + size))) {
				++i;
			}
		}

		throw parse_error( error_it.get_currentline(), error_it.get_position().line, error_it.get_position().column, filename );
	}
}


CTDFParser::CTDFParser(char const* buf, std::size_t size) {
  LoadBuffer( buf, size );
}

CTDFParser::CTDFParser(std::string const& filename) {
	LoadFile(filename);
}


void CTDFParser::LoadBuffer(char const* buf, std::size_t size) {
	filename = "buffer";
	parse_buffer(buf, size);
}

bool CTDFParser::LoadFile(std::string const& fname) {
	CFileHandler f(fname);

	if (!f.FileExists()) {
		return false;
	}

	filename = fname;
	boost::scoped_array<char> fbuf(new char[f.FileSize()]);

	f.Read(fbuf.get(), f.FileSize());
	parse_buffer(fbuf.get(), f.FileSize());

	return true;
}

std::string CTDFParser::SGetValueDef(std::string const& defaultvalue, std::string const& location) const {
	std::string value;
	bool found = SGetValue(value, location);
	if (!found)
		value = defaultvalue;
	return value;
}

bool CTDFParser::SGetValue(std::string& value, const std::string& location) const {
	std::string searchpath;
	std::vector<std::string> loclist = GetLocationVector(location);
	std::map<std::string, TDFSection*>::const_iterator sit = root_section.sections.find(loclist[0]);

	if (sit == root_section.sections.end()) {
		value = "Section " + loclist[0] + " missing in file " + filename;
		return false;
	}

	TDFSection* sectionptr = sit->second;
	searchpath = loclist[0];

	for (unsigned int i = 1; i < loclist.size() - 1; i++) {
		searchpath += '\\';
		searchpath += loclist[i];
		sit = sectionptr->sections.find(loclist[i]);

		if (sit == sectionptr->sections.end()) {
			value = "Section " + searchpath + " missing in file " + filename;
			return false;
		}

		sectionptr = sit->second;
	}

	searchpath += '\\';
	searchpath += loclist[loclist.size() - 1];

	std::map<std::string, std::string>::const_iterator vit = sectionptr->values.find(loclist[loclist.size()-1]);

	if (vit == sectionptr->values.end()) {
		value = "Value " + searchpath + " missing in file " + filename;
		return false;
	}

	std::string svalue = vit->second;
	value = svalue;
	return true;
}

const std::map<std::string, std::string>& CTDFParser::GetAllValues(std::string const& location) const {
	static std::map<std::string, std::string> emptymap;

	std::string searchpath;
	std::vector<std::string> loclist = GetLocationVector(location);
	std::map<std::string, TDFSection*>::const_iterator sit = root_section.sections.find(loclist[0]);

	if (sit == root_section.sections.end()) {
		return emptymap;
	}

	TDFSection* sectionptr = sit->second;
	searchpath = loclist[0];

	for (unsigned int i = 1; i < loclist.size(); i++) {
		searchpath += '\\';
		searchpath += loclist[i];
		sit = sectionptr->sections.find(loclist[i]);

		if (sit == sectionptr->sections.end()) {
			// logOutput.Print ("Section " + searchpath + " missing in file " + filename);
			return emptymap;
		}

		sectionptr = sit->second;
	}

	return sectionptr->values;
}

std::vector<std::string> CTDFParser::GetSectionList(std::string const& location) const {
	std::vector<std::string> loclist = GetLocationVector(location);
	std::vector<std::string> returnvec;

	const std::map<std::string, TDFSection*>* sectionsptr = &root_section.sections;

	if (loclist[0].compare("") != 0) {
		std::string searchpath;

		for (unsigned int i = 0; i < loclist.size(); i++) {
			searchpath += loclist[i];

			if (sectionsptr->find(loclist[i]) == sectionsptr->end()) {
				// logOutput.Print ("Section " + searchpath + " missing in file " + filename);
				return returnvec;
			}

			sectionsptr = &sectionsptr->find(loclist[i])->second->sections;
			searchpath += '\\';
		}
	}

	std::map<std::string,TDFSection*>::const_iterator it;
	for (it = sectionsptr->begin(); it != sectionsptr->end(); it++) {
		returnvec.push_back(it->first);
	}

	return returnvec;
}

bool CTDFParser::SectionExist(const std::string& location) const {
	std::vector<std::string> loclist = GetLocationVector(location);
	std::map<std::string, TDFSection*>::const_iterator sit = root_section.sections.find(loclist[0]);

	if (sit == root_section.sections.end()) {
		return false;
	}

	TDFSection* sectionptr = sit->second;

	for (unsigned int i = 1; i < loclist.size(); i++) {
		sit = sectionptr->sections.find(loclist[i]);

		if (sit == sectionptr->sections.end()) {
			return false;
		}

		sectionptr = sectionptr->sections[loclist[i]];
	}

	return true;
}

std::vector<std::string> CTDFParser::GetLocationVector(const std::string& location) const {
	std::vector<std::string> loclist;
	std::string::size_type start = 0;
	std::string::size_type next = 0;

	while ((next = location.find_first_of("\\", start)) != std::string::npos) {
		loclist.push_back(location.substr(start, next - start));
		start = next + 1;
	}

	loclist.push_back(location.substr(start));
	return loclist;
}


vec3f CTDFParser::GetVec3(vec3f def, const std::string& location) const {
	std::string s = SGetValueDef("", location);

	if (s.empty())
		return def;

	vec3f v;
	ParseArray(s, &v.x, 3);
	return v;
}

vec4f CTDFParser::GetVec4(vec4f def, const std::string& location) const {
	std::string s = SGetValueDef("", location);

	if (s.empty())
		return def;

	vec4f v;
	ParseArray(s, &v.x, 4);
	return v;
}
