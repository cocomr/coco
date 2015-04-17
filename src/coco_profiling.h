#pragma once
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <mutex>

namespace coco
{
class Profiler
{
public:
	Profiler(std::string name);
	~Profiler();
private:
	std::string name_;
	clock_t start_time_;
};

class ProfilerManager
{
public:
	~ProfilerManager();
	static ProfilerManager* getInstance();
	void addTime(std::string id, double elapsed_time);
	void addServiceTime(std::string id, clock_t elapsed_time);
	void printStatistics();
	void printStatistic(std::string id);
	void resetStatistics();
private:
	ProfilerManager(const std::string &log_file);
	std::ofstream log_file_;
	std::map<std::string, std::vector<double>> time_list_;
	std::map<std::string, std::vector<clock_t>> service_time_list_;
	std::mutex mutex_;
	int counter_ = 0;
};

} // End of namespace coco