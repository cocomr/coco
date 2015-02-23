#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <vector>

#ifdef LOGGING
#	define COCO_INIT_LOG(x) coco::Logger::getInstance()->init(x);
//#   define GET_MACRO(_1, NAME, ...) NAME
//#	define COCO_INIT_LOG(...) GET_MACRO(__VA_ARGS__, COCO_INIT_LOG1, COCO_INIT_LOG2)(__VA_ARGS__)
#	define COCO_LOG(x, ...) coco::Logger::getInstance()->log(coco::Logger::LOG, x, ##__VA_ARGS__);
#	define COCO_ERR(x, ...) coco::Logger::getInstance()->log(coco::Logger::ERR, 0, x, ##__VA_ARGS__);
#	define COCO_FATAL(x, ...) coco::Logger::getInstance()->log(coco::Logger::FATAL, 0, x, ##__VA_ARGS__);
#	ifndef NDEBUG
#		define COCO_DEBUG(x, ...) coco::Logger::getInstance()->log(coco::Logger::DEBUG, 0, x, ##__VA_ARGS__);
#	else
#		define COCO_DEBUG(x, ...)
#	endif
#else
#	define COCO_INIT_LOG(x)
#	define COCO_LOG(x, ...) 
#	define COCO_DEBUG(x, ...)
#   define COCO_ERR(x, ...)
#   define COCO_FATAL(x, ...)
#endif

namespace coco
{

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

  	ss << now->tm_hour << ":"
  	   << now->tm_min  << ":"
  	   << now->tm_sec;
  	return ss.str();
}

inline bool isSpace(const char c) {
	return (c == ' ') || (c == '\t');
}

inline float parseInt(const char*& token)
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
    while (std::getline(ss, item, delim)) {
        elems.insert(item);
    }
}

class Logger
{
public:
	enum Type
	{
		ERR = 0,
		LOG = 1,
		DEBUG = 2,
		FATAL = 3
	};

	~Logger()
	{
		if (file_stream_.is_open())
			file_stream_.close();
	}

	static Logger* getInstance()
	{
		static Logger log;
		return &log;
	}

	void init(const std::string &config_file)
	{
		std::ifstream config_stream;
		config_stream.open(config_file);
		if (!config_stream.is_open())
		{
			std::cerr << "[COCO_FATAL]: Configuration file " << 
						 config_file << " doesn't exist.\n";
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
    				split(line, '|', types_);
    			}
    		}
		}
		initialized_ = true;
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
		std::cout << out.str() << std::endl;
		
	}

	void addPrefix(Type type, std::stringstream &stream)
	{
		switch (type)
		{
			case LOG:
				stream << "[COCO_LOG] ";
				break;
			case DEBUG:
				stream << "[COCO_DEBUG] ";
				break;
			case ERR:
				stream << "[COCO_ERR] ";
				break;
			case FATAL:
				stream << "[COCO_FATAL] ";
				break;
		}
		stream << getTime() << ": ";
	}

	void log(Type type, int level)
	{
		if (!initialized_)
		{
			std::cerr << init_not_called_err_;
			return;
		}
		if (shell_stream_.str().empty())
		{
			return;
		}

		std::stringstream final_stream;
		addPrefix(type, final_stream);
		final_stream << shell_stream_.str();

		if (type == LOG || type == DEBUG)
		{
			std::cout << final_stream.str();
			std::cout.flush();
		}
		else
		{
			std::cerr << final_stream.str();
			std::cerr.flush();
		}
		
		if (file_stream_.is_open())
		{
			file_stream_ << final_stream.str();
			file_stream_.flush();
		}
		shell_stream_.clear();
		shell_stream_.str(std::string());
	}

	template<typename T, typename... Args>
	void log(Type type, int level, T value, Args... args)
	{
		if (!initialized_)
		{
			std::cerr << init_not_called_err_;
			return;
		}

		if (type == LOG)
		{
			if (levels_.find(level) != levels_.end())
				return;
		}
		else if (type == DEBUG)
		{
			if (types_.find("DEBUG") == types_.end())
				return;
		}
		else if (type == ERR)
		{
			if (types_.find("ERR") == types_.end())
				return;
		}

		shell_stream_ << value;
		log(type, level, args ...);
	}
	
private:
	Logger(){}

	std::stringstream shell_stream_;
	std::ofstream file_stream_;
	std::string log_file_name_;
	int level_ = 0;
	std::set<int> levels_;
	std::set<std::string> types_;
	bool initialized_ = false;
	const std::string init_not_called_err_ =
			"[COCO_FATAL]: Logger not initialize!\n\
			\tCall COCO_INIT_LOG(level, log_file)\n";
};

} // end of namespace coco