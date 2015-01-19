#include "coco_profiling.h"

namespace coco
{


Logger::Logger(std::string name) 
	: name_(name)
{
	start_time_ = clock();
}

Logger::~Logger()
{
	double elapsed_time = ((double)(clock() - start_time_)) / CLOCKS_PER_SEC;
	LoggerManager::getInstance()->addTime(name_, elapsed_time);
}

LoggerManager::LoggerManager(const std::string &log_file)
{
	if (log_file.compare("") != 0)
		log_file_.open(log_file);	
}

LoggerManager::~LoggerManager()
{
	log_file_.close();
}

LoggerManager* LoggerManager::getInstance()
{
	const std::string log_file = "";	
	static LoggerManager log(log_file);
	return &log;
}

void LoggerManager::addTime(std::string id, double elapsed_time)
{
	std::unique_lock<std::mutex> mlock(mutex_);
	time_list_[id].push_back(elapsed_time);
}

void LoggerManager::addServiceTime(std::string id, clock_t service_time)
{
	std::unique_lock<std::mutex> mlock(mutex_);
	service_time_list_[id].push_back(service_time);
}

void LoggerManager::printStatistic(std::string id) 
{
	std::cout << id << std::endl;
	if (time_list_[id].size() > 0) { 
		double tot_time = 0;
		for(auto &itr : time_list_[id])
			tot_time += itr;

		double avg_time = tot_time / time_list_[id].size();
		std::cout << "\tExecution time: " << avg_time <<
					 " s on " << time_list_[id].size() << " iterations\n";

		if (log_file_.is_open())
			log_file_ << id << " " << avg_time << 
						 " s on " << time_list_[id].size() << " iterations\n";
	}
// Service Time
	if (service_time_list_[id].size() > 0)
	{
		double tot_time = 0;
		clock_t tmp = service_time_list_[id][0];
		for (int i = 1; i < service_time_list_[id].size(); ++i)
		{
			tot_time += (double)(service_time_list_[id][i] - tmp) / CLOCKS_PER_SEC;
			tmp = service_time_list_[id][i];
		}
		
		double avg_time = tot_time / (service_time_list_[id].size() - 1);
		std::cout << "\tService Time: " << " " << avg_time << " s on " << service_time_list_[id].size() << " iterations\n";

		if (log_file_.is_open())
			log_file_ << "Service Time: " << " " << avg_time << " s on " << service_time_list_[id].size() << " iterations\n";
	}
}

void LoggerManager::printStatistics()
{
	std::unique_lock<std::mutex> mlock(mutex_);
	std::cout << "Statistics of " << time_list_.size() << " components\n";
	std::map<std::string, std::vector<double>>::iterator map_itr;
	for (map_itr = time_list_.begin(); map_itr != time_list_.end(); ++ map_itr)
		printStatistic(map_itr->first);
}

void LoggerManager::resetStatistics()
{
	std::unique_lock<std::mutex> mlock(mutex_);
	time_list_.clear();
	service_time_list_.clear();
	std::cout << "Statistics cleared\n";
}

}
