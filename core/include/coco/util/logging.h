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


#ifndef COCO_LOGGING
#   define COCO_LOGGING
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
        file_stream_.close();
    }

    static LoggerManager* instance();

    void init(const std::string &config_file);
    void init();

    std::string info() const;

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

    inline bool findType(Type type) const
    {
        return types_.find(type) != types_.end();
    }

    void printToFile(const std::string &buffer);

    void printToStdout(Type type, std::ostream & ons, const std::string & s);

    // TODO protect with guard
    int sampledMessageCount(std::string id) const
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
        auto lm = LoggerManager::instance();
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
                if (!lm->findLevel(level_))
                    return;
            }
            else if (!lm->findType(type_))
            {
                return;
            }
            stream_.flush();

            if(lm->useStdout())
            {
                lm->printToStdout(type_,(type_ == Type::LOG || type_ == Type::DEBUG) ? std::cout : std::cerr,buffer_.str());
            }
            lm->printToFile(buffer_.str());
        }


        if (type_ == Type::FATAL)
        {
            lm->printToStdout(Type::FATAL,std::cerr,"FATAL EXIT\n");
            exit(1);
        }
    }
    void addPrefix();

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

