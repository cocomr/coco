#include "coco/util/logging.h"
#include <termcolor/termcolor.hpp>

/*
Old
#ifdef NONWIN32
#define CCOLOR_RST ""
#define CCOLOR_RED ""
#define CCOLOR_GREEN ""
#define CCOLOR_BLU ""
#define CCOLOR_CYAN ""
#else
#define CCOLOR_RST  "\x1B[0m"
#define CCOLOR_RED  "\x1B[31m"
#define CCOLOR_GREEN  "\x1B[32m"
#define CCOLOR_BLU  "\x1B[34m"
#define CCOLOR_CYAN  "\x1B[36m"
#endif
*/


namespace coco
{
	namespace util
	{
		static LoggerManager *xlog;
		static std::mutex singletonmutex_;
		        
    void LogMessage::addPrefix()
    {
        switch (type_)
        {
            case Type::LOG:
                if (name_.empty())
                    buffer_ <<   "[LOG " << level_ << "] "  ;
                else
                    buffer_ <<  "[LOG " << level_ << ", " << name_ << "] "  ;
                break;
            case Type::DEBUG:
                if (name_.empty())
                    buffer_ <<  "[DEBUG] " ;
                else
                    buffer_ <<  "[DEBUG " << name_ << "] " ;
                break;
            case Type::ERR:
                if (name_.empty())
                    buffer_ <<  "[ERR]   " ;
                else
                    buffer_ <<  "[ERR " << name_ << "] " ;
                break;
            case Type::FATAL:
                if (name_.empty())
                    buffer_ <<  "[FATAL] " ;
                else
                    buffer_ <<  "[FATAL " << name_ << "] " ;
                break;
            case Type::NO_PRINT:
                return;
        }
        buffer_ << getTime() << ": ";
    }		        
		LoggerManager* LoggerManager::instance()
		{
	        std::lock_guard<std::mutex> g(singletonmutex_);
			if(!xlog)
			{
				xlog = new LoggerManager();
			}
		    return xlog;
		}

	    void LoggerManager::printToFile(const std::string &buffer)
	    {
	        if (file_stream_.is_open())
	        {
		        std::lock_guard<std::mutex> g(singletonmutex_);
	            file_stream_ << buffer << std::endl; // endl==newline+flush
	        }
	    }		

	    void LoggerManager::printToStdout(Type t, std::ostream & ons, const std::string & s)
	    {
	        int k = s.find(']');
	        std::lock_guard<std::mutex> g(singletonmutex_);
	        if(k >= 0)
	        {
	        	switch(t)
	        	{
	        		case Type::DEBUG: 
	        			ons << termcolor::green << s.substr(0,k+1) << termcolor::reset << s.substr(k+1) << std::endl;
	        			break;
	        		case Type::ERR:
	        		case Type::FATAL: 
	        			ons << termcolor::red << s.substr(0,k+1) << termcolor::reset << s.substr(k+1) << std::endl;
	        			break;
	        		case Type::LOG: 
	        			ons << termcolor::cyan << s.substr(0,k+1) << termcolor::reset << s.substr(k+1) << std::endl;
	        			break;
	        		default:
				    	ons << s << std::endl;
				    	break;
	        	}
	        }
	        else
	        {
		    	ons << s << std::endl;
		    }
	    }

	    std::string LoggerManager::info() const
	    {
	        std::stringstream out;
	        out << "\n================================================\n";
	        out << "LoggerManager instance " << (void*)this << std::endl;
	        out << "[COCO_INIT_LOG]:\nToday, " << getDataAndTime() << "\n"
	            << "Coco Logger initialized!\n"
	            << "Enabled levels for COCO_LOG:\n\t";
	        for (auto l : levels_)
	            out << l << " ";
	        out << "\nEnabled types:\n\t";
	        for (auto t : types_)
	            out << static_cast<int>(t) << " ";
	        out << "\n";
	        if (file_stream_.is_open())
	            out << "Log written to file: " << log_file_name_ << "\n";
	        out << "================================================\n";
	        return out.str();	    	
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

	            if (token[0] == '\0') continue;  // empty line
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
	                if (strncmp(token, "CONFIGURATION_FILE = ", 21) == 0)
	                {
	                    token += 21;
	                    char name_buf[4096];
	                    sscanf(token, "%s", name_buf);
	                    log_file_name_ = std::string(name_buf);
	                    if (!log_file_name_.empty())
	                        file_stream_.open(log_file_name_);
	                }
	                if (strncmp(token, "TYPES = ", 8) == 0)
	                {
	                    token += 8;
	                    std::string line(token);
	                    std::unordered_set<std::string> types;
	                    split(line, '|', types);
	                    for (auto t : types)
	                    {
	                        if (t == "DEBUG " || t == " DEBUG" || t == " DEBUG ")
	                            types_.insert(Type::DEBUG);
	                        else if (t == "ERR " || t == " ERR" || t == " ERR ")
	                            types_.insert(Type::ERR);
	                    }
	                    types_.insert(Type::FATAL);
	                }
	            }
	        }
	    }

	    void LoggerManager::init()
	    {
	        levels_.insert(0);
	        // types_.insert(ERR);
	        types_.insert(Type::LOG);
	        use_stdout_ = true;
	        initialized_ = true;
	        return;
	    }	    

	}
}