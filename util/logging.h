/**
Copyright 2015, Filippo Brizzi"
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

@Author 
Filippo Brizzi, PhD Student
fi.brizzi@sssup.it
Emanuele Ruffaldi, PhD
e.ruffaldi@sssup.it

PERCRO, (Laboratory of Perceptual Robotics)
Scuola Superiore Sant'Anna
via Luigi Alamanni 13D, San Giuliano Terme 56010 (PI), Italy
*/

#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <vector>
#include <cstring>
#include <unordered_map>

#ifndef LOGGING
#	define LOGGING
#	define COCO_LOG_INFO() std::cout << coco::util::LoggerManager::getInstance()->info() << std::endl;
#	define COCO_INIT_LOG(x) coco::util::LoggerManager::getInstance()->init(x);
#	define COCO_LOG(x) coco::util::LogMessage(coco::util::Type::LOG, x).stream()
#	define COCO_ERR() coco::util::LogMessage(coco::util::Type::ERR, -1).stream()
#	define COCO_FATAL() coco::util::LogMessage(coco::util::Type::FATAL, -1).stream()
#   define COCO_SAMPLE(x, y) coco::util::LogMessageSampled(x, y).stream()
#	ifndef NDEBUG
#		define COCO_DEBUG() coco::util::LogMessage(coco::util::Type::DEBUG, 0).stream()
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
namespace util
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

	// TODO ADD DEFAULT INIT FILE!
	void init(const std::string &config_file);
	void init();
	std::string info() const;
	void setLevels(const std::set<int> &levels) { levels_ = levels; }
	void setTypes(const std::set<Type> &types) {types_ = types; }
	void setOutLogFile(const std::string &file);

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

    // TODO protect with guard
    int sampledMessageCount(std::string id)
    {
        auto count = sampled_messages_.find(id);
        if (count != sampled_messages_.end())
            return count->second;
        else
            return -1;
    }

    // TODO protect with guard
    void setSampledMessageCount(std::string id, int count)
    {
        sampled_messages_[id] = count;
    }

private:
	LoggerManager();
	std::stringstream shell_stream_;
	std::ofstream file_stream_;
	std::string log_file_name_;
	int level_ = 0;
	std::set<int> levels_;
	std::set<Type> types_;
	bool initialized_ = false;
	bool use_stdout_ = true;
	
    std::unordered_map<std::string, int> sampled_messages_;
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

class LogMessageSampled
{
public:
    LogMessageSampled(std::string id, int sample_rate)
        : id_(id), sample_rate_(sample_rate), stream_(NULL)
    {
        init();
    }

    ~LogMessageSampled()
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

private:
    std::string id_;
    int sample_rate_;
    int count_;
    std::ostringstream buffer_;
    std::ostream stream_;
};

} // End of namespace util
} // end of namespace coco

