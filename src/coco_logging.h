#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <vector>

#ifndef LOGGING
#	define LOGGING
#	define COCO_INIT_LOG(x) coco::LoggerManager::getInstance()->init(x);
#	define COCO_LOG(x) coco::LogMessage(coco::Type::LOG, x).stream()
//#	define COCO_LOG(x) coco::LogMessage(coco::Type::LOG, x)
#	define COCO_ERR() coco::LogMessage(coco::Type::ERR, -1).stream()
#	define COCO_FATAL() coco::LogMessage(coco::Type::FATAL, -1).stream()
#	ifndef NDEBUG
#		define COCO_DEBUG() coco::LogMessage(coco::Type::DEBUG, 0).stream()
#	else
#		define COCO_DEBUG(x, ...)
#	endif
#endif

std::string getData();
std::string getTime();
bool isSpace(const char c);
float parseInt(const char*& token);
void split(const std::string &s, char delim, 
		   std::set<std::string> &elems);

namespace coco
{

enum Type
{
	ERR = 0,
	LOG = 1,
	DEBUG = 2,
	FATAL = 3
};

class LoggerManager
{
public:
	~LoggerManager();

	static LoggerManager* getInstance();

	void init(const std::string &config_file);

	inline bool isInit() const { return initialized_; }
	inline bool useStdout() const { return use_stdout_; }
	inline bool findLevel(int level) const
	{
		return levels_.find(level) != levels_.end();
	}
	inline bool findType(Type type)
	{
		return types_.find(type) != types_.end();
	} 

	void printToFile(const std::string &buffer);

private:
	LoggerManager(){}

	std::stringstream shell_stream_;
	std::ofstream file_stream_;
	std::string log_file_name_;
	int level_ = 0;
	std::set<int> levels_;
	std::set<Type> types_;
	bool initialized_ = false;
	bool use_stdout_ = true;
	
};

class LogMessage
{
public:
	LogMessage(Type type, int level)
		: level_(level), type_(type), stream_(NULL)
	{
		init();
	}

	~LogMessage()
	{
		flush();
	}

	std::ostream &stream()
	{
		return stream_;
	}

private:
	void init();
	void flush();
	void addPrefix();

private:
	int level_ = 0;
	Type type_;
	std::ostringstream buffer_;
	std::ostream stream_;
	const std::string init_not_called_err_ =
			"[COCO_FATAL]: Logger not initialize!\n\
			\tCall COCO_INIT_LOG(level, log_file)\n";
};
/*
template<class T>
LogMessage &operator<< (LogMessage &log, const T &msg)
{
	log.stream_ << msg;
	return log;
}
*/
} // end of namespace coco

