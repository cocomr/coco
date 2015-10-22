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

#include "logging.h"
#include <time.h>

std::string getData()
{
    time_t t = time(0);   // get time now
  	struct tm * now = localtime( & t );
  	std::stringstream ss;

  	ss << now->tm_mday << '-'
  	   << (now->tm_mon + 1) << '-'
  	   << (now->tm_year + 1900);
       
       
    return ss.str();
}

std::string getTime()
{
	time_t t = time(0);   // get time now
  	struct tm * now = localtime( & t );
  	std::stringstream ss;

  	if (now->tm_hour < 10)
  		ss << 0;
  	ss << now->tm_hour << ":";
  	if (now->tm_min < 10)
  		ss << 0;
  	ss << now->tm_min  << ":";
  	if (now->tm_sec < 10)
  		ss << 0;
  	ss << now->tm_sec; 

  	return ss.str();
}

bool isSpace(const char c)
{
	return (c == ' ') || (c == '\t');
}

float parseInt(const char*& token)
{
	token += strspn(token, " \t");
	int i = (int)atoi(token);
	token += strcspn(token, " \t\r");
	return i;
}

void split(const std::string &s, char delim, 
		   std::set<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        elems.insert(item);
    }
}

static coco::util::LoggerManager *singleton;

namespace coco
{
namespace util
{

LoggerManager::LoggerManager()
{

}

LoggerManager::~LoggerManager()
{
	if (file_stream_.is_open())
		file_stream_.close();
}

LoggerManager* LoggerManager::getInstance()
{
	//static LoggerManager log;
	if (!singleton)
		singleton = new LoggerManager();
	std::cout << "LOGGER: " << (void *)(singleton) << std::endl;
	return singleton;
	//return &log;
}

void LoggerManager::init()
{
	std::cout << "INIT CALLED\n";
	levels_.insert(0);
	//types_.insert(ERR);
	types_.insert(LOG);
	use_stdout_ = true;
	initialized_ = true;
	return;
}

void LoggerManager::init(const std::string &config_file)
{
	std::ifstream config_stream;
	if (!config_file.empty())
	{
		config_stream.open(config_file);
		if (!config_stream.is_open())
		{
			std::cerr << "[COCO_FATAL]: Configuration file " << 
					 config_file << " doesn't exist.\n";
			return;
		}
	}
	else
	{
		init();
		return;
	}
	int max_chars = 8192;  // Alloc enough size.
	std::vector<char> buf(max_chars);
	while (config_stream.peek() != -1)
	{
		config_stream.getline(&buf[0], max_chars);
		std::string line_buf(&buf[0]);

		if (line_buf.size() > 0)
		{
	      if (line_buf[line_buf.size() - 1] == '\n') 
	      	line_buf.erase(line_buf.size() - 1);
	    }
	    if (line_buf.size() > 0)
	    {
	    	if (line_buf[line_buf.size()-1] == '\r') 
				line_buf.erase(line_buf.size() - 1);
	    }
	    // Skip if empty line.
	    if (line_buf.empty())
	    {
	      continue;
	    }

	    const char* token = line_buf.c_str();
		token += strspn(token, " \t");

		if (token[0] == '\0') continue; // empty line
		if (token[0] == '#') continue;  // comment line

		if (token[0] == '*' && isSpace(token[1]))
		{
			token += 2;
			if (strncmp(token, "LEVELS = ", 9) == 0)
			{
				token += 9;
				while (token[0] != '\0')
				{
					int l = parseInt(token);
					if (l != -1)
						levels_.insert(l);
				}
				levels_.insert(0);
			}
			if(strncmp(token, "CONFIGURATION_FILE = ", 21) == 0)
			{
				token += 21;
				char name_buf[4096];
				sscanf(token, "%s", name_buf);
				log_file_name_ = std::string(name_buf);
				if (!log_file_name_.empty())
					file_stream_.open(log_file_name_);
			}
			if(strncmp(token, "TYPES = ", 8) == 0)
			{
				token += 8;
				std::string line(token);
				std::set<std::string> types;
				//split(line, '|', types_);
				split(line, '|', types);
				for (auto t : types)
				{
					if (t == "DEBUG " || t == " DEBUG" || t == " DEBUG ")
						types_.insert(DEBUG);
					else if (t == "ERR " || t == " ERR" || t == " ERR ")
						types_.insert(ERR);
				}
				types_.insert(FATAL);
			}
		}
	}
}

std::string LoggerManager::info() const
{
	std::stringstream out;
	out << "================================================\n";
	out << "[COCO_INIT_LOG]:\nToday, " << getData() << ", " 
		<< getTime() << "\n"
	    << "Coco Logger initialized!\n"
	    << "Enabled levels for COCO_LOG:\n\t";
	for (auto l : levels_)
		out << l << " ";
	out << "\nEnabled types:\n\t";
	for (auto t : types_)
		out << t << " ";
	out << "\n";
	if (file_stream_.is_open())
		out << "Log written to file: " << log_file_name_ << "\n";
	out << "================================================\n";
	return out.str();
}

void LoggerManager::setOutLogFile(const std::string &file)
{ 
	log_file_name_ = file;
	if (file_stream_.is_open())
		file_stream_.close();
	file_stream_.open(log_file_name_);
}

void LoggerManager::printToFile(const std::string &buffer)
{
	// TODO add thread safty
	if (file_stream_.is_open())
	{
		file_stream_ << buffer << std::endl;
		file_stream_.flush();
	}
}

void LogMessage::init()
{
	stream_.rdbuf(buffer_.rdbuf());
	addPrefix();
}

void LogMessage::flush()
{
	if (type_ == NO_PRINT)
		return;
	if (!LoggerManager::getInstance()->isInit())
	{
		std::cout << "LOGGER NOT INIT: " << buffer_.str() << std::endl;
		//COCO_INIT_LOG();
		return;
	}
	if (type_ == LOG)
	{
		if (!LoggerManager::getInstance()->findLevel(level_))
			return;
	}
	else if (!LoggerManager::getInstance()->findType(type_))
	{
		return;
	}
	stream_.flush();
	if (LoggerManager::getInstance()->useStdout())
	{
		if (type_ == LOG || type_ == DEBUG)
		{
			std::cout << buffer_.str() << std::endl;
			std::cout.flush();
		}
		else
		{
			std::cerr << buffer_.str() << std::endl;
			std::cerr.flush();
		}
	}
	LoggerManager::getInstance()->printToFile(buffer_.str());

    if (type_ == FATAL)
	{
		exit(1);
	}
}

void LogMessage::addPrefix()
{
	switch (type_)
	{
		case LOG:
			buffer_ << "[LOG " << level_ << "] ";
			break;
		case DEBUG:
			if (name_.empty())
				buffer_ << "[DEBUG] ";
			else
				buffer_ << "[DEBUG " << name_ << "] ";
			break;
		case ERR:
			buffer_ << "[ERR]   ";
			break;
		case FATAL:
			buffer_ << "[FATAL] ";
			break;
	}
	buffer_ << getTime() << ": ";
}

void LogMessageSampled::init()
{
    count_ = LoggerManager::getInstance()->sampledMessageCount(id_);
    if (count_ == -1)
        LoggerManager::getInstance()->setSampledMessageCount(id_, 0);
    else
        LoggerManager::getInstance()->setSampledMessageCount(id_, ++count_);

    stream_.rdbuf(buffer_.rdbuf());

    buffer_ << "[LOG SAMPLED " << sample_rate_ << "] ";
}

void LogMessageSampled::flush()
{
    if (count_ < sample_rate_)
        return;

    LoggerManager::getInstance()->setSampledMessageCount(id_, 0);

    if (!LoggerManager::getInstance()->isInit())
    {
        COCO_INIT_LOG();
    }

    stream_.flush();
    if (LoggerManager::getInstance()->useStdout())
    {
        std::cout << buffer_.str() << std::endl;
        std::cout.flush();
    }
    LoggerManager::getInstance()->printToFile(buffer_.str());

}

} // end of namespace util
} // end of namespace coco
