/**
 * Project: CoCo
 * Copyright (c) 2016, Scuola Superiore Sant'Anna
 *
 * Authors: Filippo Brizzi <fi.brizzi@sssup.it>, Emanuele Ruffaldi
 * 
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
 */

#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <vector>
#include <cstring>
#include <unordered_map>
#include <chrono>
#include <ctime>
#include <cassert>

#include "coco/util/threading.h"
#include "coco/web_server/web_server.h"

#include "coco/util/generics.hpp"

inline std::string instantiationName()
{
    return "";
}

#ifndef LOGGING
#   define LOGGING
#   define COCO_LOG_INFO() COCO_LOG(0) << coco::util::LoggerManager::instance()->info();
#   define COCO_INIT_LOG(x) coco::util::LoggerManager::instance()->init(x);

#   define GET_MACRO(_1,_2,NAME,...) NAME
#   define COCO_LOG(...) GET_MACRO(__VA_ARGS__, COCO_LOG2, COCO_LOG1)(__VA_ARGS__)
#   define COCO_LOG1(x) coco::util::LogMessage(coco::util::Type::LOG, x, instantiationName()).stream()
#   define COCO_LOG2(x, y) coco::util::LogMessage(coco::util::Type::LOG, x, y).stream()

#   define COCO_ERR() coco::util::LogMessage(coco::util::Type::ERR, -1, instantiationName()).stream()
//#   define COCO_FATAL() coco::util::LogMessage(coco::util::Type::FATAL, -1, coco::util::task_name(this)).stream()
#   define COCO_FATAL() coco::util::LogMessage(coco::util::Type::FATAL, -1, instantiationName()).stream()
#   define COCO_LOG_SAMPLE(x, y) coco::util::LogMessageSampled(x, y).stream()
#   ifndef NDEBUG
#       define COCO_DEBUG(x) coco::util::LogMessage(coco::util::Type::DEBUG, 0, x).stream()
#   else
#       define COCO_DEBUG(x) coco::util::LogMessage(coco::util::Type::DEBUG, 0, x).stream()
#   endif
#endif



namespace coco
{
namespace util
{

inline std::string getDataAndTime()
{
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    return  std::ctime(&time);
}

inline std::string getTime()
{
    time_t t = time(0);   // get time now
    struct tm * now = localtime(&t);
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
inline bool isSpace(const char c)
{
    return (c == ' ') || (c == '\t');
}
inline int parseInt(const char*& token)
{
    token += strspn(token, " \t");
    int i = static_cast<int>(atoi(token));
    token += strcspn(token, " \t\r");
    return i;
}
inline void split(const std::string &s, char delim,
                  std::unordered_set<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        elems.insert(item);
    }
}

enum class Type
{
    ERR = 0,
    LOG = 1,
    DEBUG = 2,
    FATAL = 3,
    NO_PRINT = 4,
};

class COCOEXPORT LoggerManager
{
public:
    ~LoggerManager()
    {
        if (file_stream_.is_open())
        file_stream_.close();
    }

    static LoggerManager* instance();

    // TODO ADD DEFAULT INIT FILE!
    void init(const std::string &config_file)
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

    void init()
    {
        levels_.insert(0);
        // types_.insert(ERR);
        types_.insert(Type::LOG);
        use_stdout_ = true;
        initialized_ = true;
        return;
    }

    std::string info() const
    {
        std::stringstream out;
        out << "\n================================================\n";
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

    void setLevels(const std::unordered_set<int> &levels) { levels_ = levels; }

    void setTypes(const std::unordered_set<Type, enum_hash> &types) {types_ = types; }

    void setOutLogFile(const std::string &file)
    {
        log_file_name_ = file;
        if (file_stream_.is_open())
            file_stream_.close();
        file_stream_.open(log_file_name_);
    }

    void setUseStdout(bool use = true)
    {
        use_stdout_ = use;
    }

    void addLevel(int level) { levels_.insert(level); }

    bool removeLevel(int level) { return levels_.erase(level) > 0; }

    void addType(Type type) { types_.insert(type); }

    bool removeType(Type type) { return types_.erase(type) > 0; }

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

    void printToFile(const std::string &buffer)
    {
        if (file_stream_.is_open())
        {
            file_stream_ << buffer << std::endl;
            file_stream_.flush();
        }
    }

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
    LoggerManager() {}
    std::stringstream shell_stream_;
    std::ofstream file_stream_;
    std::string log_file_name_;
    std::unordered_set<int> levels_;
    std::unordered_set<Type, util::enum_hash> types_;
    bool initialized_ = false;
    bool use_stdout_ = true;

    std::unordered_map<std::string, int> sampled_messages_;
};

class COCOEXPORT LogMessage
{
public:
    LogMessage(Type type, int level, std::string name = "")
        : level_(level), type_(type), stream_(NULL), name_(name)
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
    void init()
    {
        stream_.rdbuf(buffer_.rdbuf());
        addPrefix();
    }
    void flush()
    {
        if (WebServer::isRunning())
        {
            buffer_ << std::endl;
            stream_.flush();
            WebServer::addLogString(buffer_.str());
        }
        else
        {
            if (type_ == Type::NO_PRINT)
                return;

            if (type_ == Type::LOG)
            {
                if (!LoggerManager::instance()->findLevel(level_))
                    return;
            }
            else if (!LoggerManager::instance()->findType(type_))
            {
                return;
            }
            stream_.flush();

            if (LoggerManager::instance()->useStdout())
            {
                if (type_ == Type::LOG || type_ == Type::DEBUG)
                {
                    std::cout << buffer_.str() << std::endl;
                    // TODO add mutex
                    std::cout.flush();
                }
                else
                {
                    std::cerr << buffer_.str() << std::endl;
                    std::cerr.flush();
                }
            }
            LoggerManager::instance()->printToFile(buffer_.str());
        }


        if (type_ == Type::FATAL)
        {
            std::cerr << "FATAL EXIT\n";
            exit(1);
        }
    }
    void addPrefix()
    {
        switch (type_)
        {
            case Type::LOG:
                if (name_.empty())
                    buffer_ << "[LOG " << level_ << "] ";
                else
                    buffer_ << "[LOG " << level_ << ", " << name_ << "] ";
                break;
            case Type::DEBUG:
                if (name_.empty())
                    buffer_ << "[DEBUG] ";
                else
                    buffer_ << "[DEBUG " << name_ << "] ";
                break;
            case Type::ERR:
                if (name_.empty())
                    buffer_ << "[ERR]   ";
                else
                    buffer_ << "[ERR " << name_ << "] ";
                break;
            case Type::FATAL:
                if (name_.empty())
                    buffer_ << "[FATAL] ";
                else
                    buffer_ << "[FATAL " << name_ << "] ";
                break;
            case Type::NO_PRINT:
                return;
        }
        buffer_ << getTime() << ": ";
    }

private:
    int level_ = 0;
    Type type_;
    std::ostringstream buffer_;
    std::ostream stream_;
    std::string name_;

    const std::string init_not_called_err_ =
            "[COCO_FATAL]: Logger not initialize!\n"
            "\tCall COCO_INIT_LOG(level, log_file)\n";
};

class COCOEXPORT LogMessageSampled
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
    void init()
    {
        count_ = LoggerManager::instance()->sampledMessageCount(id_);
        if (count_ == -1)
            LoggerManager::instance()->setSampledMessageCount(id_, 0);
        else
            LoggerManager::instance()->setSampledMessageCount(id_, ++count_);

        stream_.rdbuf(buffer_.rdbuf());

        buffer_ << "[LOG SAMPLED " << sample_rate_ << "] ";
    }
    void flush()
    {
        if (count_ < sample_rate_)
            return;

        LoggerManager::instance()->setSampledMessageCount(id_, 0);

        stream_.flush();

        if (WebServer::isRunning())
        {
            buffer_ << std::endl;
            WebServer::addLogString(buffer_.str());
        }
        else
        {
            if (LoggerManager::instance()->useStdout())
            {
                std::cout << buffer_.str() << std::endl;
                std::cout.flush();
            }
            LoggerManager::instance()->printToFile(buffer_.str());
        }
    }

private:
    std::string id_;
    int sample_rate_;
    int count_;
    std::ostringstream buffer_;
    std::ostream stream_;
};

}  // end of namespace util
}  // end of namespace coco

