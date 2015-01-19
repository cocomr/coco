#pragma once
#include <vector>
#include <map>
#include <iostream>
#include <fstream>

namespace coco
{
class Logger
{
public:
	Logger(std::string name);
	~Logger();
private:
	std::string name_;
	clock_t start_time_;
};

class LoggerManager
{
public:
	~LoggerManager();
	static LoggerManager* getInstance();
	void addTime(std::string id, double elapsed_time);
	void addServiceTime(std::string id, clock_t elapsed_time);
	void printStatistics();
	void printStatistic(std::string id);
	void resetStatistics();
private:
	LoggerManager(const std::string &log_file);
	std::ofstream log_file_;
	std::map<std::string, std::vector<double>> time_list_;
	std::map<std::string, std::vector<clock_t>> service_time_list_;
	std::mutex mutex_;
};

} // End of namespace coco