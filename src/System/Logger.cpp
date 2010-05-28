#include <ctime>
#include <iostream>
#include <sstream>

#include "./Logger.hpp"
#include "./LuaParser.hpp"

CLogger::CLogger(LuaParser* parser):
	dir(parser->GetRoot()->GetTblVal("general")->GetStrVal("logDir", "data/logs/")), name("") {

	name = dir + GetLogName();
	log.open(name.c_str());

	std::cout << "[CLogger::CLogger] logging to " << name << std::endl;
}

std::string CLogger::GetLogName() {
	if (!name.empty()) {
		return name;
	}

	time_t t;
	time(&t);
	struct tm* lt = localtime(&t);

	char buf[1024] = {0};

	snprintf(
		buf,
		1024 - 1,
		"PFFG_%02d-%02d-%04d_%02d%02d.txt",
		lt->tm_mon + 1,
		lt->tm_mday,
		lt->tm_year + 1900,
		lt->tm_hour,
		lt->tm_min
	);

	/*
	// TODO: precision control
	std::stringstream ss;
		ss << "PFFG_";
		ss << lt->tm_mon + 1;
		ss << "-";
		ss << lt->tm_mday;
		ss << "-";
		ss << lt->tm_year + 1900;
		ss << "_";
		ss << lt->tm_hour;
		ss << lt->tm_min;
		ss << ".txt";

	return (ss.str());
	*/

	return (std::string(buf));
}
